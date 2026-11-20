# Spec Delta

## Purpose

Defines the Vulkan Video Frame Capture API, how captured frames are imported into sampleable Vulkan images with Rialto-owned samplers and YCbCr conversions, how image ownership moves between the producer and the application's queue, and how Vulkan objects are cleaned up.

## ADDED Requirements

### Requirement: Vulkan capture API
Rialto SHALL declare `VulkanImageResource` with `image`, `imageView`, `sampler` and `ycbcrConversion`; `IVulkanVideoFrame` with `imageFormat()`, `imageLayout()` and `images()`; `VulkanFrameAcquisition`; `IVulkanVideoFrameCapture::acquireCurrentFrame()`; and `createVulkanVideoFrameCapture(physicalDevice, device, queue, queueFamilyIndex, requiredImageLayout)`. Vulkan headers SHALL be needed only at build time.

#### Scenario: Application compiles against the Vulkan API
- **WHEN** an application includes `<vulkan/vulkan.h>` and `IVideoFrameCapture.h` and calls `createVulkanVideoFrameCapture`
- **THEN** it compiles, and `libRialtoClient.so` has no link-time dependency on `libvulkan`

### Requirement: Vulkan capture creation
Creating a Vulkan capture SHALL load Vulkan at run time and register the capture with the server as a Vulkan capture. It SHALL return null if Vulkan cannot be loaded, the device is below Vulkan 1.1, the sampler YCbCr conversion feature is not available, or a required device command is unavailable. An application that creates no Vulkan capture SHALL NOT load Vulkan through Rialto.

#### Scenario: Successful creation
- **WHEN** a Vulkan 1.1 device with the required features and extensions enabled is supplied
- **THEN** a capture is returned whose `apiType()` is `VULKAN`

#### Scenario: Missing DMA-BUF import
- **WHEN** the device does not provide the memory-FD import command
- **THEN** creation returns null and nothing is registered with the server

### Requirement: Frames are sampleable through the provided sampler
Each imported frame SHALL expose one image resource with a DRM-modifier image, an image view, and a sampler created with the frame's YCbCr conversion; `ycbcrConversion` SHALL be the conversion for YUV formats and null for RGB formats. The conversion SHALL use the frame's matrix, range and chroma siting. `imageLayout()` SHALL be the `requiredImageLayout` given at creation, `layout()` SHALL be `RGBA` and `numResources()` 1.

#### Scenario: NV12 frame
- **WHEN** an NV12 BT709 narrow-range frame is acquired
- **THEN** its single resource has a non-null sampler and conversion, and the conversion uses BT709 and narrow range

#### Scenario: RGB frame
- **WHEN** an RGB frame is acquired
- **THEN** its resource has a non-null sampler and a null conversion

### Requirement: Stable sampler handles
Frames that share a format, modifier and colour description SHALL return the same sampler and conversion handles. A frame with a different colour description SHALL return different handles, and earlier frames SHALL keep theirs.

#### Scenario: Same description
- **WHEN** two consecutive frames have the same format, modifier and colour
- **THEN** they return the same sampler handle

#### Scenario: Colour change
- **WHEN** a BT601 frame follows BT709 frames that the application still holds
- **THEN** the BT601 frame returns a new sampler, and the held BT709 frames keep their original sampler

### Requirement: Sampler lifetime
Rialto SHALL keep a sampler and its conversion alive at least until every frame that returned them has been released, and MAY destroy them afterwards. The application SHALL use the sampler as an immutable sampler and SHALL stop using descriptor layouts and pipelines built with it once every frame carrying it is released.

#### Scenario: Sampler still in use
- **WHEN** the application still holds a frame that returned a sampler
- **THEN** that sampler and its conversion are not destroyed

### Requirement: Import follows the SoC specification
Rialto SHALL import each DMA-BUF object into Vulkan memory using the frame's plane layouts and modifier, SHALL give each ownership-transferring import its own valid descriptor, SHALL not close a descriptor whose ownership passed to Vulkan, and SHALL close every other received descriptor. A failed import SHALL destroy every object and free every allocation created by that attempt, in reverse order.

#### Scenario: One object, two planes
- **WHEN** an NV12 frame has both planes in one object
- **THEN** one memory import consumes that object's descriptor

#### Scenario: Object imported twice
- **WHEN** one object must be imported into two memory allocations
- **THEN** the second import uses a duplicated descriptor

#### Scenario: Allocation fails
- **WHEN** a memory import fails
- **THEN** that descriptor is closed, every object created by the attempt is destroyed, and the call returns no new frame

### Requirement: Queue ownership barriers
For each changed acquisition, Rialto SHALL submit on the creation queue a barrier that acquires the image from the external producer queue family into the creation queue family, transitioning from `GENERAL` to `requiredImageLayout`, before returning the frame. Work the application submits on the same queue after the call SHALL be ordered after that barrier. `acquireCurrentFrame()` SHALL be called while the creation queue is externally synchronized.

#### Scenario: Changed frame
- **WHEN** a new frame is returned
- **THEN** exactly one acquire barrier for its image has been submitted on the creation queue with old layout `GENERAL` and new layout `requiredImageLayout`

#### Scenario: Unchanged frame
- **WHEN** the call returns the current frame unchanged
- **THEN** no barrier is submitted

### Requirement: Release returns ownership before the slot is freed
When a frame is released, Rialto SHALL submit a barrier returning its image to the external producer queue family at the next acquisition, and SHALL release the frame's capture slot exactly once, only after that barrier's fence has signalled. Releasing a frame SHALL make no queue submission on the releasing thread. At final teardown, Rialto SHALL wait for its own fences, destroy the images, and then release any slots still pending.

#### Scenario: Release then acquire
- **WHEN** the application releases a frame and later acquires again
- **THEN** the release barrier is submitted during that acquisition, and the slot release is sent once its fence has signalled

#### Scenario: Release on another thread
- **WHEN** the last frame reference is dropped on a thread that does not own the queue
- **THEN** no Vulkan queue submission happens on that thread

### Requirement: Vulkan objects are cleaned up
After the capture and all its frames are gone, Rialto SHALL have destroyed every sampler, image view, conversion and image it created, freed every memory allocation, and released every slot.

#### Scenario: Validation layers after teardown
- **WHEN** a capture session ends and the device is destroyed with validation layers enabled
- **THEN** no object created by Rialto is reported as leaked
