# Spec Delta

## Purpose

Defines how captured frames are imported into EGL images and exposed as GL external textures, how file descriptors are handled during import, and when and on which thread the resulting GL and EGL objects are deleted.

## ADDED Requirements

### Requirement: EGL libraries are loaded at run time
A GL capture SHALL load EGL at run time when it is created, and an application that creates no GL capture SHALL NOT load EGL through Rialto. If a required EGL or GL extension is missing on the configured context, acquisitions SHALL fail without crashing.

#### Scenario: No GL capture
- **WHEN** an application uses Rialto without creating a GL capture
- **THEN** Rialto does not load `libEGL.so.1`

#### Scenario: Missing DMA-BUF import extension
- **WHEN** the configured display lacks `EGL_EXT_image_dma_buf_import`
- **THEN** acquisitions return no frame, the error is logged once, and every received descriptor is closed

### Requirement: Frames are exposed as external textures
Each imported frame SHALL be exposed as exactly one texture of target `GL_TEXTURE_EXTERNAL_OES`, created from one EGL image whose attributes use the frame's DRM format, the descriptor of each plane's object (repeated when planes share an object), each plane's offset and pitch, the modifier for every plane, and colour hints from the frame's colour description.

#### Scenario: Two planes in one object
- **WHEN** an NV12 frame has both planes in one DMA-BUF object (SoC spec example B)
- **THEN** the EGL attribute list uses the same descriptor for planes 0 and 1 with their own offsets and pitches

#### Scenario: Two objects
- **WHEN** an NV12 frame has its planes in two objects (SoC spec example C)
- **THEN** each plane uses its own object's descriptor

#### Scenario: Colour hints
- **WHEN** a frame is BT709, narrow range, with co-sited horizontal and midpoint vertical siting
- **THEN** the image is created with the matching colour-space, sample-range and siting hints

#### Scenario: Texture accessors
- **WHEN** the application reads a GL frame
- **THEN** `textureTarget()` is `GL_TEXTURE_EXTERNAL_OES` and `textureIds()` has one ID

### Requirement: Descriptors are closed after import
Rialto SHALL close every descriptor passed to EGL image creation once creation returns, whether it succeeded or failed. A failed import SHALL destroy every texture and EGL image created by that attempt.

#### Scenario: Successful import
- **WHEN** a frame is imported successfully
- **THEN** none of the frame's received descriptors remains open in the client

#### Scenario: Failed image creation
- **WHEN** EGL image creation fails
- **THEN** every received descriptor is closed and no texture or image from that attempt remains

### Requirement: Releasing a frame makes no GL calls
Releasing a frame or dropping a capture SHALL make no EGL or GL call on the releasing thread. Imports that become deletable because of a release SHALL be queued.

#### Scenario: Release on a thread without a context
- **WHEN** the application drops the last reference to a frame on a thread with no current context
- **THEN** no EGL or GL function is called on that thread

### Requirement: Deletions run during acquisition
`acquireCurrentFrame()`, which the application calls with a context of the configured share group current, SHALL first delete every queued import and SHALL then delete entries invalidated or evicted by that acquisition directly.

#### Scenario: Retired import deleted at the next acquisition
- **WHEN** a frame using an invalidated import is released, and the application then acquires again
- **THEN** that import's texture and image are deleted during the acquisition

### Requirement: Final teardown always cleans up
When the capture and all its frames are gone, Rialto SHALL delete every remaining texture and EGL image. If the creation context is current on that thread, it SHALL delete them there; otherwise it SHALL create a temporary context sharing the application's objects on a Rialto thread, delete them, destroy that context, and log at info level. The application never needs a current context to release a frame or destroy the capture; the display and share group must outlive every frame and the capture.

#### Scenario: Teardown with the context current
- **WHEN** the application releases its last frame and destroys the capture with its context current
- **THEN** all textures and images are deleted on that thread and no temporary context is created

#### Scenario: Teardown from another thread
- **WHEN** the last capture reference is dropped on a thread with no current context
- **THEN** a temporary shared context deletes all textures and images, and an info message is logged

#### Scenario: No buffer leaks
- **WHEN** a capture session ends and teardown has completed
- **THEN** the client process's open descriptor count is back to its value before the capture was created

### Requirement: Single-layout frames
GL frames SHALL report `layout()` as `RGBA` and one resource.

#### Scenario: Layout of an NV12 frame
- **WHEN** an NV12 frame is acquired
- **THEN** its layout is `RGBA` and it has one texture
