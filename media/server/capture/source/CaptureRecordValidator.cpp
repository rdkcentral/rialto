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

#include "CaptureRecordValidator.h"

namespace firebolt::rialto::capture
{
ValidationResult validateCapabilities(const CaptureCapabilities &capabilities, uint32_t builtSize)
{
    if (capabilities.size < builtSize)
        return ValidationResult::fail("capabilities.size: record smaller than this build expects");
    if (capabilities.minimumSlots < 1)
        return ValidationResult::fail("capabilities.minimumSlots: must be at least 1");
    if (capabilities.minimumSlots > capabilities.maximumSlots)
        return ValidationResult::fail("capabilities.slotRange: minimumSlots greater than maximumSlots");
    if (capabilities.maximumContentSize.width == 0 || capabilities.maximumContentSize.height == 0)
        return ValidationResult::fail("capabilities.maximumContentSize: zero width or height");
    return ValidationResult::pass();
}

ValidationResult validatePool(const CapturePool &pool, const CaptureCapabilities &capabilities, uint32_t requestedCount)
{
    if (requestedCount < capabilities.minimumSlots || requestedCount > capabilities.maximumSlots)
        return ValidationResult::fail("pool.requestedCount: outside the capability slot limits");
    if (pool.slotLayouts.size() != requestedCount)
        return ValidationResult::fail("pool.slotCount: number of slot layouts differs from the requested count");
    for (uint32_t position = 0; position < pool.slotLayouts.size(); ++position)
    {
        const CaptureSlotLayout &kLayout = pool.slotLayouts[position];
        if (kLayout.slotIndex != position)
            return ValidationResult::fail("pool.slotIndex: slot index differs from its position");
        for (const FramePlane &kPlane : kLayout.planes)
        {
            if (kPlane.dmaBufObjectId != 0)
                return ValidationResult::fail("pool.templateObjectId: template plane has a non-zero object ID");
        }
    }
    if (pool.backingSize.width == 0 || pool.backingSize.height == 0)
        return ValidationResult::fail("pool.backingSize: zero width or height");
    if (pool.backingSize.width > capabilities.maximumContentSize.width ||
        pool.backingSize.height > capabilities.maximumContentSize.height)
        return ValidationResult::fail("pool.backingSize: larger than maximumContentSize");
    return ValidationResult::pass();
}

} // namespace firebolt::rialto::capture
