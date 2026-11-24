## Why

A capture HAL session must outlive the pipeline that created it: the application may still hold frames after `stop()`, after a playback failure or after the pipeline is destroyed, and those frames must stay valid until released. The SoC spec (§4.4, §4.6) and proposal (§3.5 "Session lifetime", §3.6, §12) define when a session is retired, what it still accepts, and the exact preconditions for deleting it and unloading the vendor library. Without this, teardown either leaks HAL sessions and DMA-BUFs or deletes a session that the sink or a late release still uses.

## What Changes

- On `stop()`, FAILURE, detach, pipeline destruction, capture destruction or client disconnect, the server retires the pipeline's capture session.
- A retired session takes no new acquisitions: the capture answers `NO_FRAME` until it is attached to another pipeline that commits, which creates a new session. The final acquisition opportunity is abandoned, as proposal §3.5 requires `NO_FRAME` after stop.
- A retired session still accepts matching releases, including after the pipeline is destroyed and after the session became capture-fatal.
- The session is deleted only after the sink has reached NULL (capture disconnected, native writes stopped), every frame lock is released, every server-owned FD is closed, and no HAL call or delivery callback is in progress. The vendor library stays loaded until then (R2).
- On client disconnect, the server releases every remaining frame lock of that client's captures itself, then deletes the sessions and the registry.
- A retired session is never attached to another pipeline.

## Capabilities

### New Capabilities

- `capture-session-lifecycle`: When a capture HAL session is retired, what a retired session still does, and the conditions and order for deleting it and unloading the vendor library.

### Modified Capabilities

None.

## Impact

- **Server:** the R5 registry keeps retired `CaptureSessionRuntime`s with outstanding locks; pipeline stop, failure and destruction paths in `MediaPipelineServerInternal` and `GstGenericPlayer` retire the session and signal when the sink has reached NULL; the disconnect path releases locks.
- **Tests:** component tests for every trigger, late release, disconnect with held locks, and library unload order.
- **Dependencies:** R9 (session runtime, release), R2 (library lifetime), R6 (detach and destruction triggers), R7 (sink teardown order) for the on-target sequence.
- **Downstream:** completes the server side of capture; R11's client lease relies on late release succeeding.
