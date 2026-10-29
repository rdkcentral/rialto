# Proposal: Implement Metrics Passthrough Relay (GStreamer Bus → Server → Client)

**Proposal: Establish metrics relay pathway from GStreamer SoC elements through Rialto-Server IPC to client applications without intermediate processing or buffering**

---

## Why

**Problem Statement:**
- Playback quality is invisible to client applications—while GStreamer SoC elements capture frame drops, decode errors, and other metrics at the hardware level, Rialto-Server currently has no way to surface these signals to Netflix DPI, Cobalt, WPE-WebKit, or other consumers
- Without structured metrics visibility, applications cannot diagnose playback degradation, implement real-time adaptive bitrate strategies, or meet W3C compliance requirements for video analytics
- The metrics gap forces developers to rely on timeout patterns and guesswork instead of data-driven decisions about quality, resulting in poor user experience and longer debugging cycles
- Rialto lacks the formalized plumbing (subscription management, query endpoints, event notification) to connect hardware-level telemetry to application logic

**Value Statement:**
- Enable real-time playback quality monitoring for Netflix, Cobalt, YouTube, and WPE-WebKit
- Provide data foundation for adaptive bitrate (ABR) algorithms and QoE optimization
- Support W3C compliance requirements for video metrics APIs
- Establish clean, formalized two-flow architecture (Phase 1): QUERY (snapshot), PUSH (events)
- Reduce debugging time by exposing frame counters and error metrics to applications

## What Changes

**Architecture Overview:**
Metrics flow passthrough from GStreamer SoC to Rialto clients via dedicated `RialtoMetrics` module and two IPC flows (QUERY, PUSH):

```
GStreamer Bus (media-pipeline-metric messages)
        ↓ posts
GstGenericPlayer (owns RialtoMetrics instance)
        ↓ delegates to
    RialtoMetrics (Metrics Processor)
        ├─ Handles GStreamer bus callbacks
        ├─ Caches frame counters
        └─ Forwards metric events to IPC
        ↓
IPC/Protobuf (two RPC flows)
        ↓ delivers to
MediaPipelineClient (client-side receiver)
        ↓ forwards to
RialtoMSE Sink (consumer apps)
```

**Two IPC Flows (Phase 1):**
1. **QUERY FLOW** (GetFrameCounters RPC): Client polls for frame counter snapshot → Server returns current state
2. **PUSH FLOW** (NotifyMetricEvent RPC): Server pushes frame counter snapshots asynchronously to all connected clients → Clients receive via callback

**High-Level Modifications:**

### 1. **Create RialtoMetrics Module** (New)
- **Files:** `rialto/media/server/metrics/include/IRialtoMetrics.h` (interface) / `RialtoMetrics.h` (implementation)
- **File:** `rialto/media/server/metrics/source/RialtoMetrics.cpp` (implementation)
- **Responsibilities:**
  - Listen to GStreamer bus messages (`media-pipeline-metric` custom messages)
  - Accumulate and cache frame counters (rendered, dropped, corrupted)
  - Forward metric events to all connected sessions via IPC layer (passthrough, no filtering)
  - Thread-safe access to all data structures (maps, counters)
- **Public Interface:**
  - `void onMetricMessage(GstMessage *message)` — Receive metric messages routed by the existing GStreamer dispatcher
  - `int32_t getFrameCounters(int32_t sessionId, int32_t sourceId, uint64_t *rendered, uint64_t *dropped, uint64_t *corrupted, uint64_t *timestamp)` — Retrieve counter snapshot
  - `void onMetricMessage(const MetricMessageData &data)` — Internal handler for GStreamer bus callback (static trampoline)
  - `void notifyMetricEvent(int32_t sessionId, int32_t sourceId, uint64_t rendered, uint64_t dropped, uint64_t corrupted, const std::string &errorInfo)` — Forward event to IPC layer if session subscribed
- **Internal Data Structures:**
  - Counter cache: `std::map<std::pair<int32_t, int32_t>, FrameCounters>` (sessionId+sourceId → {rendered_frames, dropped_frames, corrupted_frames, timestamp})
  - Thread synchronization: Mutex or RWLock for concurrent access

