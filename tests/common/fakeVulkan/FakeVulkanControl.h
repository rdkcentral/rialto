/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FAKE_VULKAN_CONTROL_H_
#define FAKE_VULKAN_CONTROL_H_

/**
 * @file FakeVulkanControl.h
 *
 * Control functions of the fake libvulkan used by tests. Tests resolve them with dlsym on the loaded library.
 */

extern "C"
{
    /** Resets every counter and setting. */
    void fakeVulkanReset();
    /** Makes vkGetDeviceProcAddr return null for \a name (empty string: none). */
    void fakeVulkanSetMissingDeviceCommand(const char *name);
    /** Objects created and not destroyed: images, views, samplers, conversions, memory, pools, fences. */
    int fakeVulkanLiveObjects();
    /** Calls to vkQueueSubmit since the last reset. */
    int fakeVulkanSubmitCount();
    /** Image memory barriers recorded since the last reset. */
    int fakeVulkanBarrierCount();
}

#endif // FAKE_VULKAN_CONTROL_H_
