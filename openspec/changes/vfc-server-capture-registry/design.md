# Design

## Context

See proposal.md for the motivation. Server modules are IPC services in `media/server/ipc` (for example `MediaPipelineModuleService`) that receive `clientConnected(client)` and `clientDisconnected(client)` from `SessionManagementServer`, and call into the service layer (`media/server/service`) and main layer (`media/server/main`). The client talks to each module through an `*Ipc` class in `media/client/ipc`. R2 provides `ICaptureHalLibrary::isAvailable()`; R4 provides the proto and the public header.

## Goals / Non-Goals

**Goals:**
- A registry keyed by IPC client that later changes extend with attachment (R6), sessions (R8) and retired sessions with outstanding locks (R10), as proposal §12 sketches.
- Client capture objects with a clean null path.

**Non-Goals:**
- Attaching to pipelines (R6), capabilities (R8), frame delivery (R9, R11), EGL import (R12).

## Decisions

### D1. Registry per IPC client, owned by a capture service
`VideoFrameCaptureService` (service layer) holds `std::map<ClientId, VideoFrameCaptureRegistry>`. Each registry maps `capture_id` to a `CaptureEntry { GraphicsAPIType api; }`, extended later with the attached pipeline and sessions. IDs come from a per-registry counter starting at 1, so IDs are never reused on a connection and a foreign ID is simply not found.

*Why:* matches proposal §12 ("one registry per client IPC connection") and keeps cross-connection access impossible by construction.
*Alternative:* one global map with a client field. Rejected: every lookup would need an ownership check.

### D2. HAL availability decides support
`createVideoFrameCapture` checks `ICaptureHalLibrary::isAvailable()` (which loads the library on first use) and answers `UNSUPPORTED` when false. No session is created here; sessions belong to commit (R7, R8).

### D3. Thread model
Registry calls arrive on the IPC thread. The registry is guarded by one mutex per service; the disconnect path takes the same lock. R9 adds a per-session lock for HAL calls; the registry lock is never held during a HAL call.

### D4. Client capture object and EGL check
`GLVideoFrameCapture` (client main) holds `capture_id` and a `VideoFrameCaptureIpc`. Creation order: load `libEGL.so.1` through a small `EglLibrary` loader (dlopen once, keep the handle), then call `createVideoFrameCapture`; return null if either fails. The destructor sends `destroyVideoFrameCapture` and ignores failures (the server may be gone). R12 extends `EglLibrary` with the EGL and GL function table.

*Why:* checking EGL before registering avoids a server registration that the client would immediately destroy.

## Risks / Trade-offs

- [Destroy sent while the capture is still attached] → cannot happen from the public API because the pipeline holds a reference to an attached capture (R6); the server still treats it as an implicit detach in R6, so a misbehaving client cannot leave a dangling binding.
- [Client destructor runs after the IPC connection is gone] → the send fails and is logged at debug; disconnect cleanup already removed the registration.

## Migration Plan

None. A V1.1 client against an older server keeps getting null (R4's path).
