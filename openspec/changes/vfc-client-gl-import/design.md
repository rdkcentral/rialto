# Design

## Context

See proposal.md for the motivation. R11 calls `IFrameImporter::importFrame`, `retire` and `drain`; R5 has an `EglLibrary` that dlopens `libEGL.so.1`. The SoC spec §6.3 defines the attribute list and FD rules, §6.4 the texture path and cleanup order (texture first, then image). The decision for A9 (option D) is recorded in the roadmap: only four triggers delete GL objects (invalidation and eviction inside acquire; retired imports freed by a release; final teardown), and the last two can happen on any thread.

## Goals / Non-Goals

**Goals:**
- Exact attribute construction from records, testable without a GPU.
- No GL call outside acquisition and teardown; no leaked DMA-BUF after teardown.

**Non-Goals:**
- Vulkan (R13). Colour conversion in Rialto: sampling relies on EGL hints and `samplerExternalOES`.

## Decisions

### D1. Function table loaded from EGL
`EglLibrary` resolves `eglGetProcAddress` and, through it, `eglCreateImage(KHR)`, `eglDestroyImage(KHR)`, `eglQueryDmaBufFormatsEXT`, `eglQueryDmaBufModifiersEXT`, `eglGetCurrentContext`, `eglCreateContext`, `eglMakeCurrent`, `eglDestroyContext`, and the GL entry points (`glGenTextures`, `glBindTexture`, `glTexParameteri`, `glEGLImageTargetTexture2DOES`, `glDeleteTextures`). Tests replace the table with mocks.

*Why:* keeps `libRialtoClient.so` free of EGL/GL link dependencies (proposal "Packaging").

### D2. Extension check once per capture
On the first `importFrame`, with the application context current, check `EGL_EXT_image_dma_buf_import`, `..._modifiers`, `GL_OES_EGL_image_external`, and that the format/modifier pair is listed by `eglQueryDmaBufModifiersEXT`. Cache the result; a failure makes every later import fail fast.

### D3. Attribute builder as a pure function
`buildEglAttributes(frame, fdsByObjectId)` returns the attribute vector. Planes sharing an object repeat its FD; modifier LO/HI on every plane; colour hints mapped from `CaptureColorDescription`. Unit-tested against SoC examples B and C.

### D4. Deletion queue and teardown (A9 option D)
- `retire(handle)` appends to a mutex-protected queue; no GL call.
- `drain()` (called by R11 at the start of acquire) deletes queued entries: `glDeleteTextures` then `eglDestroyImage`.
- Teardown: when the shared importer state is destroyed (last of capture and frames), compare `eglGetCurrentContext()` with the creation context. If equal, delete everything there. Otherwise start a short-lived thread that creates a context sharing the creation context (same config, from `eglQueryContext(EGL_CONFIG_ID)`), makes it current with `EGL_NO_SURFACE` if `EGL_KHR_surfaceless_context` is present, else a 1x1 pbuffer, deletes everything, destroys the context and joins. Log at info.

*Alternative:* require the application's context at release (option A) or only drain at acquire (option B). Rejected in A9: the pipeline can drop the last capture reference on a thread the application does not control, and skipped cleanup pins SoC buffers.

### D5. FD closing
EGL borrows FDs; the importer reports "borrowed" so R11 closes them right after `importFrame` returns, on success and failure.

## Risks / Trade-offs

- [A driver cannot create a second shared context] → teardown logs an error and leaks for that session; the platform must then document context-current teardown. Track per platform (roadmap A9 open item).
- [Application destroys its display or share group first] → undefined behaviour per proposal §3.4; documented in the header.
- [Deleting a texture another context still has bound] → GL keeps the object alive until unbound; no crash.

## Migration Plan

None.
