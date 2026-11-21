# Tasks

Unit tests run with `./build_ut.py -s client` using a Vulkan wrapper mock. "Spec" means `specs/vulkan-frame-import/spec.md`; "D#" means a decision in `design.md`.

## 1. Public API

- [x] 1.1 Add the Vulkan types to `IVideoFrameCapture.h`: `VulkanImageResource` with `sampler` and `ycbcrConversion`, `IVulkanVideoFrame`, `VulkanFrameAcquisition`, `IVulkanVideoFrameCapture`, and `createVulkanVideoFrameCapture(physicalDevice, device, queue, queueFamilyIndex, requiredImageLayout)`, with Doxygen for the required extensions, the queue-synchronization rule and the sampler contract. Verify a consumer compiles and `readelf -d libRialtoClient.so` shows no `libvulkan`.

## 2. Loading and creation (D1)

- [x] 2.1 Add `VulkanLibrary` (dlopen, core 1.1 trampolines, `vkGetDeviceProcAddr` table) and a mock table. Verify: missing library, a 1.0 device, missing `samplerYcbcrConversion` and a null `vkGetMemoryFdPropertiesKHR` each make creation return null with nothing registered.
- [x] 2.2 Add the Vulkan capture class registering with API type VULKAN through R5's IPC, creating the command pool and fence ring on `queueFamilyIndex`. Verify `apiType()` is `VULKAN` and the pool uses the given family.

## 3. Import (D2, D3)

- [x] 3.1 Implement image creation and memory import per D2, with FD consume/dup/close rules and reverse-order unwind. Verify with the mock: one object with two planes imports once; an object imported twice uses a duplicated FD; an allocation failure closes its FD and destroys everything created.
- [ ] 3.2 Implement the sampler/conversion cache. Verify: same description returns the same handles; a colour change returns new ones while held frames keep theirs; RGB returns a null conversion; linear filtering only when the format reports it.
- [ ] 3.3 Make Vulkan frames report `imageFormat()`, `imageLayout() == requiredImageLayout`, one resource and `RGBA`. Verify with a unit test.

## 4. Barriers and deferred release (D4, D5)

- [ ] 4.1 Extend R11's `IFrameImporter` with `deferRelease` (GL returns `false`). Verify R11 and R12 tests pass unchanged.
- [ ] 4.2 Record and submit acquire and release barriers in `acquireCurrentFrame()` and send deferred releases when fences signal. Verify with the mock: one acquire barrier per changed frame with `GENERAL` → `requiredImageLayout` and FOREIGN → app family; none for unchanged frames; a released frame's slot release is sent only after its fence signals; no queue submission on a releasing thread.

## 5. Teardown (D6)

- [ ] 5.1 Implement teardown: wait fences, destroy objects in §6.5 order, free memory, send remaining releases. Verify with the mock that every created object is destroyed once and every slot released once.

## 6. Integration

- [ ] 6.1 Add a client component test with the fake HAL and the Vulkan mock: acquire, colour change, release on another thread, acquire, teardown. Verify with `./build_ct.py`.
- [ ] 6.2 Optional on-target test with validation layers: import an NV12 frame, sample it, tear down, and confirm no validation errors or leaked objects. Record the result in the PR.

## Workflow follow-up

- Run `openspec validate vfc-client-vulkan-import --strict` before review.
- Confirm the `queueFamilyIndex` addition with the spec authors as part of the A8 acceptance.