### 2. **Update GstGenericPlayer** (Modification)
- **File:** `rialto/media/server/gstplayer/source/GstGenericPlayer.cpp` / `.h`
- **Changes:**
  - Add member variable: `std::shared_ptr<IRialtoMetrics> m_metrics`
  - In constructor, instantiate RialtoMetrics: `m_metrics = std::make_shared<RialtoMetrics>()`
  - Route application messages from the existing dispatcher to `RialtoMetrics`; do not register a second bus watch
  - Add accessor method: `std::shared_ptr<IRialtoMetrics> getRialtoMetrics() const { return m_metrics; }`
  - No other changes to GstGenericPlayer; metrics handling delegated to RialtoMetrics

### 3. **Update MediaPipelineServerInternal** (Modification)
- **File:** `rialto/media/server/main/source/MediaPipelineServerInternal.cpp` / `.h`
- **Changes:**
  - Add public method: `bool getFrameCounters(int32_t sessionId, int32_t sourceId, uint64 &rendered, uint64 &dropped, uint64 &corrupted, uint64 &timestamp)`
    - Delegate to: `return m_gstGenericPlayer->getRialtoMetrics()->getFrameCounters(sessionId, sourceId, &rendered, &dropped, &corrupted, &timestamp)`
    - Return true for success, false for not found
  - All metric storage/processing now owned by RialtoMetrics (no local storage in MediaPipelineServerInternal)

### 4. **Create Protobuf Message Definitions** (New)
- **File:** `rialto/proto/mediapipelinemodule.proto`
- **Messages:**

```protobuf
enum MetricType {
  VIDEO_FRAME_DROP = 0;
  AUDIO_FRAME_DROP = 1;
  VIDEO_DECODE_ERROR = 2;
  AUDIO_DECODE_ERROR = 3;
  VIDEO_FRAME_REPEAT = 4;
  AUDIO_GAP = 5;
  BUFFER_UNDERFLOW = 6;
}

// QUERY FLOW (Phase 1)
message GetFrameCountersRequest {
  int32 session_id = 1;
  int32 source_id = 2;
}

message GetFrameCountersResponse {
  uint64 rendered_frames = 1;
  uint64 dropped_frames = 2;
  uint64 corrupted_frames = 3;  // Frames with decode errors
  uint64 timestamp = 4;  // Server timestamp (nanoseconds)
}

// PUSH FLOW (Phase 1, 1-to-1 passthrough)
message NotifyMetricEvent {
  int32 session_id = 1;
  int32 source_id = 2;
  uint64 rendered_frames = 3;
  uint64 dropped_frames = 4;
  uint64 corrupted_frames = 5;  // Frames with decode errors
  string error_info = 6;  // Error details if applicable
}
```

### 5. **Create IPC Service RPC Handlers** (New)
- **File:** `rialto/media/server/ipc/source/MediaPipelineModuleService.cpp`
- **Changes:**
  - Add RPC handler: `void MediaPipelineModuleService::getFrameCounters(GetFrameCountersRequest *request, GetFrameCountersResponse *response)`
    - Call `MediaPipelineServerInternal::getFrameCounters(request->session_id, request->source_id, rendered, dropped, corrupted, timestamp)`
    - Populate response with counter values and timestamp
  - Register async callback handler for NotifyMetricEvent push delivery from server

### 6. **Create Client-Side IPC Stubs** (New)
- **File:** `rialto/media/server/ipc/source/MediaPipelineClient.cpp` / `.h`
- **Changes:**
  - Implement `bool callGetFrameCounters(int32_t sourceId, uint64 &rendered, uint64 &dropped, uint64 &corrupted, uint64 &timestamp)` (QUERY)
    - Build GetFrameCountersRequest protobuf message
    - Send RPC call, wait for GetFrameCountersResponse
    - Copy response fields to output parameters
    - Return true on success, false on timeout/error
  - Register async callback handler for incoming NotifyMetricEvent messages (PUSH)
    - Forward to registered client callback when received

### 7. **Create Unit Tests** (New)
- **File:** `rialto/tests/unittests/media/server/metrics_passthrough_test.cpp`
- **Test Cases:**
  - Protobuf message serialization/deserialization for GetFrameCounters*, NotifyMetricEvent
  - Counter snapshot retrieval and cache consistency for multiple sources
  - GStreamer bus callback parsing (valid + invalid metric messages)
  - Thread safety: concurrent getFrameCounters calls across multiple sources
  - Frame counter monotonicity (never decrease) per source

