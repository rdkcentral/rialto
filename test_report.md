# Rialto Master Branch - Test Coverage & Testability Analysis

**Date:** September 2026 | **Scope:** `rialto` (master) `tests/` folder

---

## 1. Executive Summary

Rialto has a solid two-tier test infrastructure:

- **L1 (Unit Tests)** — `tests/unittests/` — GTest+GMock, per-class isolation with comprehensive mocks
- **L2 (Component Tests)** — `tests/componenttests/` — End-to-end server and client component tests with stubs

**No L3 integration tests, no contract testing framework, and no E2E tests exist.**

| Area | Current State | Verdict |
|------|--------------|---------|
| L1 Unit Tests | Good coverage, but gaps exist | **CAN BE IMPROVED** |
| L2 Component Tests | Good scenario coverage | **CAN BE IMPROVED** |
| Contract Testing | **Does not exist** | **SHOULD BE ADDED** |
| CI Integration | `COVERAGE_ENABLED` flag exists, no CI YAML found in repo | **SHOULD BE ENHANCED** |

---

## 2. Current Test Architecture

### 2.1 Test Structure

```
tests/
  unittests/           # L1 - Unit tests (GTest + GMock)
    media/
      client/          # Client-side unit tests
      common/          # Shared media types, frame writers, schema version
      server/
        gstplayer/     # GstGenericPlayer, tasks, WebAudioPlayer, GstSrc, decryptor
        ipc/           # IPC module services (control, mediaKeys, mediaPipeline, etc.)
        main/          # MediaPipelineServerInternal, MediaKeySession, MainThread, etc.
        service/       # CdmService, PlaybackService, SessionServerManager
        mocks/         # GMock mocks for all interfaces
    ipc/               # IPC transport layer tests
    logging/           # Logging framework tests
    wrappers/          # GstWrapper, GlibWrapper tests
    serverManager/     # Server manager tests
  componenttests/      # L2 - Component tests
    server/            # Server-side component tests with GStreamer stub
      fixtures/        # RialtoServerComponentTest base fixture
      stubs/           # ClientStub, GstreamerStub, ServerManagerStub
      tests/           # Test scenarios
    client/            # Client-side component tests
      stubs/           # Server stubs for client testing
      tests/           # Test scenarios (mse, eme, control, webaudio, metrics)
```

### 2.2 Build Configuration

- Unit tests: `cmake -DCMAKE_BUILD_FLAG=UnitTests`
- Component tests: `cmake -DCMAKE_BUILD_FLAG=ComponentTests`
- Coverage: `cmake -DCOVERAGE_ENABLED=ON` (gcov/lcov)
- Proto contract: Client and server share **identical** `.proto` files (verified: no drift)

---

## 3. L1 Unit Test Analysis — Coverage Gaps

### 3.1 GstPlayer Tasks — Good, But Missing Scenarios

The task tests in `unittests/media/server/gstplayer/genericPlayer/tasksTests/` cover 44 task files. Each production task in `source/tasks/generic/` has a corresponding test file.

**GAPS IDENTIFIED:**

| # | Gap | Source File | Missing Test Scenario |
|---|-----|------------|----------------------|
| 1 | **No subtitle flush test** | `Flush.cpp` handles AUDIO, VIDEO, UNKNOWN but tests only cover Audio/Video — no SUBTITLE flush | `FlushTest.cpp` |
| 2 | **No async flush test** | `Flush.cpp` line 101-104: `m_isAsync` triggers `flushOnPrerollController->setFlushing()` — never tested with `isAsync=true` | `FlushTest.cpp` |
| 3 | **No resetTime=true test for stream position reset** | `Flush.cpp` line 116-119: `m_resetTime` resets `streamPosition` to -1 — not explicitly verified | `FlushTest.cpp` |
| 4 | **EOS re-set after subtitle flush not tested** | The known Subtitle EOS bug (VIPA video freeze) — Flush erases `endOfStreamInfo[SUBTITLE]` but test never verifies subtitle EOS re-application | `FlushTest.cpp`, `EosTest.cpp` |
| 5 | **No destructor leak tests** | `SetupElement`, `SetupSource`, `HandleBusMessage` destructors leak GStreamer refs (known findings) — no unit test verifies cleanup | `SetupElementTest.cpp`, etc. |
| 6 | **WorkerThread task drain on shutdown** | `WorkerThread.cpp` — tasks holding GstElement refs are destroyed without execute() on shutdown — no test for this | `WorkerThreadTest.cpp` |
| 7 | **SynchroniseSubtitleClock GstStructure leak path** | When `gstEventNewCustom` fails, GstStructure is leaked — no negative test exists | No test file |

### 3.2 CdmService — Missing Cleanup Verification

