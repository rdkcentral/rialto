## Why

Vulkan applications need captured frames as sampleable `VkImage`s. The interim API exposed only `image` and `imageView`, which cannot be sampled for YCbCr formats, and left the layout transition and synchronization undefined (roadmap A8). With the A8 proposal accepted, the rules are fixed: no `RGBA_SEPARATE_ALPHA`, Rialto records the queue-ownership barriers on the queue given at creation, and each image resource exposes Rialto's sampler and YCbCr conversion (option X). The SoC spec §6.5 defines the import itself. R11 provides the graphics-independent lease and cache; this change adds the Vulkan API types and importer.

## What Changes

- **Public API (moved from R4):** `VulkanImageResource { image, imageView, sampler, ycbcrConversion }`, `IVulkanVideoFrame` (`imageFormat()`, `imageLayout()`, `images()`), `VulkanFrameAcquisition`, `IVulkanVideoFrameCapture::acquireCurrentFrame()`, and `IVideoFrameCaptureFactory::createVulkanVideoFrameCapture(physicalDevice, device, queue, queueFamilyIndex, requiredImageLayout)`.
- **New finding, header addition:** `queueFamilyIndex` is added, because Vulkan cannot report a `VkQueue`'s family and Rialto needs it for its command pool and barriers. To confirm with the spec authors with the A8 acceptance.
- Load `libvulkan.so.1` at run time; registration with the server uses API type VULKAN (R5's server already accepts it). Creation returns null if Vulkan cannot be loaded, the device is below Vulkan 1.1, or required device features or extensions are missing.
- Import per SoC spec §6.5: a DRM-modifier `VkImage` with external DMA-BUF memory, one memory import per object (a duplicated FD for every extra import), disjoint-plane binding where required, then a YCbCr conversion from the frame's colour description, an image view and a sampler using it. FD ownership passes to Vulkan on a successful allocation; every other FD is closed; failures unwind in reverse order.
- Samplers and conversions are cached by format, modifier and colour description, so frames sharing a description return the same handles; `ycbcrConversion` is null for RGB formats.
- Barriers (A8 part 2): at each changed acquisition, Rialto submits on the creation queue an acquire from `VK_QUEUE_FAMILY_FOREIGN_EXT` with old layout `GENERAL` to `requiredImageLayout`. Released frames get a release barrier back to FOREIGN at the next acquisition, and their HAL slot is released once that submission's fence signals.
- `acquireCurrentFrame()` must be called while the creation queue is externally synchronized; the application samples on that queue, or synchronizes other queues itself.
- `layout()` is `RGBA` and `numResources()` is 1.

## Capabilities

### New Capabilities

- `vulkan-frame-import`: The Vulkan capture API, how captured frames become sampleable Vulkan images with Rialto-owned samplers and YCbCr conversions, the queue-ownership barriers and their synchronization, and Vulkan object cleanup.

### Modified Capabilities

None.

## Impact

- **Public API:** additions to `media/public/include/IVideoFrameCapture.h` (Vulkan types, factory method with `queueFamilyIndex`); `<vulkan/vulkan.h>` needed only at build time for consumers using them.
- **Client:** a Vulkan importer implementing R11's `IFrameImporter`, a `VulkanLibrary` run-time loader and function table, a Vulkan capture class, and an R11 core extension so an importer can defer a frame's slot release.
- **Tests:** unit tests with a Vulkan wrapper mock; optional on-target tests with validation layers.
- **Dependencies:** R11, R5 (registration), A8 proposal (accepted). No new link-time dependency.