### 8. **Create Component/Integration Tests** (New)
- **File:** `rialto/tests/componenttests/media/server/metrics_passthrough_integration_test.cpp`
- **Test Cases:**
  - End-to-end: Mock GStreamer bus inject metric message → GstGenericPlayer callback → MediaPipelineServerInternal → IPC → response
  - Multiple sessions receiving same metrics (no per-session filtering in Phase 1)
  - Counter monotonicity (never decrease) per source
  - Push event delivery: server sends NotifyMetricEvent → client callback receives frame_counters
  - Verify backward compatibility: existing notifyQos() and getStats() paths unchanged

### 9. **Update CMakeLists.txt** (Modification)
- **File:** `rialto/media/server/metrics/CMakeLists.txt` (NEW)
  - Create new build target for RialtoMetrics library
  - Link: GStreamer libraries, protobuf
- **File:** `rialto/media/server/main/CMakeLists.txt`
  - Add dependency on RialtoMetrics library
  - Link RialtoMetrics into MediaPipelineServer
- **File:** `rialto/media/server/ipc/CMakeLists.txt`
  - Add new protobuf message compilation targets for MetricType, GetFrameCounters*, NotifyMetricEvent (no SetMetricSubscription*)
  - Ensure MediaPipelineModuleService includes new RPC handlers
- **File:** `rialto/tests/unittests/CMakeLists.txt`
  - Add metrics_passthrough_test to build targets
  - Add rialto_metrics_test (new unit test for RialtoMetrics class)
- **File:** `rialto/tests/componenttests/CMakeLists.txt`
  - Add metrics_passthrough_integration_test to build targets

### 10. **Create Documentation** (New)
- **File:** `rialto/docs/MetricsDesign.md`
- **Content:**
  - Architecture diagram showing two RPC flows (QUERY, PUSH)
  - MetricType enum semantics (frame drops, decode errors, repeats, gaps, underflow)
  - Thread safety model (GStreamer bus thread → IPC async sender)
  - Example client code snippets for QUERY and PUSH flows

## Non-Goals (Phase 1)

- **Subscription Management (SetMetricSubscription)**: Deferred to Phase 2; Phase 1 has no per-client filtering (all metrics pushed to all clients)
- **Message Batching/Buffering**: Deferred to Phase 2; Phase 1 delivers 1-to-1 push messages as events occur (no server-side batching)
- **GStreamer Message Format Modification**: Will not modify `media-pipeline-metric` message structure or introduce new message types (westeros-sink and decoders emit in their current format; validation is rialto-gstreamer responsibility)
- **Metrics Processing/Filtering**: Will NOT implement filtering, accumulation, or statistical analysis on server (pure passthrough relay only)
- **Counter Reset Semantics**: Will not define or implement counter reset behavior (pass-through HAL values as-is; reset logic deferred to future phase)
- **Adaptive Bitrate Implementation**: Will not implement ABR algorithms (only provide data source for external ABR systems)
- **Historical Time-Series**: Will not create time-series database; only current snapshot + last event

## Impacted Areas

| Component | Impact Type | Location / Path | Details |
|-----------|------------|-----------------|---------|
| **RialtoMetrics Module (PRIMARY)** | New Module | `rialto/media/server/metrics/include/IRialtoMetrics.h`, `RialtoMetrics.h`, `source/RialtoMetrics.cpp` | Core metrics processor: GStreamer bus callback, counter caching, metric forwarding (Phase 1: no subscription storage) |
| **GstGenericPlayer** | Modification | `rialto/media/server/gstplayer/source/GstGenericPlayer.cpp/.h` | Add member: `std::shared_ptr<IRialtoMetrics> m_metrics`; instantiate `RialtoMetrics` in constructor; route dispatcher-delivered metric messages to RialtoMetrics |
| **MediaPipelineServerInternal** | Modification | `rialto/media/server/main/source/MediaPipelineServerInternal.cpp/.h` | Add public method: `getFrameCounters()` that delegates to RialtoMetrics; add accessor `getRialtoMetrics()` (Phase 1: no setMetricSubscription) |

