# Metrics Gathering System Design

## Overview

The Rialto metrics system provides CPU and memory usage monitoring for both client and server processes, with state-aware aggregation, configurable thresholds, and pluggable output.

The system is built on top of the `PrivateMetricsModule` — a dedicated IPC service channel between the Rialto client library and the Rialto server that is separate from the media pipeline control path. It is "private" in the sense that it is an internal implementation detail not exposed to application developers.

## Why "Private" Metrics

Rialto uses a client-server architecture where the client library (`libRialtoClient.so`) runs inside the application process and communicates with a separate `rialto-server` process via protobuf-over-Unix-socket IPC. The metrics system needs data from *both* processes:

- **Client process**: CPU time and memory of the application hosting the media pipeline
- **Server process**: CPU time, memory, and cgroup resource limits of the renderer

Since these are different processes, the server cannot simply read `/proc/self/...` to get client data — it must ask the client to report it. The `PrivateMetricsModule` provides this request/response channel.

## PrivateMetrics IPC Protocol

### Proto Definition (`privatemetricsmodule.proto`)

```protobuf
enum MetricsSampleReason {
    METRICS_SAMPLE_REASON_UNKNOWN = 0;
    METRICS_SAMPLE_REASON_CONNECTED = 1;
    METRICS_SAMPLE_REASON_PERIODIC = 2;
    METRICS_SAMPLE_REASON_STATE_TRANSITION = 3;
}

message ClientProcessMetrics {
    optional uint64 sample_id = 1;
    optional MetricsSampleReason reason = 2;
    optional string app_name = 3;
    optional uint32 process_id = 4;
    optional uint64 monotonic_time_ms = 5;
    optional uint64 epoch_time_ms = 6;
    optional uint64 process_cpu_time_ms = 7;
    optional uint64 process_memory_kb = 8;
}

message MetricsSampleRequestEvent {
    optional uint64 sample_id = 1;
    optional MetricsSampleReason reason = 2;
}

service PrivateMetricsModule {
    rpc notifyClientReady(NotifyClientReadyRequest) returns (NotifyClientReadyResponse);
    rpc reportClientMetrics(ReportClientMetricsRequest) returns (ReportClientMetricsResponse);
}
```

### Communication Pattern

The protocol uses a **server-initiated push** model:

```mermaid
sequenceDiagram
    participant Client as Client (ClientController)
    participant IPC as PrivateMetricsModuleService (ipc)
    participant Svc as PrivateMetricsService (service)
    participant Main as MetricsCollector (main)

    Note over Client,Main: Client connects via IPC socket
    IPC->>Client: exportService(PrivateMetricsModule)
    Client->>IPC: notifyClientReady()
    IPC->>Svc: clientReady(clientId, ipcClient)
    Svc->>Main: creates MetricsCollector(clientId)
    Main->>IPC: requestSample(clientId, CONNECTED)
    IPC->>Client: MetricsSampleRequestEvent(id=1, reason=CONNECTED)
    Client->>IPC: reportClientMetrics(clientId, metrics)
    IPC->>Svc: reportMetrics(clientId, metrics)
    Svc->>Main: processMetrics(metrics)
    Note over Main: Stores baseline (no CPU% yet)

    loop Every 15 seconds (ITimer periodic)
        Main->>IPC: requestSample(clientId, PERIODIC)
        IPC->>Client: MetricsSampleRequestEvent(id=N, reason=PERIODIC)
        Client->>IPC: reportClientMetrics(clientId, metrics)
        IPC->>Svc: reportMetrics(clientId, metrics)
        Svc->>Main: processMetrics(metrics)
        Note over Main: Compute CPU%, feed aggregators, check thresholds
    end

    Note over Main: Playback state changes
    Main->>IPC: requestSample(clientId, STATE_TRANSITION)
    IPC->>Client: MetricsSampleRequestEvent(id=N, reason=STATE_TRANSITION)
    Client->>IPC: reportClientMetrics(clientId, metrics)
    IPC->>Svc: reportMetrics(clientId, metrics)
    Svc->>Main: processMetrics(metrics)
```

