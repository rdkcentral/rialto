# Design

## Context

See proposal.md for the motivation. The interim header and protos are in `platform-architecture-docs/working/rialto-interim/api-headers/IVideoFrameCapture.h`, `api-headers/IMediaPipeline.h` and `api-proto/`. Public headers live in `media/public/include/`, protos in `proto/` with `proto/CMakeLists.txt`, and client entry points in `media/client/main` (for example `MediaPipelineProxy`). V1.1 is not implemented yet, so the interim names may still change; only additions to shipping declarations are allowed (V1-P1).

## Goals / Non-Goals

**Goals:**
- Freeze the parts of the API and wire that no open question touches.
- Prove V1-P1 to V1-P3 with build and component tests.

**Non-Goals:**
- Any capture behaviour: registration (R5), attach/detach (R6), acquisition (R9, R11), import (R12).
- `getCapabilities` and its types (R8, pending A2) and Vulkan types (R13, pending A8).

## Decisions

### D1. Split the header by open question, not by layer
The header ships GL and common types now. `getCapabilities()` and the `*Caps` structs are added by R8, and the Vulkan types by R13.

*Why:* A2 and A8 can still change those declarations. Adding pure virtuals later is safe because V1.1 has no consumers yet; changing a shipped declaration would not be.
*Alternative:* ship the full interim header now and patch it later. Rejected: it would freeze shapes the spec authors are still deciding.

### D2. Factory entry point
`IVideoFrameCaptureFactory::createFactory()` is implemented in `libRialtoClient.so` and returns a factory. In this change `createGLVideoFrameCapture()` always returns null; R5 makes it register with the server.

### D3. Proto taken from the interim file, minus capabilities
Copy `videoframecapturemodule.proto` from the interim folder, removing `GetVideoFrameCaptureCapabilitiesRequest/Response` and the `getCapabilities` RPC. Keep every other message, field number and comment. The service still declares `getPool`, `acquireCurrentFrame` and `releaseFrame`; their server handlers arrive in R8 and R9.

*Why:* tag numbers are the compatibility contract; copying avoids renumbering mistakes. A17 says to freeze the proto before R4 lands, so this change is the freeze point for everything except the capabilities messages.

### D4. Unimplemented RPCs answer through the existing failure path
The server registers no `VideoFrameCaptureModule` yet, so a call fails as for any unknown service; `MediaPipelineModule` attach/detach handlers call `controller->SetFailed("not supported")`. The client maps both to null or `false`.

## Risks / Trade-offs

- [The spec authors change a frozen field under A17] → raise the freeze explicitly when sending A2; until R4 merges, a change is a copy update.
- [EGL types in a public header] → the header uses `void *` aliases for `EGLDisplay` and `EGLContext`, as the interim header does, so no EGL header is required to compile consumers that only pass handles.

## Migration Plan

None. Additive only; rollback is reverting the change.
