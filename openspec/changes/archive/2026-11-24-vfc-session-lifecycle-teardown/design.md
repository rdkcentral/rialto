# Design

## Context

See proposal.md for the motivation. R9 introduced `CaptureSessionRuntime` (session lock, slot records, `fatal`). R5's registry maps `capture_id` to a `CaptureEntry`, which proposal §12 says also holds "every retired session that still has locked slots". R2's session deleter drops a shared library handle after the virtual destructor, so the library unloads after the last session. Proposal §12 also notes the sink disconnects capture in its READY→NULL transition, so the session must stay valid until the pipeline has reached NULL.

## Goals / Non-Goals

**Goals:**
- One retirement path for all triggers.
- Deletion exactly when the SoC spec §4.6 preconditions hold, never earlier and never leaked.

**Non-Goals:**
- Server INACTIVE/application-switch policy: it destroys pipelines, which reaches the same path.
- Client-side cleanup (R11, R12).

## Decisions

### D1. Retirement moves the runtime, it does not destroy it
`retire()` sets `retired = true` under the session lock, detaches the runtime from the pipeline, and moves its `shared_ptr` from `CaptureEntry::activeSession` to `CaptureEntry::retiredSessions`. Acquire on the entry then finds no active session and answers `NO_FRAME` (R9's no-session path).

*Why:* releases still need the runtime; moving it keeps lookups by `capture_id` and `pool_id` working.

### D2. Abandon the final acquisition
A retired runtime never calls HAL `acquireCurrentFrame`. This satisfies SoC spec §4.6 ("resolve or abandon") and matches proposal §3.5 (`NO_FRAME` after stop).
*Alternative:* allow one final acquisition. Rejected: the public API already promises `NO_FRAME` after stop.

### D3. Deletion gate
The runtime tracks three conditions: `sinkAtNull`, `lockedCount == 0` and `inFlight == 0` (HAL calls plus pending R9 delivery callbacks). Whichever event makes all three true (sink reaches NULL, last release, last callback) removes the runtime from `retiredSessions` and deletes the HAL session outside the session lock. Server-owned FDs are already closed by R1/R9 at send time, so no FD condition is needed beyond asserting it.

`sinkAtNull` is set by the pipeline when playbin has reached NULL after `stop()` or destruction. For FAILURE, the server drives the pipeline to NULL as it does today, then sets it.

### D4. Disconnect
`clientDisconnected` takes the registry; for each runtime (active or retired), under its lock, call HAL `releaseFrame` for every stored record, then mark it retired. Pipelines of that client are destroyed by the existing session cleanup, which sets `sinkAtNull`; the deletion gate then deletes the sessions. The registry is removed when its last runtime is gone.

### D5. Ownership in callbacks
R9's delivery callback captures a `shared_ptr` to the runtime and increments `inFlight`, so a retirement during a send cannot delete the session under the callback.

## Risks / Trade-offs

- [Pipeline never reaches NULL (stuck teardown)] → the session leaks rather than being deleted under a live sink; log a warning after a timeout. Deleting early would violate SoC spec §4.6.
- [Vendor library unload races a late callback] → impossible by D3/D5: the library handle is owned by the session, which is deleted only when `inFlight == 0`.

## Migration Plan

None. Pipelines without a capture are unchanged.