Key design points:
- The **server drives timing** — the client never spontaneously reports; it only responds to requests
- The `sample_id` field correlates requests with responses and provides ordering
- The `reason` field is echoed back by the client so the server knows how to handle the response
- The `client_id` maps each client to its per-client `MetricsCollector` in `server/main`
- The service is exported per-client on connection, allowing multi-client support

### Client Side (`ClientController` + `PrivateMetricsIpc`)

When the client library initializes (via `ClientController`), it:
1. Creates a `PrivateMetricsIpc` that subscribes to `MetricsSampleRequestEvent`
2. Calls `notifyClientReady()` to signal the server it can accept sample requests
3. On each `MetricsSampleRequestEvent`, gathers:
   - `monotonic_time_ms`: `CLOCK_MONOTONIC` in milliseconds
   - `epoch_time_ms`: wall-clock time (for log correlation)
   - `process_cpu_time_ms`: `CLOCK_PROCESS_CPUTIME_ID` (total user+system CPU)
   - `process_memory_kb`: VmRSS from `/proc/self/status`
   - `app_name`: from `/proc/self/comm`
   - `process_id`: `getpid()`
4. Sends the data back via `reportClientMetrics()`

### Server-Side Architecture

The server follows Rialto's standard three-layer architecture (ipc → service → main):

```
┌──────────────────────────────────────────────────────────────┐
│  server/ipc (PrivateMetricsModuleService)                    │
│    - Receives protobuf RPC calls                             │
│    - Sends MetricsSampleRequestEvent to IPC clients          │
│    - Generates unique client IDs on notifyClientReady        │
│    - Implements IMetricsCollectorClient (callback from main) │
│    - Delegates ALL business logic to service layer           │
└────────────────────────┬─────────────────────────────────────┘
                         │
┌────────────────────────▼─────────────────────────────────────┐
│  server/service (IPrivateMetricsService)                     │
│    - Routes calls between ipc and main                       │
│    - Maps client IDs to MetricsCollector instances            │
│    - Creates/destroys MetricsCollector instances              │
└────────────────────────┬─────────────────────────────────────┘
                         │
┌────────────────────────▼─────────────────────────────────────┐
│  server/main (MetricsCollector)                              │
│    - Owns ITimer (periodic, 15s)                             │
│    - Owns MetricsAccumulator, StateMetricsAggregator         │
│    - Owns MetricsThresholdChecker                            │
│    - Owns IMetricsReporter                                   │
│    - Computes CPU%, feeds aggregators, checks thresholds     │
│    - Receives playback/application state notifications       │
│    - Samples /proc, cgroup for server-side metrics           │
└──────────────────────────────────────────────────────────────┘
```

#### IPC Layer (`server/ipc`)

`PrivateMetricsModuleService`:
- Receives `notifyClientReady` → generates unique client ID → calls service layer → returns ID in response
- Receives `reportClientMetrics` → extracts client ID and metrics → delegates to service layer
- Implements `IMetricsCollectorClient` so `MetricsCollector` can request samples back through IPC
- Maintains mapping of client IDs to IPC client connections
- **No business logic** — no CPU calculation, no aggregation, no thresholds

#### Service Layer (`server/service`)

`IPrivateMetricsService` / `PrivateMetricsService`:
- `clientReady(clientId, client)` → creates `MetricsCollector` in main layer
- `clientDisconnected(clientId)` → destroys `MetricsCollector`
- `reportMetrics(clientId, metrics)` → finds collector, calls `processMetrics()`
- `notifyPlaybackStateChanged(sessionId, oldState, newState)` → routes to all collectors
- `notifyApplicationStateChanged(oldState, newState)` → routes to all collectors

#### Main Layer (`server/main`)

`MetricsCollector` (one per connected client):
- Created by `PrivateMetricsService` when client is ready
- Constructor creates `ITimer` (periodic, 15s) — timer callback requests sample via `IMetricsCollectorClient`
- `processMetrics()` — CPU calculation, aggregator feeding, threshold checking, reporting
- `notifyPlaybackStateChanged()` — finalize/begin state aggregators
- `notifyApplicationStateChanged()` — finalize/begin global aggregator
- Destructor cancels timer (ITimer destructor handles this automatically)

### Timer Model

Uses the existing `ITimer` framework (`common/interface/ITimer.h`):

