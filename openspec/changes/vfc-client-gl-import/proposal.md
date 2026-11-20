## Why

GL applications, starting with the Netflix PGIO integration (proposal §3.8), receive captured frames as `GL_TEXTURE_EXTERNAL_OES` textures. The SoC spec (§6.3, §6.4) defines exactly how DMA-BUF FDs and plane records become an EGL image and an external texture, and roadmap decision A9 (option D) defines on which thread GL objects are deleted, so that releasing a frame or destroying a capture never needs the application's context. R11 provides the graphics-independent lease and cache; this change supplies the GL importer.

## What Changes

- Load `libEGL.so.1` at run time when a GL capture is created (extending R5's loader) and resolve the EGL and GL entry points from it; verify the required extensions on first acquisition with the configured context current.
- Import each new frame as one EGL image built from the capture records (DRM fourcc, per-plane FD, offset, pitch and modifier, and the colour hints) and bind it to one `GL_TEXTURE_EXTERNAL_OES` texture. `textureTarget()` returns `GL_TEXTURE_EXTERNAL_OES` and `textureIds()` one ID.
- Close every FD after EGL image creation, on success and failure; on any failure, destroy what was created and leave nothing behind.
- GL cleanup per A9 option D: releasing a frame makes no GL or EGL call and only queues retired imports; `acquireCurrentFrame()` first deletes queued imports, then handles invalidation and eviction directly; final teardown deletes everything directly if the creation context is current on that thread, otherwise on a short-lived Rialto context sharing the application's objects, logged at info.
- The header states that the application never needs a current context to release a frame or destroy the capture, and that the display and share group must outlive every frame and the capture.
- `RGBA_SEPARATE_ALPHA` is never produced (A8 proposal part 1).

## Capabilities

### New Capabilities

- `gl-frame-import`: How captured frames become GL external textures, how FDs are handled during EGL import, and on which thread and when GL and EGL objects are deleted.

### Modified Capabilities

None.

## Impact

- **Client:** a GL importer implementing R11's `IFrameImporter`; an EGL/GL function table on R5's `EglLibrary`; a deletion queue and a teardown helper that can create a shared context; header documentation in `IVideoFrameCapture.h`.
- **Tests:** unit tests with EGL/GL wrapper mocks and a GL call counter; an optional on-target test importing a udmabuf.
- **Dependencies:** R11. No new link-time dependency: EGL and GL are loaded at run time.
- **Open:** the teardown fallback needs every target GPU driver to support a second context sharing the application's objects; where one does not, that platform logs an error and requires context-current teardown (roadmap A9 note).