| # | Gap | Details |
|---|-----|---------|
| 8 | **switchToInactive does not clear m_sessionInfo** | `CdmService::switchToInactive()` only clears `m_mediaKeys` but leaves `m_sessionInfo` and `m_mediaKeysClients` maps populated. No test verifies these are cleaned up. |
| 9 | **No test for client disconnect without destroyMediaKeys** | If IPC client disconnects without calling `destroyMediaKeys()`, session maps leak. No test covers this path. |
| 10 | **releaseKeySession deferred path undertested** | `decrementSessionIdUsageCounter` can trigger deferred release — the "usage count reaches 0" path has minimal test coverage. |

### 3.3 MediaPipelineServerInternal — Missing Edge Cases

| # | Gap | Details |
|---|-----|---------|
| 11 | **No test for addSegment when NO_SPACE after partial fill** | Tests cover NO_SPACE for single segment, but not for the case where the buffer is partially filled and the next segment triggers NO_SPACE. |
| 12 | **notifyNeedMediaData concurrent access** | No test for race condition between `notifyNeedMediaData` and `haveData` on worker thread. |

### 3.4 GstGenericPlayer — Missing Lifecycle Tests

| # | Gap | Details |
|---|-----|---------|
| 13 | **No test for pipeline teardown with queued buffers** | `streamInfo.buffers` may contain GstBuffers when pipeline is torn down — no test verifies they are all unref'd. |
| 14 | **No test for double-stop** | Calling `stop()` twice in succession is not tested. |
| 15 | **No test for play after EOS without reset** | Attempting to `play()` after EOS without a `setPosition()` reset is not covered. |

### 3.5 SchemaVersion — Well Tested (No Gaps)

`SchemaVersionTest.cpp` covers: initialization, current version, equality, compatibility (major/minor/patch), string conversion. **No gaps.**

---

## 4. L2 Component Test Analysis

### 4.1 Server Component Tests — Current Coverage

27 mediaPipeline test files, 12 mediaKeys test files, 2 control tests, 1 recovery test, 3 webAudio test files, 1 mediaPipelineCapabilities, 1 mediaKeysCapabilities, 3 sessionServer tests.

**WELL COVERED:**
- Playback lifecycle (create/load/play/pause/stop/destroy)
- Encrypted playback with DRM sessions
- Dual video playback
- Audio source switching
- Flush, SetPosition, SetSourcePosition
- Volume, Mute, PlaybackRate
- QoS updates, position updates, underflow notifications
- Non-fatal player errors, first frame notification
- Healthcheck, state changes, recovery
- WebAudio play/pause/stop/failures

### 4.2 Server Component Test Gaps

| # | Missing Scenario | Impact |
|---|-----------------|--------|
| 1 | **Subtitle playback end-to-end** | No component test creates a subtitle source and verifies subtitle EOS handling. This is the exact scenario that caused the VIPA video freeze bug. |
| 2 | **Period transition with flush+reattach** | No test simulates: play period 1 -> flush all sources -> reattach -> play period 2 -> verify EOS fires for all sources. |
| 3 | **Multiple channel changes (rapid retune)** | No test creates session, plays, destroys, then creates a new session. Memory/resource cleanup between sessions is untested at L2. |
| 4 | **Client disconnect during playback** | No test simulates IPC client disconnecting mid-playback to verify server-side cleanup. |
| 5 | **Concurrent dual DRM sessions** | `DualVideoPlaybackTest` exists but doesn't test two independent DRM key systems simultaneously. |
| 6 | **Buffer overflow / backpressure** | No test pushes segments faster than pipeline consumes to verify EnoughData/flow-control behavior end-to-end. |
| 7 | **Error recovery mid-playback** | `RecoveryTest.cpp` tests healthcheck-based recovery but not recovery from GStreamer pipeline errors (e.g., decoder failure). |

### 4.3 Client Component Tests — Good Coverage

26 MSE test files, 13 EME test files, control/webaudio/metrics tests present. Client-side coverage is strong for the IPC-to-server path. Same gaps as server side for subtitle and period transition scenarios.

---

## 5. Contract Testing — DOES NOT EXIST (Recommended)

### 5.1 Current State

Rialto uses protobuf-based IPC between client (`librialto`) and server (`RialtoServer`). The proto files are currently **duplicated** in three locations:

- `proto/` (canonical source)
- `media/client/ipc/proto/` (client copy)
- `media/server/ipc/proto/` (server copy)

Currently identical (verified via `fc.exe`), but there is **no automated mechanism** to detect proto drift or breaking changes.

The `SchemaVersion` mechanism in `controlmodule.proto` exists (major/minor/patch with compatibility check), but there is **no test that validates the full contract** between a real client message and real server handler.

### 5.2 Recommended Contract Testing Framework

**Option A: Proto-based Contract Tests (Recommended)**

Add a new test target `ContractTests` that:

