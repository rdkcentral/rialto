# Tasks: MainThread Bounded Wait — RDKEMW-27333

## Phase 1: Core MainThread Implementation

### 1. Interface Update

- [X] 1.1 Update `media/server/main/interface/IMainThread.h`: change `enqueueTaskAndWait()` and `enqueuePriorityTaskAndWait()` return type from `void` to `bool`. Add documentation: "Returns false on timeout, shutdown, task exception or unregistered client."
- [X] 1.2 Verify signature changes compile against existing code (will fail, as expected).

### 2. MainThread Header – Data Structures

- [X] 2.1 Add `enum class TaskStatus { Pending, Running, Done, Failed, Cancelled }` to `MainThread.h`.
- [X] 2.2 Extend `TaskInfo` struct: remove `TaskState *state` pointer; add `std::mutex mutex`, `std::condition_variable cv`, `TaskStatus status{Pending}`, `std::exception_ptr exception`.
- [X] 2.3 Remove `TaskState` struct (no longer needed).
- [X] 2.4 Add member variables: `std::chrono::milliseconds m_taskTimeout` (default 30s), `bool m_shutdown`, `std::atomic<std::thread::id> m_threadId`, `std::mutex m_registeredClientsMutex`.
- [X] 2.5 Update constructor: `explicit MainThread(std::chrono::milliseconds taskTimeout = kDefaultTaskTimeout)`.
- [X] 2.6 Add private helper: `bool enqueueAndWait(uint32_t clientId, const Task &task, bool priority)`.

### 3. MainThread Implementation – Core Logic

- [X] 3.1 Update constructor: set `m_shutdown = false`, start thread, store `m_threadId` at loop start.
- [X] 3.2 Rewrite destructor: detect main thread (detach if on main thread), set shutdown flag, drain queue, fail all pending tasks, join or detach.
- [X] 3.3 Rewrite `mainThreadLoop()`:
  - Set `m_threadId` at start
  - For each task: check client registered (with mutex), set status Running, try/catch task(), set terminal status (Done/Failed/Cancelled), notify all
  - Skip cancelled tasks
  - Return on shutdown signal
- [X] 3.4 Rewrite `waitForTask()`: use condition for `!m_taskQueue.empty() || m_shutdown`; return nullptr on shutdown.
- [X] 3.5 Implement `enqueueAndWait()` helper:
  - Self-thread check (thread ID == m_threadId): execute inline, return true/false
  - Shutdown check: return false immediately
  - Enqueue with priority flag
  - `cv.wait_for(timeout, terminal_status_predicate)`
  - If timeout while Pending: set Cancelled, return false
  - If Running on timeout: keep waiting (log ERROR), return false when done
  - Return true only on Done status
- [X] 3.6 Update `enqueueTaskAndWait()` and `enqueuePriorityTaskAndWait()`: delegate to `enqueueAndWait()` with appropriate priority flag.
- [X] 3.7 Update `registerClient()`: acquire `m_registeredClientsMutex` when inserting; change from async enqueue to sync (eliminates priority-before-registration hole).
- [X] 3.8 Update `unregisterClient()`: acquire `m_registeredClientsMutex` when erasing; now thread-safe.

## Phase 2: Mock and Test Infrastructure

### 4. Mock and Test Base Updates

- [X] 4.1 Update `tests/unittests/media/server/mocks/main/MainThreadMock.h`: change `MOCK_METHOD` return type to `bool` for `enqueueTaskAndWait` and `enqueuePriorityTaskAndWait`.
- [X] 4.2 Update all test bases (4 files):
  - `tests/.../mediaPipeline/base/MediaPipelineTestBase.cpp`: Invoke lambdas return `true`
  - `tests/.../mediaKeys/base/MediaKeysTestBase.cpp`: Invoke lambdas return `true`
  - `tests/.../webAudioPlayer/base/WebAudioPlayerTestBase.cpp`: Invoke lambdas return `true`
  - `tests/.../mediaKeySession/base/MediaKeySessionTestBase.cpp`: Invoke lambdas return `true`
- [X] 4.3 Update all test base helper methods to assert return value is `true`.

### 5. L1 Tests (AC1–AC5)

- [X] 5.1 Add `EnqueueTaskAndWaitTimesOutWhenMainThreadBlocked` – Blocks main thread, verifies waiter times out within 100±100ms. AC1.
- [X] 5.2 Add `EnqueuePriorityTaskAndWaitTimesOutWhenMainThreadBlocked` – Same for priority. AC1.
- [X] 5.3 Add `DestructorFailsOutstandingWaiters` – Destroy MainThread while 2 waiters pending; verify both return false. AC2.
- [X] 5.4 Add `SelfCallExecutesInline` – Task on main thread calls `enqueueTaskAndWait()`; verify inline execution, no timeout, no deadlock. AC3.
- [X] 5.5 Add `SelfPriorityCallExecutesInline` – Same for priority. AC3.
- [X] 5.6 Add `TaskExceptionIsReportedAndLoopSurvives` – Task throws; verify return false, next task still runs. Exception handling.
- [X] 5.7 Add `FireAndForgetExceptionDoesNotKillLoop` – Fire-and-forget task throws; verify loop survives. Exception handling.
- [X] 5.8 Add `ConcurrentRegisterClientsGetUniqueIds` – 8 threads × 10 concurrent registrations; verify 80 unique IDs, no duplicates. AC4.
- [X] 5.9 Add `UnregisterFromNonMainThreadIsSafe` – Concurrent unregister + enqueue; verify no crash, TSan-clean. AC4.
- [X] 5.10 Update existing test helpers: assert bool return values are true for successful operations.