| **Counter Cache** | Data Structure (owned by RialtoMetrics) | `rialto/media/server/metrics/source/RialtoMetrics.cpp` | Thread-safe map: `std::map<std::pair<int32_t, int32_t>, FrameCounters>` (sessionId+sourceId → rendered/dropped/decoded/timestamp) |
| **Protobuf Messages** | New Message Definitions | `rialto/proto/mediapipelinemodule.proto` | Add enum (MetricType 0-6), messages (GetFrameCounters*, NotifyMetricEvent with frame_counters embedded) |
| **IPC Service Handlers** | New RPC Methods | `rialto/media/server/ipc/source/MediaPipelineModuleService.cpp` | Implement two RPC handler methods: `getFrameCounters()`, async `NotifyMetricEvent` callback delivery (Phase 1) |
| **Client-Side IPC Callers** | New RPC Stubs | `rialto/media/server/ipc/source/MediaPipelineClient.cpp/.h` | Implement `callGetFrameCounters()` and async callback handler registration for NotifyMetricEvent (Phase 1) |
| **Unit Tests (RialtoMetrics)** | New Test Suite | `rialto/tests/unittests/media/server/metrics/RialtoMetricsTest.cpp` | Test GStreamer bus callback, counter caching, thread safety, metric forwarding (Phase 1: no subscription tests) |
| **Unit Tests (IPC)** | New Test Suite | `rialto/tests/unittests/media/server/metrics_passthrough_test.cpp` | Test protobuf message serialization/deserialization, IPC handler routing |
| **Component Tests** | New Test Suite | `rialto/tests/componenttests/media/server/metrics_passthrough_integration_test.cpp` | End-to-end: mock bus → RialtoMetrics → IPC → client; verify monotonicity, multiple sessions, backward compatibility |
| **Build System** | Configuration Updates | `rialto/media/server/metrics/CMakeLists.txt` (NEW), `rialto/media/server/main/CMakeLists.txt`, `rialto/media/server/ipc/CMakeLists.txt`, `rialto/tests/CMakeLists.txt` | Create RialtoMetrics build target; add dependency in MediaPipelineServer; add test targets |
| **Documentation** | New Specification | `rialto/docs/MetricsDesign.md` | Architecture showing RialtoMetrics module role, two RPC flows (QUERY, PUSH), MetricType semantics, thread safety model, example code |

## Success Criteria

**Functional Requirements (Phase 1):**
- ✅ The existing GStreamer dispatcher routes `media-pipeline-metric` messages to `RialtoMetrics` without registering another bus watch
- ✅ `RialtoMetrics::onMetricMessage()` correctly parses GST_MESSAGE_CUSTOM with structure name 'media-pipeline-metric'
- ✅ Bus callback non-blocking; GStreamer pipeline latency overhead < 1ms
- ✅ `RialtoMetrics::getFrameCounters()` returns current counter snapshot from internal cache within 1ms
- ✅ Frame counters strictly monotonic (never decrease) across all pipeline events
- ✅ GetFrameCounters RPC call (QUERY FLOW) reaches server handler and returns frame counter response (rendered/dropped/corrupted/timestamp)
- ✅ NotifyMetricEvent RPC call (PUSH FLOW) delivers 1-to-1 push messages from server to all connected clients as events occur

**Quality Requirements:**
- ✅ All protobuf messages (GetFrameCounters*, NotifyMetricEvent) serialize/deserialize correctly
- ✅ MetricType enum values 0-6 properly defined with correct names
- ✅ GetFrameCountersResponse returns: rendered_frames (uint64), dropped_frames (uint64), corrupted_frames (uint64), timestamp (uint64)
- ✅ NotifyMetricEvent includes source_id field to disambiguate multi-source sessions (e.g., audio + video)
- ✅ IPC latency: GetFrameCounters response < 10ms (excluding network RTT)
- ✅ Push notification latency: NotifyMetricEvent delivered as event occurs (no buffering in Phase 1)
- ✅ Memory overhead: counter cache < 100KB per session
- ✅ CPU overhead: metric handling < 0.5% CPU utilization

**Integration & Backward Compatibility:**
- ✅ Existing `notifyQos()` and `getStats()` paths unchanged and still functional
- ✅ All existing unit/component tests pass without modification
- ✅ No regression in playback latency (< 5% difference vs. baseline)
- ✅ End-to-end flow (GStreamer bus → server → IPC → client) latency < 100ms under normal load

**Test Coverage:**
- ✅ Unit tests verify protobuf message serialization/deserialization for all message types (GetFrameCounters*, NotifyMetricEvent)
- ✅ Unit tests verify counter snapshot retrieval and cache consistency
- ✅ Unit tests verify GStreamer message parsing (valid messages, malformed handling)
- ✅ Unit tests verify thread safety (concurrent counter queries without subscription changes)
- ✅ Component tests verify end-to-end passthrough with injected metric messages
- ✅ Component tests verify counter monotonicity over multiple frame events
- ✅ Component tests verify multiple sessions receive all metric events (no filtering in Phase 1)