1. **Proto Schema Validation** — Verify client and server proto files are byte-identical at build time:
```cmake
add_test(NAME ProtoContractCheck
  COMMAND ${CMAKE_COMMAND} -E compare_files
    ${CMAKE_SOURCE_DIR}/proto/mediapipelinemodule.proto
    ${CMAKE_SOURCE_DIR}/media/client/ipc/proto/mediapipelinemodule.proto)
```

2. **Wire Format Compatibility** — Serialize a message with client proto, deserialize with server proto, verify all fields round-trip correctly. This catches field renumbering, type changes, and removed fields.

3. **Schema Version Gate** — Test that RegisterClient rejects incompatible schema versions and accepts compatible ones (partially exists in `ControlModuleServiceTests`).

4. **Backward Compatibility** — Store golden proto binary snapshots per release tag. On each new build, verify new server can still deserialize old client messages.

**Option B: Pact-style Consumer-Driven Contract Tests**

For cross-repo validation (rialto-gstreamer <-> rialto server), consider a Pact-like framework:
- Consumer (rialto-gstreamer) defines expected IPC interactions
- Provider (rialto server) verifies it can satisfy those interactions
- Contract artifacts stored in a shared location and validated in CI

### 5.3 Implementation Plan for Contract Tests

```
tests/
  contracttests/
    CMakeLists.txt
    proto_identity/          # Verify proto files are identical across copies
      ProtoIdentityTest.cpp
    wire_compat/             # Serialize with client proto, deserialize with server proto
      MediaPipelineWireTest.cpp
      MediaKeysWireTest.cpp
      ControlModuleWireTest.cpp
    schema_version/          # Version negotiation tests
      SchemaCompatTest.cpp
    golden_snapshots/        # Binary proto snapshots per release
      v0.15.2/
        CreateSessionRequest.bin
        AttachSourceRequest.bin
        ...
```

Add to root `CMakeLists.txt`:
```cmake
if( CMAKE_BUILD_FLAG STREQUAL "ContractTests" )
    include( cmake/build_google_test.cmake )
    add_subdirectory( tests/common EXCLUDE_FROM_ALL )
    add_subdirectory( tests/contracttests EXCLUDE_FROM_ALL )
endif()
```

---

## 6. Recommended L1 Test Improvements — Specific New Tests

### 6.1 FlushTest.cpp — Add These Tests

```cpp
// Test subtitle flush
TEST_F(FlushTest, ShouldFlushSubtitle)
{
    setContextStreamInfo(firebolt::rialto::MediaSourceType::SUBTITLE);
    shouldFlushSubtitle();
    shouldFlushSubtitleSrcSuccess();
    setPipelinePlaying();
    triggerFlush(firebolt::rialto::MediaSourceType::SUBTITLE);
    checkSubtitleFlushed();
}

// Test async flush sets flushing state
TEST_F(FlushTest, ShouldSetFlushingOnAsyncFlush)
{
    shouldFlushAudio();
    shouldFlushAudioSrcSuccess();
    setPipelinePlaying();
    triggerAsyncFlush(firebolt::rialto::MediaSourceType::AUDIO);
    checkFlushOnPrerollControllerFlushingSet(firebolt::rialto::MediaSourceType::AUDIO);
    checkAudioFlushed();
}

// Test resetTime resets stream position
TEST_F(FlushTest, ShouldResetStreamPositionWhenResetTimeTrue)
{
    shouldFlushAudio();
    shouldFlushAudioSrcSuccess();
    setContextStreamPosition(1000);
    setPipelinePlaying();
    triggerFlushWithResetTime(firebolt::rialto::MediaSourceType::AUDIO, true);
    checkStreamPositionReset();
    checkAudioFlushed();
}

// Test subtitle flush clears EOS (regression for VIPA freeze)
TEST_F(FlushTest, ShouldClearSubtitleEosOnFlush)
{
    setContextStreamInfo(firebolt::rialto::MediaSourceType::SUBTITLE);
    setContextEndOfStream(firebolt::rialto::MediaSourceType::SUBTITLE);
    shouldFlushSubtitle();
    shouldFlushSubtitleSrcSuccess();
    setPipelinePlaying();
    triggerFlush(firebolt::rialto::MediaSourceType::SUBTITLE);
    checkEndOfStreamCleared(firebolt::rialto::MediaSourceType::SUBTITLE);
}
```

### 6.2 Destructor Leak Regression Tests

