# Proposal: MainThread Bounded Wait — RDKEMW-27333

## Implementation Status

The core implementation is complete. MainThread class, interface, and comprehensive tests (13 L1 tests) have been implemented covering all acceptance criteria. Mock updates and test base updates are complete.

The following remains:

- **Call site updates** (~95 sites across 4 ServerInternal files) – Most changes are mechanical; example patterns implemented for all four types
- **Full test suite execution** – MainThread tests pass design criteria; full servermain/serverservice/serveripc suites pending
- **TSan/Valgrind verification** – Thread sanitizer and memory validation

Current progress:

- ✅ Core MainThread fix (per-task state, bounded wait, cancellation, shutdown escape, self-call guard, exception safety)
- ✅ Interface changes (bool return type)
- ✅ Mock updates (MainThreadMock.h)
- ✅ Test base updates (4 files: MediaPipelineTestBase, MediaKeysTestBase, WebAudioPlayerTestBase, MediaKeySessionTestBase)
- ✅ Comprehensive L1 tests (13 new tests covering all ACs)
- ✅ Example call site updates (3 patterns demonstrated in MediaPipelineServerInternal.cpp)
- ⏳ Remaining call sites (~95 total, template-driven approach documented)
- ⏳ Full integration testing

## Criteria Coverage

This proposal addresses the following MainThread concurrency issues:

| Criterion | Coverage |
|-----------|----------|
| **AC1: Bounded timeout** | `cv.wait_for()` with 30s default (configurable); 2 tests verify timeout behavior within 100-300ms |
| **AC2: Shutdown escape** | Destructor drains queue, cancels pending tasks, fails all waiters; test verifies cleanup |
| **AC3: Self-call inline** | Thread ID detection, inline execution; 2 tests verify no deadlock (<10ms execution) |
| **AC4: Race-free registry** | `std::atomic<uint32_t>` for IDs; separate `m_registeredClientsMutex` for registry; 2 concurrent tests + TSan support |
| **AC5: L1 test coverage** | 13 comprehensive tests covering timeout, shutdown, self-call, exceptions, concurrency |

## Why

**Current Problems (Pre-Fix):**

1. **F1 – No timeout:** `cv.wait(lockTask, [&]{ return newTask->done; })` has no timeout. If main thread blocks, IPC threads hang forever. Client RPC times out (~3s), server IPC thread leaked per call. Over many calls, server becomes unresponsive due to thread starvation.

2. **F2 – Self-call deadlock:** Task running on main thread calls `enqueueTaskAndWait()`. Task waits for completion, but completion can only happen on main thread → guaranteed deadlock.

3. **F3 – Exception crash:** Task throws uncaught exception. Exception escapes `mainThreadLoop()` → `std::thread::~thread()` calls `std::terminate()` → process dies.

4. **F4 – Registry race:** `m_registeredClients` is plain `std::set` accessed from multiple threads without synchronization. `ControlServerInternal::~ControlServerInternal()` calls `unregisterClient()` directly (not on main thread) → data race detected by TSan.

**Consequences:**

- Blocked server process (F1)
- Cascading deadlocks (F2)  
- Process crashes on exception (F3)
- Hidden data races (F4)

**Solution:** This proposal implements per-task shared state, bounded waits with cancellation, shutdown-safe queue draining, inline self-call execution, exception safety, and thread-safe registry access.

