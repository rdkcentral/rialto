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
#include "RialtoLogging.h"

#include <algorithm>
#include <map>
#include <set>

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

namespace
{
ValidationResult validateIdentity(const CapturedFrame &frame, uint32_t admittedSlots)
{
    if (frame.slotIndex >= admittedSlots)
        return ValidationResult::fail("frame.slotIndex: outside the admitted pool");
    if (frame.acquisitionToken == kInvalidAcquisitionToken)
        return ValidationResult::fail("frame.acquisitionToken: zero token");
    if (frame.planes.empty())
        return ValidationResult::fail("frame.planes: no planes");

    std::set<uint64_t> listedObjects;
    for (const DmaBufObject &kObject : frame.dmaBufObjects)
    {
        if (kObject.objectId == 0)
            return ValidationResult::fail("frame.objectId: zero object ID");
        if (!listedObjects.insert(kObject.objectId).second)
            return ValidationResult::fail("frame.duplicateObject: object listed more than once");
    }

    std::set<uint64_t> referencedObjects;
    for (const FramePlane &kPlane : frame.planes)
    {
        if (kPlane.dmaBufObjectId == 0)
            return ValidationResult::fail("frame.objectId: plane references object ID zero");
        if (listedObjects.count(kPlane.dmaBufObjectId) == 0)
            return ValidationResult::fail("frame.planeObject: plane references an unlisted object");
        referencedObjects.insert(kPlane.dmaBufObjectId);
    }
    if (referencedObjects.size() != listedObjects.size())
        return ValidationResult::fail("frame.unreferencedObject: listed object referenced by no plane");

    std::set<uint64_t> exportedObjects;
    for (const ExportedDmaBuf &kExported : frame.exportedDmaBufs)
    {
        if (referencedObjects.count(kExported.dmaBufObjectId) == 0)
            return ValidationResult::fail("frame.unrelatedDescriptor: descriptor for an unreferenced object");
        if (!exportedObjects.insert(kExported.dmaBufObjectId).second)
            return ValidationResult::fail("frame.duplicateDescriptor: more than one descriptor for an object");
        if (kExported.fd < 0)
            return ValidationResult::fail("frame.invalidDescriptor: negative descriptor");
    }
    if (exportedObjects.size() != referencedObjects.size())
        return ValidationResult::fail("frame.missingDescriptor: referenced object has no descriptor");

    return ValidationResult::pass();
}
constexpr uint32_t fourcc(char a, char b, char c, char d)
{
    return static_cast<uint32_t>(a) | (static_cast<uint32_t>(b) << 8) | (static_cast<uint32_t>(c) << 16) |
           (static_cast<uint32_t>(d) << 24);
}

// DRM RGB formats, which carry no YCbCr interpretation. Any other format is treated as YUV, so an unfamiliar
// format must supply a complete colour description.
bool isRgbFormat(uint32_t drmFormat)
{
    static const std::set<uint32_t> kRgbFormats{
        fourcc('A', 'R', '2', '4'), fourcc('X', 'R', '2', '4'), fourcc('A', 'B', '2', '4'), fourcc('X', 'B', '2', '4'),
        fourcc('R', 'A', '2', '4'), fourcc('R', 'X', '2', '4'), fourcc('B', 'A', '2', '4'), fourcc('B', 'X', '2', '4'),
        fourcc('R', 'G', '1', '6'), fourcc('B', 'G', '1', '6'), fourcc('R', 'G', '2', '4'), fourcc('B', 'G', '2', '4'),
        fourcc('A', 'R', '3', '0'), fourcc('X', 'R', '3', '0'), fourcc('A', 'B', '3', '0'), fourcc('X', 'B', '3', '0'),
    };
    return kRgbFormats.count(drmFormat) != 0;
}

ValidationResult validateColour(const CapturedFrame &frame)
{
    if (isRgbFormat(frame.capabilities.drmFormat))
        return ValidationResult::pass();

    const CaptureColorDescription &kColour = frame.colorDescription;
    if (kColour.matrix == CaptureYcbcrMatrix::UNKNOWN || kColour.range == CaptureSampleRange::UNKNOWN ||
        kColour.horizontalSiting == CaptureChromaSiting::UNKNOWN ||
        kColour.verticalSiting == CaptureChromaSiting::UNKNOWN)
        return ValidationResult::fail("frame.colorDescription: YUV frame without a complete colour description");
    return ValidationResult::pass();
}

// Overflow-safe: only subtracts values already known to be in range.
bool fitsWithin(uint64_t offset, uint64_t length, uint64_t size)
{
    return offset <= size && length <= size - offset;
}

ValidationResult validateRanges(const CapturedFrame &frame)
{
    std::map<uint64_t, uint64_t> objectSizes;
    for (const DmaBufObject &kObject : frame.dmaBufObjects)
        objectSizes[kObject.objectId] = kObject.sizeBytes;

    for (const FramePlane &kPlane : frame.planes)
    {
        if (!fitsWithin(kPlane.offsetBytes, kPlane.lengthBytes, objectSizes.at(kPlane.dmaBufObjectId)))
            return ValidationResult::fail("frame.planeRange: plane extends past the end of its object");
    }

    const Rectangle &kVisible = frame.visibleRegion;
    if (!fitsWithin(kVisible.x, kVisible.size.width, frame.backingSize.width) ||
        !fitsWithin(kVisible.y, kVisible.size.height, frame.backingSize.height))
        return ValidationResult::fail("frame.visibleRegion: visible region extends past the backing size");

    return ValidationResult::pass();
}
} // namespace