```cpp
// SetupElementTest.cpp — verify gstObjectUnref on destruction
TEST_F(SetupElementTest, ShouldUnrefElementOnDestruction)
{
    // Construct SetupElement with a mock GstElement
    // Expect gstObjectUnref called when task is destroyed without execute()
    EXPECT_CALL(*m_gstWrapper, gstObjectUnref(&m_element));
    // Let task go out of scope
}

// HandleBusMessageTest.cpp — verify gstMessageUnref on destruction
TEST_F(HandleBusMessageTest, ShouldUnrefMessageOnDestruction)
{
    EXPECT_CALL(*m_gstWrapper, gstMessageUnref(&m_message));
    // Let task go out of scope without calling execute()
}
```

### 6.3 CdmService Cleanup Test

```cpp
TEST_F(CdmServiceTests, shouldClearSessionInfoOnSwitchToInactive)
{
    triggerSwitchToActiveSuccess();
    mediaKeysFactoryWillCreateMediaKeys();
    createMediaKeysShouldSucceed();
    mediaKeysWillCreateKeySessionWithStatus(MediaKeyErrorStatus::OK);
    createKeySessionShouldSucceed();
    triggerSwitchToInactive();
    // Verify m_sessionInfo and m_mediaKeysClients are empty
    // (requires either friend access or observable side-effect test)
}
```

---

## 7. Recommended L2 Component Test Improvements

### 7.1 New Component Test: Subtitle Period Transition

```
SubtitlePeriodTransitionTest:
  1. Create pipeline with audio + video + subtitle
  2. Play period 1 (push segments, receive EOS for all 3 sources)
  3. Flush all 3 sources (period transition)
  4. Reattach sources
  5. Push segments for audio + video only (subtitle has no content)
  6. Send EOS for audio + video
  7. VERIFY: Pipeline EOS fires (not stuck due to missing subtitle EOS)
```

### 7.2 New Component Test: Rapid Channel Change

```
RapidChannelChangeTest:
  1. Create session 1, load, play, push segments
  2. Stop and destroy session 1
  3. Immediately create session 2, load, play
  4. VERIFY: Session 2 plays successfully with no leaked resources
  5. Repeat 3 times to catch accumulation
```

### 7.3 New Component Test: Client Disconnect Cleanup

```
ClientDisconnectTest:
  1. Create session, load, start playback
  2. Disconnect IPC client (simulate crash)
  3. VERIFY: Server cleans up pipeline, DRM sessions, shared memory
  4. New client can connect and create a new session
```

### 7.4 New Component Test: Buffer Backpressure

```
BufferBackpressureTest:
  1. Create session, attach audio+video, play
  2. Push segments rapidly without consuming (stub GStreamer doesn't pull)
  3. VERIFY: EnoughData signal fires, client stops receiving NeedMediaData
  4. VERIFY: After consuming, NeedData resumes
```

---

## 8. CI/Release Gate Recommendations

### 8.1 Tag Release Validation Pipeline

For every tag release, the CI should run:

```
1. Unit Tests (L1)           — All must pass, coverage >= threshold
2. Component Tests (L2)      — All must pass
3. Contract Tests (new)       — Proto identity + wire compat + golden snapshots
4. Coverage Report            — gcov/lcov, fail if coverage drops below baseline
5. Static Analysis            — cppcheck or clang-tidy for leak patterns
6. Valgrind/ASan run          — At least on component tests to catch memory leaks
```

### 8.2 Per-PR Validation

```
1. Unit Tests (L1)           — Must pass
2. Component Tests (L2)      — Must pass
3. Proto Identity Check      — Fail if client/server protos diverge
4. Coverage delta             — Warn if new code has < 80% line coverage
```

---

## 9. Summary of All Recommendations

| Priority | Recommendation | Type | Effort |
|----------|---------------|------|--------|
| **HIGH** | Add subtitle flush + EOS regression tests (L1) | L1 gap fix | 1 day |
| **HIGH** | Add destructor leak regression tests for SetupElement/SetupSource/HandleBusMessage | L1 gap fix | 0.5 day |
| **HIGH** | Add contract test framework with proto identity checks | New framework | 2-3 days |
| **HIGH** | Add subtitle period transition component test (L2) | L2 gap fix | 1 day |
| **MEDIUM** | Add CdmService switchToInactive cleanup verification | L1 gap fix | 0.5 day |
| **MEDIUM** | Add rapid channel change component test | L2 gap fix | 1 day |
| **MEDIUM** | Add client disconnect cleanup component test | L2 gap fix | 1 day |
| **MEDIUM** | Add wire format backward compatibility tests | Contract test | 2 days |
| **MEDIUM** | Add Valgrind/ASan CI gate for component tests | CI enhancement | 1 day |
| **LOW** | Add buffer backpressure component test | L2 gap fix | 1 day |
| **LOW** | Add golden proto snapshot mechanism per release | Contract test | 1 day |
| **LOW** | Add double-stop and play-after-EOS unit tests | L1 gap fix | 0.5 day |

**Total estimated effort: ~12-14 engineering days**

---

*Report generated from code inspection of rialto master branch, September 2026.*
