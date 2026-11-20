# Design

## Context

See proposal.md for the motivation. R11's `CaptureCore` calls `IFrameImporter::importFrame`, `retire` and `drain`, and sends a frame's release when its last reference is dropped. R5 registers captures by API type. SoC spec §6.5 lists the Vulkan query, import and object order; the A8 proposal (accepted) fixes the barrier rules and option X. Vulkan has no query for the family of a `VkQueue`, so the creation call gains `queueFamilyIndex`.

## Goals / Non-Goals

**Goals:**
- Zero-copy import with Rialto-owned, cached samplers and conversions.
- Correct queue-family ownership transfer with no submission outside `acquireCurrentFrame()` and teardown.

**Non-Goals:**
- Converting to RGBA in Rialto (option Y, rejected in A8).
- Capabilities (R8). Semaphores or fences in the public API.

## Decisions

### D1. Loading and entry points
`VulkanLibrary` dlopens `libvulkan.so.1` and uses the loader's exported core 1.1 trampolines for physical-device queries (`vkGetPhysicalDeviceProperties2`, `...Features2`, `...FormatProperties2`, `...ImageFormatProperties2`), so no `VkInstance` is needed. Device commands come from `vkGetDeviceProcAddr`; a null result for `vkGetMemoryFdPropertiesKHR` or another required command means the extension is not enabled, and creation fails. The header documents the required extensions (`VK_KHR_external_memory_fd`, `VK_EXT_external_memory_dma_buf`, `VK_EXT_image_drm_format_modifier`, `VK_EXT_queue_family_foreign`) and the `samplerYcbcrConversion` feature.

### D2. Import pipeline
For a cache miss: check format/modifier support with `VkDrmFormatModifierPropertiesListEXT` and `vkGetPhysicalDeviceImageFormatProperties2`; create the image with `VkImageDrmFormatModifierExplicitCreateInfoEXT` (plane offsets and pitches) and `VkExternalMemoryImageCreateInfo`; import memory per object with `VkImportMemoryFdInfoKHR` (`dup` for each extra import of the same object), bind with `vkBindImageMemory2` and `VkBindImagePlaneMemoryInfo` when disjoint; get the conversion and sampler from the cache (D3); create the view with `VkSamplerYcbcrConversionInfo`. Report FDs as consumed or closed per import. Unwind in reverse order on failure.

### D3. Sampler and conversion cache
Key: `{VkFormat, modifier, matrix, range, xSiting, ySiting}`. Value: conversion (null for RGB), sampler (clamp-to-edge, unnormalized off, linear filters only if the format reports `SAMPLED_IMAGE_YCBCR_CONVERSION_LINEAR_FILTER`), and a reference count from cache entries and live frames. An entry is destroyed in `drain()` or teardown once its count is zero.

### D4. Command pool, barriers and fences
At creation, Rialto creates a command pool on `queueFamilyIndex` and a small ring of command buffers with fences. During `acquireCurrentFrame()`:
1. record release barriers (old layout `requiredImageLayout`, new `GENERAL`, `srcQueueFamily` = app family, `dst` = `VK_QUEUE_FAMILY_FOREIGN_EXT`) for frames queued since the last call;
2. after a changed import, record the acquire barrier (`src` = FOREIGN, `dst` = app family, `GENERAL` → `requiredImageLayout`);
3. submit both in one `vkQueueSubmit` with a fence;
4. poll earlier fences without blocking; for each signalled one, send the deferred slot releases and recycle the buffer.

### D5. Deferred slot release through R11
Extend R11's `IFrameImporter` with `bool deferRelease(ImportHandle, std::function<void()> sendRelease)`. The Vulkan importer returns `true` and keeps `sendRelease` until D4 step 4; the GL importer returns `false`, keeping R11's behaviour. The callback still runs exactly once.

### D6. Teardown
When the capture and all frames are gone: `vkWaitForFences` on Rialto's fences, destroy views, samplers, conversions and images, free memory, then call the remaining deferred `sendRelease`s. Frames released after the last acquisition do not get a release barrier: their images are destroyed and the GPU only read them, so nothing needs to be made visible to the producer. Vulkan destroy calls need no current context; object destruction is safe on the teardown thread because the application has finished its GPU work before releasing.

## Risks / Trade-offs

- [`queueFamilyIndex` is an API addition beyond the A8 text] → raise with the spec authors with the A8 acceptance; it is required to record barriers at all.
- [Skipping release barriers at teardown] → images are destroyed; the producer never sees them again in this Vulkan device. Documented in D6.
- [Slot release delayed until the next acquisition] → costs about one frame of capacity at steady cadence (A8 note; A3 default to be measured).
- [Driver lacks linear YCbCr filtering] → sampler falls back to nearest, as the format reports.

## Migration Plan

None. Additive API; GL captures are unaffected.
