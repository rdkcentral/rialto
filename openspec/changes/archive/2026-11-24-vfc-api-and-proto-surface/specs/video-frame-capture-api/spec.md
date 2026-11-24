# Spec Delta

## Purpose

Defines the public Video Frame Capture API surface and the private capture protocol, and how they behave with peers that do not support capture, so applications and rialto-gstreamer can compile against V1.1 safely.

## ADDED Requirements

### Requirement: Public capture header
Rialto SHALL install `IVideoFrameCapture.h` declaring the capture factory, the GL capture and frame interfaces, and the common frame interface of proposal §3.1, and SHALL need EGL headers only at build time.

#### Scenario: Application includes the header
- **WHEN** an application includes `IVideoFrameCapture.h` and links `libRialtoClient.so`
- **THEN** it compiles, and `libRialtoClient.so` has no link-time dependency on `libEGL`

#### Scenario: Application that does not use capture
- **WHEN** an application does not include `IVideoFrameCapture.h` and creates no capture
- **THEN** its behaviour is unchanged and no graphics library is loaded

### Requirement: Platform detail stays private
The public capture API SHALL NOT expose DMA-BUF descriptors, DRM formats, modifiers or colour metadata.

#### Scenario: Public header inspected
- **WHEN** the installed `IVideoFrameCapture.h` is inspected
- **THEN** it declares no file descriptor, DRM format, modifier or colour field

### Requirement: GL capture declarations
The header SHALL declare `GraphicsAPIType`, `GraphicsTextureLayout`, `IVideoFrame`, `IGLVideoFrame`, `GLFrameAcquisition`, `IVideoFrameCapture`, `IGLVideoFrameCapture::acquireCurrentFrame()`, and `IVideoFrameCaptureFactory` with `createFactory()` and `createGLVideoFrameCapture(EGLDisplay, EGLContext)`.

#### Scenario: Declarations present
- **WHEN** an application calls `IVideoFrameCaptureFactory::createFactory()->createGLVideoFrameCapture(display, context)`
- **THEN** the call compiles against the installed header

### Requirement: Pipeline methods are additive and source compatible
`IMediaPipeline` SHALL gain non-pure `attachVideoFrameCapture(const std::shared_ptr<IVideoFrameCapture> &)` and `detachVideoFrameCapture()` whose default implementations return `false`. Existing implementations and mocks of `IMediaPipeline` SHALL compile unchanged.

#### Scenario: Existing mock
- **WHEN** an existing `IMediaPipeline` mock that does not override the new methods is compiled against the V1.1 header
- **THEN** it compiles, and calling either new method on it returns `false`

### Requirement: Private capture protocol
Rialto SHALL define a private `VideoFrameCaptureModule` service with `createVideoFrameCapture`, `destroyVideoFrameCapture`, `getPool`, `acquireCurrentFrame` and `releaseFrame`, with the message fields and tag numbers of the interim proto, and SHALL add `attachVideoFrameCapture` and `detachVideoFrameCapture` to `MediaPipelineModule`. File descriptors SHALL appear only in `AcquireCurrentVideoFrameResponse.exported_dma_bufs[].fd`, marked `field_is_fd`.

#### Scenario: Proto compiles with the shipping import roots
- **WHEN** the protos are compiled with the shipping `rialtoipc.proto` import
- **THEN** compilation succeeds and existing generated messages are unchanged

#### Scenario: Only one FD field
- **WHEN** the capture protos are inspected
- **THEN** the only `field_is_fd` field is `VideoFrameCaptureExportedDmaBuf.fd`

### Requirement: Capture fails cleanly without server support
Until a server registers capture support, `createGLVideoFrameCapture()` SHALL return null and `attachVideoFrameCapture()` and `detachVideoFrameCapture()` SHALL return `false`, without crashing or affecting the pipeline.

#### Scenario: V1.1 client and a server without capture
- **WHEN** a V1.1 client calls `createGLVideoFrameCapture()` against a server that does not provide `VideoFrameCaptureModule`
- **THEN** it returns null and the client keeps working

#### Scenario: Attach on a server without the RPC
- **WHEN** a V1.1 client calls `attachVideoFrameCapture()` against a server that does not implement it
- **THEN** it returns `false` and the pipeline keeps working

### Requirement: Old clients are unaffected
A client built against the shipping headers SHALL behave exactly as before against a V1.1 server.

#### Scenario: Shipping client
- **WHEN** a client built without V1.1 headers plays content through a V1.1 server
- **THEN** its observable behaviour and messages are unchanged
