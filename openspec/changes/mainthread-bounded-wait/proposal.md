# Proposal: MainThread Bounded Wait — RDKEMW-27333

## Status

Core worker behavior and focused tests are implemented. Remaining work includes propagating `enqueueTaskAndWait()` failures through all relevant server call sites, running the broader unit-test suites, and TSan/Valgrind verification.

## Review decisions incorporated

- Keep `std::lock_guard<std::mutex> lock(m_registeredClientsMutex);` around registered-client set access.
- Remove `enqueuePriorityTaskAndWait()` from the interface and implementation. It has no production callers; tests that depended on priority ordering are replaced with FIFO checks.
- Catch task exceptions with `catch (...)`, log an error, mark the task failed, notify its waiter, and do not rethrow.
- Store the worker thread id atomically and execute `enqueueTaskAndWait()` inline when called from that worker.

## Acceptance-criteria coverage

| Criterion | Coverage |
|-----------|----------|
| **AC1: Bounded wait** | `cv.wait_for()` with a 30-second default, configurable for tests; a queued task that times out is cancelled and skipped |
| **AC2: Shutdown escape** | Destructor drains queued tasks, cancels pending waiters, and wakes them |
| **AC3: Self-call** | Atomic worker-thread id check executes nested synchronous calls inline |
| **AC4: Race-free registry** | `m_nextClientId` is atomic; `m_registeredClients` access uses `m_registeredClientsMutex` |
| **AC5: L1 coverage** | Tests cover timeout/cancellation, shutdown, self-call, exception handling, and concurrent client registration |

## Problem

`MainThread` serializes operations by running queued tasks on a worker thread. Callers using `enqueueTaskAndWait()` block until their task finishes. Without bounded waiting, a blocked worker can leave request threads waiting indefinitely. A synchronous call made from the worker deadlocks because the worker is the only thread that can execute the queued task. An uncaught task exception can terminate the worker process, and unprotected access to the client registry can race.

## Solution

Use per-task shared completion state, bounded waiting, cancellation of tasks that have not started, shutdown-aware queue draining, inline execution for nested worker-thread calls, logged exception handling, and a mutex for registered-client set access. The API exposes only `enqueueTaskAndWait()`; there is no priority blocking API.

If a task has already started when its timeout expires, its caller continues waiting for it to finish. Existing task lambdas may reference caller-owned stack variables, so returning while such a lambda is executing would create use-after-return behavior. This remaining limitation must be considered when verifying the ticket's bounded-wait requirement.