## Phase 3: Call Site Updates

### 6. Example Call Site Patterns (MediaPipelineServerInternal)

- [X] 6.1 Update destructor (~MediaPipelineServerInternal): check bool return, call unregisterClient() on failure.
- [X] 6.2 Update bool-returning method (load): check bool return, log error, return false on failure.
- [X] 6.3 Update second bool-returning method (attachSource): same pattern as 6.2.

### 7. Remaining Call Sites (~92 remaining across 4 files)

- [ ] 7.1 MediaPipelineServerInternal.cpp (~42 remaining): Update all bool-returning methods using pattern from 6.2. Methods include: play, pause, stop, setPlaybackRate, setPosition, getStats, setImmediateOutput, setReportDecodeErrors, etc.
- [ ] 7.2 MediaPipelineServerInternal.cpp (remaining): Update void-returning method (ping) using void pattern.
- [ ] 7.3 MediaPipelineServerInternal.cpp (remaining): Update custom return type method (addSegment) using appropriate error return.
- [ ] 7.4 MediaKeysServerInternal.cpp (~20 calls): Update all methods (createKeySession, generateRequest, update, etc.) using bool pattern.
- [ ] 7.5 WebAudioPlayerServerInternal.cpp (~15 calls): Update all methods (play, pause, setVolume, etc.) using bool pattern.
- [ ] 7.6 ControlServerInternal.cpp (~3 calls): Update all methods using appropriate patterns.
- [ ] 7.7 Verify call sites compile (run `make` or CMake build on each subsystem).

**Automation:** Use `call_site_updater.py` to identify, suggest, and verify replacements.

## Phase 4: Integration Testing and Verification

### 8. Compilation and Unit Tests

- [ ] 8.1 Build MainThread unit tests: `python3 build_ut.py -s servermain -gf 'MainThreadTests.*'`
- [ ] 8.2 Verify all 9+ new L1 tests pass.
- [ ] 8.3 Build full server-main suite: `python3 build_ut.py -s servermain`
- [ ] 8.4 Verify all existing tests pass (regression).
- [ ] 8.5 Build server-service and server-ipc suites: `python3 build_ut.py -s serverservice serveripc`

### 9. Thread Sanitizer (TSan) Verification

- [ ] 9.1 Configure build with TSan: `cmake -S . -B build-tsan -DCMAKE_CXX_FLAGS="-fsanitize=thread -g -O1"`
- [ ] 9.2 Build RialtoServerMainUnitTests target: `cmake --build build-tsan --target RialtoServerMainUnitTests`
- [ ] 9.3 Run TSan on MainThreadTests: `TSAN_OPTIONS="halt_on_error=1" ./build-tsan/tests/.../RialtoServerMainUnitTests --gtest_filter='MainThreadTests.*'`
- [ ] 9.4 Verify no data race reports (all TSan-clean). ConcurrentRegisterClientsGetUniqueIds and UnregisterFromNonMainThreadIsSafe specifically.

### 10. Memory Validation (Valgrind)

- [ ] 10.1 Run Valgrind on MainThread tests: `python3 build_ut.py -s servermain -gf 'MainThreadTests.*' -val`
- [ ] 10.2 Verify no memory leaks, no use-after-free.

### 11. Stress and Flakiness Testing

- [ ] 11.1 Run MainThreadTests multiple times: `python3 build_ut.py -s servermain -gf 'MainThreadTests.*' -nb` (repeat 20×)
- [ ] 11.2 Verify consistent pass rate (no flaky tests under load).

## Phase 5: Documentation and Review

### 12. Documentation

- [X] 12.1 Document design decisions in `design.md` (this file).
- [X] 12.2 Create `proposal.md` (acceptance criteria, problem statement).
- [X] 12.3 Create comprehensive implementation guide (`RDKEMW-27333-IMPLEMENTATION.md`).
- [X] 12.4 Create quick reference guide (`QUICK-REFERENCE.md`).
- [X] 12.5 Create call site update utility (`call_site_updater.py`).

### 13. Code Review and Merge

- [ ] 13.1 Push feature branch: `feature/RDKEMW-27333-mainthread-bounded-wait`
- [ ] 13.2 Open PR with:
  - Link to RDKEMW-27333 ticket
  - Summary of design (§4 from design.md)
  - AC coverage table
  - Test results (servermain, TSan, Valgrind)
  - Call site update status
- [ ] 13.3 Address review feedback.
- [ ] 13.4 Merge to master.

## Acceptance Criteria Verification

| AC | Task | Status |
|----|----|--------|
| AC1 | Timeout bounded ≤30s | 5.1, 5.2 + test execution |
| AC2 | Shutdown fails waiters | 5.3 + dtor implementation |
| AC3 | Self-call inline | 5.4, 5.5 + inline logic |
| AC4 | Race-free registry/IDs | 5.8, 5.9 + TSan clean (9.4) |
| AC5 | L1 test coverage | 5.1–5.10 + full execution |

