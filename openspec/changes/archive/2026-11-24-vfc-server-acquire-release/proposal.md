## Why

Once a pipeline has an admitted capture pool (R8), the client needs a way to take the current frame and give it back. The server must call the HAL, validate the result, hand the frame's DMA-BUF FDs to the client, and keep the SoC spec's ownership promises: release exactly once, close every server-owned FD, and never lose an invalidation notice when delivery fails (SoC spec §6.1, §7.3, §7.4; proposal §3.4, §3.6). R1 provides the IPC pieces this needs: nested FD transfer, owned reply FDs and a delivery callback.

## What Changes

- Server `acquireCurrentFrame(capture_id)`: runs on the IPC thread without waiting on media, calls the HAL, validates the frame (R3), and replies with pool ID, slot, PTS, visible region, acquisition token, frame metadata, the FDs as owned reply FDs, and the invalidation flag.
- HAL results map to wire status one to one; a capture with no committed session answers `NO_FRAME`.
- A frame that fails validation is released, its FDs closed, and the session's capture becomes fatal (`FATAL_ERROR` from then on), while releases of earlier frames still work.
- If the reply cannot be delivered, the server closes the FDs, releases the frame exactly once, and keeps the invalidation flag pending for the next successful delivery, even if the HAL's next flag is false.
- When a frame's format, modifier, backing size or colour differs from what this client was last sent, the reply also carries updated `pool_metadata` (no FDs).
- Server `releaseFrame(capture_id, pool_id, slot_index, acquisition_token)`: forwards to the HAL with the original record; a wrong slot or token returns `INVALID_ARGUMENT` and keeps the lock.
- All HAL calls of one session are serialized, including releases started inside the server.
- Delivery is not gated by application state (Q7).

## Capabilities

### New Capabilities

- `capture-frame-delivery`: How the server acquires frames from the capture HAL, validates and delivers them with their FDs to the client, handles failed delivery, and releases frame locks.

### Modified Capabilities

None.

## Impact

- **Server:** `VideoFrameCaptureModuleService` gains the acquire and release handlers; a per-session capture object in the server main layer holds acquired `CapturedFrame` records in stable storage, the pending-invalidation latch, the last-sent description and the per-session lock.
- **IPC:** uses R1's `setReplyFdsOwned` and `setDeliveryCallback`.
- **Tests:** component tests with a scripted fake HAL (R2), including injected send failure and concurrency stress.
- **Dependencies:** R1 (FD transfer, delivery callback), R3 (validation), R8 (admitted session and pool ID). Can be built against R8's interfaces with the fake HAL.
- **Downstream:** R10 (teardown, late release, disconnect), R11 (client lease).