**Documentation:**
- ✅ MetricsDesign.md includes ASCII architecture diagram showing two RPC flows (QUERY, PUSH)
- ✅ MetricType enum semantics clearly explained (frame drops, decode errors, repeats, gaps, underflow)
- ✅ Thread safety model documented (GStreamer bus thread → IPC async sender)
- ✅ Example client code snippets for each RPC flow (QUERY, PUSH)
- ✅ Explicit statement that server performs NO buffering, filtering, or aggregation in Phase 1

## Rollback Plan

### **If RialtoMetrics Integration Fails:**

1. **Immediate Disable (Feature Flag):**
   ```cpp
   // In GstGenericPlayer.cpp
   #define ENABLE_RIALTO_METRICS 0  // Set to 0 to disable entire metrics subsystem
   
   #if ENABLE_RIALTO_METRICS
  // Metric messages are routed from GstGenericPlayer::handleMetricMessage.
   #endif
   ```

2. **Remove RialtoMetrics Module:**
   ```bash
   # Navigate to rialto repo
   cd rialto
   
   # Remove metrics directory entirely
   git rm -r media/server/metrics/
   
   # Revert GstGenericPlayer changes
   git checkout HEAD -- media/server/gstplayer/source/GstGenericPlayer.cpp
   git checkout HEAD -- media/server/gstplayer/include/GstGenericPlayer.h
   ```

3. **Revert IPC/Protobuf:**
   ```bash
   # Revert protobuf messages (remove MetricType enum, SetMetricSubscription*, GetFrameCounters*, NotifyMetricEvent messages)
   git checkout HEAD -- proto/mediapipelinemodule.proto
   
   # Revert IPC handlers
   git checkout HEAD -- media/server/ipc/source/MediaPipelineModuleService.cpp
   git checkout HEAD -- media/server/ipc/include/MediaPipelineModuleService.h
   git checkout HEAD -- media/server/ipc/source/MediaPipelineClient.cpp
   git checkout HEAD -- media/server/ipc/include/MediaPipelineClient.h
   ```

4. **Revert MediaPipelineServerInternal:**
   ```bash
   # Remove metrics delegation methods
   git checkout HEAD -- media/server/main/source/MediaPipelineServerInternal.cpp
   git checkout HEAD -- media/server/main/include/MediaPipelineServerInternal.h
   ```

5. **Remove Test Files:**
   ```bash
   # Remove metrics test suites
   git rm tests/unittests/media/server/metrics_passthrough_test.cpp
   git rm tests/unittests/media/server/metrics/RialtoMetricsTest.cpp
   git rm tests/componenttests/media/server/metrics_passthrough_integration_test.cpp
   ```

6. **Revert CMakeLists.txt:**
   ```bash
   # Revert build system changes
   git checkout HEAD -- media/server/metrics/CMakeLists.txt
   git checkout HEAD -- media/server/main/CMakeLists.txt
   git checkout HEAD -- media/server/ipc/CMakeLists.txt
   git checkout HEAD -- tests/unittests/CMakeLists.txt
   git checkout HEAD -- tests/componenttests/CMakeLists.txt
   ```

7. **Remove Documentation:**
   ```bash
   git rm docs/MetricsDesign.md
   ```

8. **Clean Build and Verify:**
   ```bash
   # Navigate to build directory
   cd build
   
   # Clean and rebuild
   rm -rf CMakeFiles CMakeCache.txt
   cmake ..
   make -j$(nproc)
   
   # Run existing tests to verify no regression
   ctest --verbose
   ```

9. **Verification Steps:**
   ```bash
   # Confirm RialtoMetrics code removed
   [ ! -d media/server/metrics/ ] && echo "✓ RialtoMetrics directory removed"
   
   # Confirm protobuf reverted (should not contain MetricType enum)
   grep -q "enum MetricType" proto/mediapipelinemodule.proto && echo "✗ Protobuf NOT reverted" || echo "✓ Protobuf reverted"
   
   # Confirm tests removed
   [ ! -f tests/unittests/media/server/metrics_passthrough_test.cpp ] && echo "✓ Test files removed"
   
   # Confirm all existing tests pass
   ctest --verbose
   ```