```cpp
// In MetricsCollector constructor:
m_timer = m_timerFactory->createTimer(
    std::chrono::seconds{15},
    [this]() { onTimerFired(); },
    TimerType::PERIODIC
);

// Timer callback:
void MetricsCollector::onTimerFired()
{
    m_client->requestMetricsSample(m_clientId, m_nextSampleId++, MetricsSampleReason::PERIODIC);
}
```

- `ITimer` manages its own thread internally
- `cancel()` or destructor stops the timer cleanly
- No need for `std::thread`, `std::condition_variable`, or `std::atomic<bool> m_isRunning`

### Lifecycle

1. **Server start**: `SessionManagementServer` creates `PrivateMetricsModuleService` (ipc) + `PrivateMetricsService` (service)
2. **Client connects**: `clientConnected()` → registers IPC client, exports service
3. **Client ready**: `notifyClientReady()` → generates client ID → service creates `MetricsCollector` → timer starts → requests initial sample
4. **Periodic collection**: `ITimer` fires every 15s → `MetricsCollector::onTimerFired()` → requests sample via `IMetricsCollectorClient`
5. **Client responds**: `reportClientMetrics()` → IPC extracts data → service routes → `MetricsCollector::processMetrics()`
6. **State changes**: `MediaPipelineClient` notifies playback state → routed through service → `MetricsCollector::notifyPlaybackStateChanged()`
7. **Client disconnects**: `clientDisconnected()` → service destroys `MetricsCollector` (timer auto-cancelled in destructor)
8. **Server stop**: service destructor destroys all collectors

## System Architecture

```mermaid
graph TD
    subgraph Client Process
        APP[Application] --> RCL[libRialtoClient.so]
        RCL --> CC[ClientController]
        CC -->|VmRSS, CPU time| PI[PrivateMetricsIpc]
        PI -->|protobuf RPC| IPC((Unix Socket IPC))
    end

    subgraph Server Process
        IPC --> PMS[PrivateMetricsModuleService<br/>server/ipc]
        PMS -->|IMetricsCollectorClient| MC

        PMS --> PSVC[PrivateMetricsService<br/>server/service]
        PSVC --> MC[MetricsCollector<br/>server/main]

        MC -->|ITimer periodic| TIMER[ITimer 15s]
        MC -->|samples /proc, cgroup| OS[OS Interfaces]
        MC --> AGG[StateMetricsAggregator]
        MC --> THR[MetricsThresholdChecker]
        MC --> REP[IMetricsReporter]

        MPC[MediaPipelineClient] -->|notifyPlaybackStateChanged| PMS
        SMS[SessionServerManager] -->|notifyApplicationStateChanged| PMS

        REP --> LOG[LogMetricsReporter]
        REP --> COMP[CompositeMetricsReporter]
        COMP --> LOG
        COMP --> REMOTE[Future: RemoteTelemetryReporter]
    end
```

## Components

### Data Collection

| Component | Role |
|-----------|------|
| `ClientController` | Reads client VmRSS from `/proc/self/status` and CPU time via `clock_gettime(CLOCK_PROCESS_CPUTIME_ID)` |
| `PrivateMetricsModuleService` | Reads server CPU time via `times()`, server VmRSS from `/proc/self/status`, cgroup memory from `/sys/fs/cgroup/.../memory.current` |
| `privatemetricsmodule.proto` | Defines `ClientProcessMetrics` message with `process_memory_kb` field and `MetricsSampleReason` enum |

### Sampling

- **Periodic**: Every 15 seconds, the server requests a sample from connected clients
- **On connection**: Baseline sample taken immediately (no CPU % computed)
- **State transitions**: Immediate sample requested for clean state boundaries (reported but not fed into aggregators due to unreliable CPU data from tiny time deltas)

### CPU Percentage Calculation

```
CPU% = (cpu_time_delta_ms / wall_time_delta_ms) × 100
```

- Computed independently for client, server, and combined (client+server)
- Multi-core systems can exceed 100% (acceptable)
- **Minimum elapsed time**: 100ms threshold; returns 0% if wall-clock delta is too small to prevent division artifacts

### Cgroup Memory

