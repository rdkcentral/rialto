## Why

Applications and rialto-gstreamer need the V1.1 Video Frame Capture API to exist before any capture behaviour can be built behind it, and the server and client need the private capture protocol to talk to each other. Landing the API and wire surface first, with every operation failing cleanly, lets later changes fill in behaviour without touching public headers again, and proves the additions are source- and wire-compatible (proposal §1.3 V1-P1 to V1-P3).

## What Changes

- Add the public header `IVideoFrameCapture.h` with the API-independent and GL parts of proposal §3.1: `GraphicsAPIType`, `GraphicsTextureLayout`, `IVideoFrame`, `IGLVideoFrame`, `GLFrameAcquisition`, `IVideoFrameCapture`, `IGLVideoFrameCapture::acquireCurrentFrame()` and `IVideoFrameCaptureFactory` with `createFactory()` and `createGLVideoFrameCapture()`.
- Add non-pure `IMediaPipeline::attachVideoFrameCapture()` and `detachVideoFrameCapture()` whose defaults return `false`, and the `setVideoWindow()` capture note.
- Add `videoframecapturemodule.proto` with the create, destroy, pool, acquire and release messages, the status enum and the `VideoFrameCaptureModule` service; and the `attachVideoFrameCapture` and `detachVideoFrameCapture` RPCs in `mediapipelinemodule.proto`.
- Until later changes add behaviour, the factory returns null captures and attach/detach return `false`; nothing crashes.
- **Excluded, owned by other changes:**
  - All `getCapabilities` work (the public methods, the `*Caps` structs, the capabilities messages and RPC, and any status value A2 adds) moves to R8 (`vfc-pool-admission`), so A2 sits in one change.
  - The Vulkan-only types (`VulkanImageResource`, `IVulkanVideoFrame`, `VulkanFrameAcquisition`, `IVulkanVideoFrameCapture`, `createVulkanVideoFrameCapture()`) move to R13 (`vfc-client-vulkan-import`), because the A8 proposal changes `VulkanImageResource`.
- No existing declaration, RPC, message or field changes.

## Capabilities

### New Capabilities

- `video-frame-capture-api`: The public Video Frame Capture C++ API surface and the private capture protocol, including how both behave against servers and clients that do not support capture.

### Modified Capabilities

None.

## Impact

- **Public API:** new `media/public/include/IVideoFrameCapture.h`; additive methods in `media/public/include/IMediaPipeline.h`. Installed headers grow; existing binaries are unaffected.
- **Protos:** new `proto/videoframecapturemodule.proto`; additive messages and RPCs in `proto/mediapipelinemodule.proto`; `proto/CMakeLists.txt`.
- **Client:** `media/client/main` gets the factory entry point and default implementations; `MediaPipelineProxy` forwards the new methods.
- **Server:** registers no handler yet; the new RPCs on `MediaPipelineModule` answer with the existing failure path.
- **Build:** `<EGL/egl.h>` is needed only when compiling against `IVideoFrameCapture.h`; `libRialtoClient.so` gains no new link dependency.
- **Downstream:** R5, R6, R11 and R14 build on this surface; R8 and R13 extend it.
