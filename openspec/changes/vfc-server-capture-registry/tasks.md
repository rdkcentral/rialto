# Tasks

"Spec" means `specs/video-frame-capture-registry/spec.md`; "D#" means a decision in `design.md`.

## 1. Server registry (D1–D3)

- [x] 1.1 Add `VideoFrameCaptureRegistry` (per connection: counter from 1, `capture_id` → `CaptureEntry`) and `IVideoFrameCaptureService`/`VideoFrameCaptureService` holding one registry per client, with a mock. Verify with unit tests in `./build_ut.py -s serverservice`: unique non-zero IDs, no reuse after destroy, unknown and destroyed IDs not found, removing a client empties only its registry.
- [x] 1.2 Make creation consult `ICaptureHalLibrary::isAvailable()` and return `UNSUPPORTED` when false, `INVALID_ARGUMENT` for `GRAPHICS_API_TYPE_UNKNOWN`. Verify with unit tests using the R2 loader mock.

## 2. Server IPC module

- [ ] 2.1 Add `VideoFrameCaptureModuleService` in `media/server/ipc` implementing `createVideoFrameCapture` and `destroyVideoFrameCapture`, with `clientConnected`/`clientDisconnected`, and wire it into `SessionManagementServer` and `IpcFactory`. Verify with `./build_ut.py -s serveripc` tests for both RPCs, a foreign ID, and disconnect cleanup.

## 3. Client (D4)

- [ ] 3.1 Add `VideoFrameCaptureIpc` in `media/client/ipc` for create and destroy. Verify with `./build_ut.py -s clientipc`: success maps to an ID; `UNSUPPORTED`, a failed call and an unreachable server map to failure.
- [ ] 3.2 Add the `EglLibrary` run-time loader (dlopen `libEGL.so.1` once) and `GLVideoFrameCapture` holding the ID; make `createGLVideoFrameCapture()` load EGL, then register, and return null on either failure; the destructor sends destroy. `acquireCurrentFrame()` returns `{false, nullptr}`. Verify with `./build_ut.py -s client` using mocked IPC and loader.

## 4. Component tests

- [ ] 4.1 Add server component tests with the R2 fake HAL: create and destroy; two captures get different IDs; a foreign ID is rejected; disconnect with two captures empties the registry; HAL absent gives `UNSUPPORTED`. Verify with `./build_ct.py`.
- [ ] 4.2 Add a client component test: creation returns a capture whose `apiType()` is `GL`; dropping it sends destroy; with the server's capture module disabled, creation returns null. Verify with `./build_ct.py`.

## Workflow follow-up

- Run `openspec validate vfc-server-capture-registry --strict` before review.