Resolved dynamically from `/proc/self/cgroup`:
1. Parse cgroup v2 line (`0::<path>`)
2. Read `/sys/fs/cgroup/<path>/memory.current` and `memory.max`
3. Fall back to cgroup v1 paths if v2 unavailable
4. Value of "max" (unlimited) → reported as 0

### State-Aware Aggregation

#### MetricsAccumulator (Welford's Algorithm)

Header-only implementation providing O(1) memory online computation of:
- Count, min, max, mean, standard deviation

#### StateMetricsAggregator

Wraps 6 `MetricsAccumulator` instances (one per metric dimension):
- Client CPU %, Server CPU %, Combined CPU %
- Client memory KB, Server memory KB, Cgroup memory KB

Lifecycle:
1. `begin(stateName, startTimeMs)` — reset and start accumulating
2. `addSample(MetricsSample)` — feed each periodic sample
3. `finalize(endTimeMs)` → `StateMetricsReport` with duration and all stats

#### Per-Session Tracking

Each media pipeline session has a `SessionMetricsState` containing:
- Current `PlaybackState`
- A `StateMetricsAggregator`

On playback state change:
1. Finalize the old state's aggregator → emit report
2. Begin a new accumulation period for the new state
3. On terminal states (STOPPED, END_OF_STREAM, FAILURE) — remove session

#### Global Tracking

A single `StateMetricsAggregator` tracks the RUNNING application state period.
On transition from RUNNING → INACTIVE, the report is emitted.

### Threshold Checking

`MetricsThresholdChecker` evaluates each PERIODIC sample against configured limits:

| Metric | Warning | Critical |
|--------|---------|----------|
| Client CPU % | 80 | 95 |
| Server CPU % | 80 | 95 |
| Combined CPU % | 150 | 190 |
| Client memory KB | 512,000 | 768,000 |
| Server memory KB | 512,000 | 768,000 |
| Cgroup memory % | 80 | 95 |

**Debounce**: An alert fires once when exceeded. It can fire again only after the metric drops below the threshold for 2 consecutive samples.

### Output (IMetricsReporter)

Abstract interface with three report types:

| Method | When |
|--------|------|
| `reportPeriodicSample` | Every sampling interval |
| `reportStateTransition` | On playback/application state change |
| `reportThresholdExceeded` | When a metric breaches a threshold |

Implementations:
- **LogMetricsReporter** — writes to Rialto server log (default)
- **CompositeMetricsReporter** — fans out to multiple reporters (for adding remote telemetry)

## Data Flow

```
┌─ Every 15s ─────────────────────────────────────────────────────────────┐
│                                                                          │
│  Timer fires → requestMetricsSample(PERIODIC) to all ready clients       │
│  Client responds with ClientProcessMetrics (CPU time, memory, etc.)      │
│  Server takes its own sample (CPU, memory, cgroup)                       │
│  logMetrics():                                                           │
│    1. Compute CPU percentages from deltas                                │
│    2. Report via IMetricsReporter::reportPeriodicSample                  │
│    3. Feed sample into all active session aggregators                    │
│    4. Feed sample into global aggregator (if RUNNING)                    │
│    5. Check thresholds                                                   │
│                                                                          │
└──────────────────────────────────────────────────────────────────────────┘

┌─ On State Change ───────────────────────────────────────────────────────┐
│                                                                          │
│  MediaPipelineClient::notifyPlaybackState(newState)                      │
│    → notifyPlaybackStateChanged(sessionId, oldState, newState)           │
│    → Finalize old state aggregator                                       │
│    → IMetricsReporter::reportStateTransition (aggregated stats)          │
│    → Begin new state aggregator                                          │
│    → Request STATE_TRANSITION sample (for log visibility only)           │
│                                                                          │
└──────────────────────────────────────────────────────────────────────────┘
```

## Example Output

### Periodic Sample
```
Metrics sample=5, reason=PERIODIC, app='python3', client_pid=11708,
  client_cpu=0.47%, server_cpu=23.13%, combined_cpu=23.60%,
  client_cpu_ms=440, server_cpu_ms=4250,
  client_mem_kb=59648, server_mem_kb=216684, cgroup_mem_kb=209016/0
```

