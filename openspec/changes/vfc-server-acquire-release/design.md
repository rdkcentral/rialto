# Design

## Context

See proposal.md for the motivation. Inputs from earlier changes:
- R1: FD fields at any depth, `IController::setReplyFdsOwned(true)` and `setDeliveryCallback(cb)` invoked once after the send attempt with no IPC lock held.
- R2: `CaptureSessionGuard` around every HAL call; R3: `validateFrame` and `LockedRanges`.
- R5: registry per connection; R8: the admitted session, its `pool_id` and pool description (R9 is coded against these interfaces and tested with the fake HAL until R8 lands).

Proposal §12 requires acquisition to run on the IPC thread and never enqueue on the GStreamer worker thread.

## Goals / Non-Goals

**Goals:**
- One per-session object that owns every HAL interaction after admission.
- Exactly-once release on every path.

**Non-Goals:**
- Teardown, retired sessions, final acquisition and disconnect cleanup (R10).
- Client import and leasing (R11).

## Decisions

### D1. `CaptureSessionRuntime` owns frames and the lock
Per committed session, a `CaptureSessionRuntime` holds:
- the HAL session (through the guard) and a `std::mutex` for HAL calls;
- `std::array<std::unique_ptr<CapturedFrame>, N>` indexed by slot, because records are non-movable;
- `pendingFlush` (bool), `lastSentDescription`, `fatal` (bool), and the R3 `LockedRanges`.

Each acquisition uses a fresh empty record; on `OK` it is stored at its slot.

### D2. Acquire flow
Under the session lock: if `fatal`, answer `FATAL_ERROR`; call `acquireCurrentFrame`; on non-`OK`, map status and return. On `OK`: `pendingFlush |= frame.dmaBufObjectsFlushed`; validate; on failure release (same lock), close FDs, set `fatal`, answer `FATAL_ERROR`. Otherwise fill the reply, set `dma_buf_objects_flushed = pendingFlush`, add `pool_metadata` if the description changed, put FDs in `exported_dma_bufs`, call `setReplyFdsOwned(true)` and set a delivery callback, store the record, then release the lock and call `done->Run()`.

### D3. Delivery callback resolves the frame
The callback (no IPC lock held, per R1) takes the session lock and:
- on delivered: clears `pendingFlush`, records `lastSentDescription`, adds the frame's ranges to `LockedRanges`, sets every record FD to `-1` (R1 already closed them);
- on not delivered: calls HAL `releaseFrame` on the stored record, clears the slot; `pendingFlush` stays set.

*Why:* only the callback knows the outcome, and invalidation must be consumed only by a successful delivery (SoC spec §7.3).
*Alternative:* clear `pendingFlush` before sending and restore it on failure. Rejected: a concurrent acquisition between the two would miss it.

### D4. Ordering
The IPC server processes one client's calls in order and the session lock covers each acquisition up to `done->Run()`, so replies leave in acquisition order.

### D5. Release
Under the session lock: look up the slot; if empty or the token differs, answer `INVALID_ARGUMENT`; otherwise call HAL `releaseFrame`, remove ranges, clear the slot. A non-`OK` HAL result is returned and the record kept.

### D6. Status mapping
HAL `CAPABILITIES_NOT_AVAILABLE_YET` and `INVALID_ARGUMENT` after admission indicate a Rialto or vendor bug: log and answer `FATAL_ERROR` and `INVALID_ARGUMENT` respectively; everything else maps by name.

## Risks / Trade-offs

- [Client blocks the IPC thread with slow reads] → acquisition never waits on media; socket writes use the existing non-blocking send path.
- [Callback runs on another thread after the session is retired] → R10 keeps the runtime alive until outstanding callbacks finish (shared ownership captured in the callback).
- [Pool metadata comparison misses a change] → compares the full description, including colour, as the SoC spec defines a frame description.

## Migration Plan

None. Only captures attached to committed pipelines reach this code.
