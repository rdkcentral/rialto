# Tasks

Unit tests run with `./build_ut.py -s client` using EGL/GL mocks. "Spec" means `specs/gl-frame-import/spec.md`; "D#" means a decision in `design.md`.

## 1. EGL/GL access (D1, D2)

- [x] 1.1 Extend `EglLibrary` with the EGL and GL function table resolved through `eglGetProcAddress`, and add a mockable table for tests. Verify `readelf -d libRialtoClient.so` still shows no `libEGL` or `libGLESv2` dependency.
- [x] 1.2 Implement the once-per-capture extension and format/modifier check. Verify: missing `EGL_EXT_image_dma_buf_import` fails imports, logs once and closes FDs; a listed pair passes.

## 2. Import (D3, D5)

- [x] 2.1 Implement `buildEglAttributes`. Verify against SoC examples B (shared object, repeated FD) and C (two objects), modifier LO/HI on every plane, and colour-hint mapping for BT601/BT709/BT2020, full/narrow and both siting values.
- [x] 2.2 Implement `GlFrameImporter::importFrame`: create the EGL image, generate and bind a `GL_TEXTURE_EXTERNAL_OES` texture, call `glEGLImageTargetTexture2DOES`; report FDs as borrowed; unwind on failure. Verify: success returns one texture ID; image failure and texture failure each leave no image or texture (mock counters) and all FDs closed.
- [ ] 2.3 Make GL frames report `GL_TEXTURE_EXTERNAL_OES`, one texture ID, `RGBA` and one resource. Verify with a unit test.

## 3. Cleanup (D4)

- [ ] 3.1 Implement `retire` (queue only) and `drain` (texture then image). Verify with a GL call counter: dropping a frame on a thread with no context makes zero EGL/GL calls; the queued import is deleted at the next acquire.
- [ ] 3.2 Implement teardown: direct deletion when `eglGetCurrentContext()` matches; otherwise the temporary shared context on a Rialto thread (surfaceless, else 1x1 pbuffer), with an info log. Verify both paths with mocks, including that the temporary context is destroyed and the thread joined.
- [ ] 3.3 Document in `IVideoFrameCapture.h` that no current context is needed to release frames or destroy the capture, and that the display and share group must outlive frames and the capture. Verify the Doxygen build is clean.

## 4. Integration

- [ ] 4.1 Add a client component test with the fake HAL and mocked EGL: acquire, invalidate, release on another thread, acquire again, destroy from a non-context thread; the open-FD count returns to its starting value. Verify with `./build_ct.py`.
- [ ] 4.2 Optional on-target test: import a udmabuf-backed NV12 frame into a real EGL context and sample one pixel. Run manually on a reference board and record the result in the PR.

## Workflow follow-up

- Run `openspec validate vfc-client-gl-import --strict` before review.
- Confirm second-shared-context support on each target GPU driver (roadmap A9 open item) before enabling capture on that platform.