### State Transition Report
```
Metrics state report [session=0] state='PLAYING', duration_ms=30607, samples=2,
  client_cpu={min=0.47, max=10.11, mean=5.29, stddev=6.81}%,
  server_cpu={min=23.13, max=26.40, mean=24.77, stddev=2.31}%,
  combined_cpu={min=23.60, max=36.52, mean=30.06, stddev=9.13}%,
  client_mem_kb={min=59520, max=59648, mean=59584},
  server_mem_kb={min=214704, max=216684, mean=215694},
  cgroup_mem_kb={min=206332, max=209016, mean=207674}
```

### Threshold Alert
```
Metrics threshold WARNING: server_cpu=88.24 exceeds 80.00
```

## Extension Points

1. **Remote telemetry**: Implement `IMetricsReporter` and add to `CompositeMetricsReporter`
2. **Custom thresholds**: Pass a different `MetricsThresholdConfig` to the constructor
3. **JSON config loading**: Add a loader that reads `MetricsThresholdConfig` from a file
4. **Per-session threshold tuning**: Different limits for different pipeline types
5. **QoS metrics**: Separate data path for dropped frames / buffer underruns

## File Inventory

### Client Side

| File | Purpose |
|------|---------|
| `media/client/ipc/interface/IPrivateMetricsIpc.h` | Client-side metrics IPC interface |
| `media/client/ipc/include/PrivateMetricsIpc.h` | Client-side IPC implementation header |
| `media/client/ipc/source/PrivateMetricsIpc.cpp` | Subscribes to sample requests, gathers and sends metrics |
| `media/client/main/include/ClientController.h` | Client initialization, owns PrivateMetricsIpc |
| `media/client/main/source/ClientController.cpp` | Reads VmRSS, reports metrics on request |

### Server Side

| File | Purpose |
|------|---------|
| `media/server/ipc/include/IPrivateMetricsModuleService.h` | Thin IPC service interface |
| `media/server/ipc/include/PrivateMetricsModuleService.h` | IPC service: RPC handlers + event sending only |
| `media/server/ipc/source/PrivateMetricsModuleService.cpp` | IPC service implementation |
| `media/server/service/include/IPrivateMetricsService.h` | Service-layer interface (routing) |
| `media/server/service/source/PrivateMetricsService.h` | Service implementation header |
| `media/server/service/source/PrivateMetricsService.cpp` | Service: maps clientId to MetricsCollector |
| `media/server/main/interface/IMetricsCollector.h` | MetricsCollector interface |
| `media/server/main/interface/IMetricsCollectorClient.h` | Callback: main→ipc for sending events |
| `media/server/main/include/MetricsCollector.h` | Business logic header |
| `media/server/main/source/MetricsCollector.cpp` | Business logic: CPU calc, aggregation, thresholds |
| `media/server/ipc/source/MediaPipelineClient.cpp` | Playback state hook |
| `media/server/ipc/source/MediaPipelineModuleService.cpp` | Passes metrics service to pipeline clients |
| `media/server/ipc/source/SessionManagementServer.cpp` | Application state hook, owns metrics service |
| `media/server/service/source/SessionServerManager.cpp` | Triggers app state notifications |

### Metrics Framework

| File | Purpose |
|------|---------|
| `media/server/main/include/MetricsAccumulator.h` | Welford's online mean/variance |
| `media/server/main/include/StateMetricsAggregator.h` | Per-state multi-metric accumulation |
| `media/server/main/include/IMetricsReporter.h` | Reporter interface + report structs |
| `media/server/main/include/LogMetricsReporter.h` | Log-based reporter |
| `media/server/main/include/CompositeMetricsReporter.h` | Multi-reporter fanout |
| `media/server/main/include/MetricsThresholdChecker.h` | Threshold config + checker |
| `media/server/main/source/LogMetricsReporter.cpp` | Reporter implementation |
| `media/server/main/source/CompositeMetricsReporter.cpp` | Fanout implementation |
| `media/server/main/source/MetricsThresholdChecker.cpp` | Threshold checking logic |

### Protocol

| File | Purpose |
|------|---------|
| `proto/privatemetricsmodule.proto` | IPC message and service definitions |
