# Design: MainThread Bounded Wait — RDKEMW-27333

## Context

The Rialto server's `MainThread` class is a process-wide singleton worker thread that serializes access to session objects (`MediaPipelineServerInternal`, `MediaKeysServerInternal`, `WebAudioPlayerServerInternal`, `ControlServerInternal`). IPC worker threads enqueue tasks to `MainThread` and block waiting for completion using `cv.wait()` without timeout.

Currently:

- `IMainThread::enqueueTaskAndWait()` and `enqueuePriorityTaskAndWait()` return `void` and offer no visibility into success/failure
- Callers cannot distinguish between successful completion and timeout/exception
- Main thread wedges or dies → IPC threads hang indefinitely
- Self-calls (task calls `enqueueTaskAndWait()` from main thread) deadlock
- Unregistered client operations can leave waiters blocked
- Exceptions in tasks crash the main thread (unhandled → `std::terminate()`)
- Registry access is not thread-safe (F4 race in destructor unregister)

## Goals / Non-Goals

**Goals:**

- Add per-task shared state (mutex, cv, status, exception) to decouple caller lifetime from completion signaling
- Implement bounded wait with configurable timeout (default 30s) via `cv.wait_for()`
- Cancel only tasks not yet started (`Pending` status); keep waiting for `Running` tasks to avoid stack corruption
- Fail all pending tasks on shutdown; dtor drains queue and releases all waiters
- Detect self-calls (thread ID check) and execute inline to avoid deadlock
- Wrap task execution in try/catch; store exceptions for caller inspection; main loop survives exceptions
- Protect `m_registeredClients` with separate mutex; make `unregisterClient()` thread-safe
- Change blocking enqueue methods to return `bool` (true = success, false = timeout/shutdown/exception/unregistered)
- Ensure all ~95 call sites handle the new return type

**Non-Goals:**

- No COM-RPC or alternate IPC mechanisms
- Do not use different timeout values per call (constructor parameter only)
- Do not introduce new client-server protocol messages (changes are internal)
- Do not require recompilation of client library (internal-only change)

## Decisions

### Task Status State Machine

```
Pending ─────────┐
  │              ├─→ Done (task() returned successfully)
  │              ├─→ Failed (task() threw exception)
  ├─→ Running    │
  │              ├─→ Cancelled (timeout while Pending, not yet started)
  │
  └─→ (terminal states)
```

**Key decision:** Never cancel a `Running` task. If task is already executing when timeout occurs, keep waiting. This prevents stack corruption (task still writes to caller's stack frame). Main thread wedge is then handled by a higher-level watchdog (heartbeat ping timeout).

### Per-Task Shared State

Embed completion state directly in `shared_ptr<TaskInfo>`:

```cpp
struct TaskInfo {
    uint32_t clientId;
    Task task;
    std::mutex mutex;
    std::condition_variable cv;
    TaskStatus status{TaskStatus::Pending};
    std::exception_ptr exception;
};
```

Both caller and queue hold `shared_ptr<TaskInfo>`. No extra allocation vs RDKEMW-15488 (which used `thread_local TaskState`).

**Rationale:** Eliminates lifetime coupling – if caller times out and exits, the `shared_ptr` remains valid in the queue, and main thread can still safely mark task Done/Failed without touching a dead stack frame.

### Bounded Wait with wait_for

```cpp
bool completed = cv.wait_for(lock, m_taskTimeout, [&]{ return terminal_status; });
```

Default `m_taskTimeout = 30s` (configurable via constructor for testing). Client RPC timeout is ~3s, so server gives caller ~27s extra; caller usually gives up first. Server timeout mainly recovers the IPC thread.

### Inline Self-Calls

```cpp
if (std::this_thread::get_id() == m_threadId) {
    // On main thread – execute inline
    try { task(); return true; } catch (...) { return false; }
}
```

Store `m_threadId` as `std::atomic<std::thread::id>` set at loop start. Avoids deadlock and maintains ordering (inline tasks run immediately, not queued).

### Thread-Safe Registry

Separate `std::mutex m_registeredClientsMutex` from `m_taskQueueMutex`. Protects `m_registeredClients` insertion/deletion. `m_nextClientId` remains `std::atomic<uint32_t>` (already atomic at audit baseline).

Makes `unregisterClient()` callable from any thread, fixing `ControlServerInternal::~ControlServerInternal()` race.

### Bool Return Type

Change signatures:

```cpp
// Before
virtual void enqueueTaskAndWait(uint32_t clientId, const Task &task) = 0;

// After
virtual bool enqueueTaskAndWait(uint32_t clientId, const Task &task) = 0;
// Returns true if task completed successfully, false on timeout/shutdown/exception/unregistered
```

Propagate bool result through all ~95 call sites. For bool-returning methods, return false on enqueue failure. For void methods, return early. For destructors, call `unregisterClient()` on failure to prevent ID leaks.

### Exception Handling

Wrap task execution:

```cpp
try {
    { std::unique_lock lock(taskInfo->mutex); taskInfo->status = Running; }
    task();
    { std::unique_lock lock(taskInfo->mutex); taskInfo->status = Done; }
} catch (...) {
    std::unique_lock lock(taskInfo->mutex);
    taskInfo->status = Failed;
    taskInfo->exception = std::current_exception();
}
```

Caller can inspect `exception` via `std::rethrow_exception()` if desired, but primary result is the bool return (false).

### Shutdown Escape

Destructor sets `bool m_shutdown` flag, drains queue:

```cpp
{
    std::unique_lock lock(m_taskQueueMutex);
    m_shutdown = true;
    // Extract all pending tasks
    std::vector<std::shared_ptr<TaskInfo>> pending = std::move(m_taskQueue);
    lock.unlock();
    // Fail all tasks outside lock
    for (auto& info : pending) {
        std::unique_lock taskLock(info->mutex);
        if (info->status == Pending) info->status = Cancelled;
        taskLock.unlock();
        info->cv.notify_all();
    }
}
m_taskQueueCv.notify_one(); // Wake main loop
m_thread.join();
```

If dtor runs on main thread (last owner released inside a task), `detach()` instead of `join()` to avoid `std::system_error` EDEADLK.

