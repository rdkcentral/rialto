# Tasks

"Spec" means `specs/video-frame-capture-api/spec.md`; "D#" means a decision in `design.md`.

## 1. Public headers (D1, D2)

- [x] 1.1 Add `media/public/include/IVideoFrameCapture.h` from the interim header, keeping only the GL and common types listed in the spec (no `getCapabilities`, no `*Caps`, no Vulkan types). Install it. Verify a test translation unit that includes it compiles without `<EGL/egl.h>`.
- [x] 1.2 Add non-pure `attachVideoFrameCapture()` and `detachVideoFrameCapture()` returning `false`, and the `setVideoWindow()` capture note, to `media/public/include/IMediaPipeline.h`. Verify existing mocks (`tests/unittests/media/client/mocks` and any other `IMediaPipeline` mocks) compile unchanged.
- [x] 1.3 Implement `IVideoFrameCaptureFactory::createFactory()` in `media/client/main` with `createGLVideoFrameCapture()` returning null, and forward the new pipeline methods through `MediaPipelineProxy`. Verify `readelf -d libRialtoClient.so` shows no `libEGL` dependency.

## 2. Protos (D3, D4)

- [x] 2.1 Add `proto/videoframecapturemodule.proto` from the interim file without the capabilities messages and RPC, and add it to `proto/CMakeLists.txt`. Verify it compiles and grep shows `field_is_fd` only on `VideoFrameCaptureExportedDmaBuf.fd`.
- [ ] 2.2 Add `AttachVideoFrameCaptureRequest/Response`, `DetachVideoFrameCaptureRequest/Response` and their RPCs to `proto/mediapipelinemodule.proto`. Verify the generated code builds and existing message tags are unchanged (diff of the generated descriptors for existing messages is empty).
- [ ] 2.3 Implement server `MediaPipelineModuleService` handlers for the two new RPCs that fail through `SetFailed`. Verify with a server IPC unit test (`./build_ut.py -s serveripc`).

## 3. Compatibility tests

- [ ] 3.1 Add client unit tests: the factory returns null; attach/detach on a pipeline return `false`. Verify with `./build_ut.py -s client`.
- [ ] 3.2 Add a component test that pairs a V1.1 client with a server that has no capture module: create returns null, attach returns `false`, and normal playback in the same test still works. Verify with `./build_ct.py`.
- [ ] 3.3 Run the full unit and component suites and confirm no regressions for shipping-only clients.

## Workflow follow-up

- Run `openspec validate vfc-api-and-proto-surface --strict` before review.
- Confirm with the spec authors that the proto outside the capabilities messages is frozen (A17) before merge.