ValidationResult validateFrame(const CapturedFrame &frame, uint32_t admittedSlots, const LockedRanges *lockedRanges)
{
    ValidationResult result = validateIdentity(frame, admittedSlots);
    if (result.ok)
        result = validateRanges(frame);
    if (result.ok)
        result = validateColour(frame);
    if (result.ok && lockedRanges && lockedRanges->overlaps(frame))
        result = ValidationResult::fail("frame.slotOverlap: plane overlaps a plane of another locked slot");
    if (result.ok && (frame.visibleRegion.x != 0 || frame.visibleRegion.y != 0))
        result.warning = "frame.visibleOrigin: visible region does not start at (0,0); only its size is used";
    return result;
}

void ValidationWarningLog::report(const ValidationResult &result)
{
    if (result.warning.empty() || std::find(m_logged.begin(), m_logged.end(), result.warning) != m_logged.end())
        return;
    m_logged.push_back(result.warning);
    RIALTO_LOG_ERROR(RIALTO_COMPONENT_SERVER, "Capture HAL record: %s", result.warning.c_str());
}

void LockedRanges::add(const CapturedFrame &frame)
{
    if (frame.dmaBufObjectsFlushed)
        ++m_generation;
    for (const FramePlane &kPlane : frame.planes)
        m_ranges.push_back({m_generation, frame.slotIndex, kPlane.dmaBufObjectId, kPlane.offsetBytes, kPlane.lengthBytes});
}

void LockedRanges::remove(uint32_t slotIndex)
{
    m_ranges.erase(std::remove_if(m_ranges.begin(), m_ranges.end(),
                                  [slotIndex](const Range &range) { return range.slotIndex == slotIndex; }),
                   m_ranges.end());
}

bool LockedRanges::overlaps(const CapturedFrame &frame) const
{
    if (frame.dmaBufObjectsFlushed)
        return false; // a new generation: nothing locked yet refers to the same allocations

    for (const FramePlane &kPlane : frame.planes)
    {
        for (const Range &kLocked : m_ranges)
        {
            if (kLocked.generation != m_generation || kLocked.slotIndex == frame.slotIndex ||
                kLocked.objectId != kPlane.dmaBufObjectId)
                continue;
            // both ranges were validated to fit in their object, so the ends cannot overflow
            const uint64_t kPlaneEnd = kPlane.offsetBytes + kPlane.lengthBytes;
            const uint64_t kLockedEnd = kLocked.offset + kLocked.length;
            if (kPlane.offsetBytes < kLockedEnd && kLocked.offset < kPlaneEnd)
                return true;
        }
    }
    return false;
}

} // namespace firebolt::rialto::capture
