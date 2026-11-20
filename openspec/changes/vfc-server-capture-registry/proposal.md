## Why

Before a capture can be attached to a pipeline or deliver frames, the server must know it exists. Proposal §3.1 "Capture objects" says creating a capture registers it with the server and returns null when the server is unreachable, lacks capture support, or the graphics library cannot be loaded; §3.6 and §12 scope registrations to the IPC connection. Nothing in Rialto does this yet, so `createGLVideoFrameCapture()` from R4 always returns null.

## What Changes

- The server provides `VideoFrameCaptureModule` with `createVideoFrameCapture` and `destroyVideoFrameCapture`.
- Each IPC connection has its own capture registry. `createVideoFrameCapture(api)` returns a new `capture_id`, unique on that connection and never reused on it; it fails with `UNSUPPORTED` when the capture HAL library is unavailable (R2) and `INVALID_ARGUMENT` for an unknown API type.
- A `capture_id` is valid only on the connection that created it; other connections, and IDs that were destroyed, are rejected.
- `destroyVideoFrameCapture` removes the registration. Disconnecting removes every registration of that connection; R10 extends this with releasing outstanding frame locks.
- The client's `createGLVideoFrameCapture()` returns a capture object when the server registers it and `libEGL.so.1` can be loaded at run time, and null otherwise. The object reports `apiType() == GL`, and destroying it sends `destroyVideoFrameCapture`.
- Until R11, the capture's `acquireCurrentFrame()` returns `{changed = false, frame = nullptr}`.
- No public header change beyond R4.

## Capabilities

### New Capabilities

- `video-frame-capture-registry`: How captures are created, identified and destroyed, how their registrations are scoped to an IPC connection, and when creation fails.

### Modified Capabilities

None.

## Impact

- **Server:** a new `VideoFrameCaptureModuleService` in `media/server/ipc`, wired into `SessionManagementServer` (`clientConnected`/`clientDisconnected`) like the other modules; a per-connection `VideoFrameCaptureRegistry` in the server main layer, reached through a new service interface in `media/server/service`.
- **Client:** a new `VideoFrameCaptureIpc` in `media/client/ipc`; a GL capture object in `media/client/main`; a small run-time loader for `libEGL.so.1` that R12 extends.
- **Tests:** server IPC and service unit tests, client IPC unit tests, component tests with the fake HAL from R2.
- **Dependencies:** R2 (HAL availability), R4 (API and proto).
- **Downstream:** R6 attaches registered captures; R9 and R10 use the registry for acquisition, late release and cleanup.
