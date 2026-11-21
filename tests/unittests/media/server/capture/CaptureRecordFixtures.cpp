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

#include "CaptureRecordFixtures.h"

#include <sys/mman.h>
#include <unistd.h>

#include <string>

namespace firebolt::rialto::capture::test
{
CaptureRecordFixtures::~CaptureRecordFixtures()
{
    for (int fd : m_files)
    {
        // close only descriptors a test has not already closed: the number may since have been reused for
        // something else, so check that it still refers to a fixture memfd
        char link[256]{};
        const std::string kPath = "/proc/self/fd/" + std::to_string(fd);
        if (readlink(kPath.c_str(), link, sizeof(link) - 1) > 0 &&
            std::string(link).rfind("/memfd:capture-record-fixture", 0) == 0)
        {
            close(fd);
        }
    }
}

CaptureCapabilities CaptureRecordFixtures::nv12Capabilities(uint32_t minimumSlots, uint32_t maximumSlots)
{
    CaptureCapabilities capabilities;
    capabilities.drmFormat = kDrmFormatNv12;
    capabilities.drmModifier = kDrmFormatModLinear;
    capabilities.maximumContentSize = Size{kWidth, kHeight};
    capabilities.minimumSlots = minimumSlots;
    capabilities.maximumSlots = maximumSlots;
    return capabilities;
}

void CaptureRecordFixtures::makeNv12Pool(CapturePool &pool, uint32_t slots)
{
    pool.backingSize = Size{kWidth, kHeight};
    pool.slotLayouts.clear();
    for (uint32_t slot = 0; slot < slots; ++slot)
    {
        pool.slotLayouts.push_back({slot, {{0, 0, 0, kLumaBytes, kWidth}, {1, 0, kLumaBytes, kLumaBytes / 2, kWidth}}});
    }
}

void CaptureRecordFixtures::setSingleObjectId(CapturedFrame &frame, uint64_t objectId)
{
    for (auto &object : frame.dmaBufObjects)
        object.objectId = objectId;
    for (auto &plane : frame.planes)
        plane.dmaBufObjectId = objectId;
    for (auto &exported : frame.exportedDmaBufs)
        exported.dmaBufObjectId = objectId;
}

int CaptureRecordFixtures::createFile(uint64_t sizeBytes)
{
    const int kFd = memfd_create("capture-record-fixture", MFD_CLOEXEC);
    if (kFd >= 0)
    {
        if (ftruncate(kFd, static_cast<off_t>(sizeBytes)) != 0)
        {
            close(kFd);
            return -1;
        }
        m_files.push_back(kFd);
    }
    return kFd;
}

void CaptureRecordFixtures::fillCommon(CapturedFrame &frame, uint32_t drmFormat, uint32_t slot, uint64_t token)
{
    frame.slotIndex = slot;
    frame.acquisitionToken = token;
    frame.presentationTimeNs = 40000000;
    frame.visibleRegion = Rectangle{0, 0, Size{kWidth, kVisibleHeight}};
    frame.dmaBufObjectsFlushed = false;
    frame.capabilities = nv12Capabilities();
    frame.capabilities.drmFormat = drmFormat;
    frame.backingSize = Size{kWidth, kHeight};
    if (drmFormat == kDrmFormatArgb8888)
    {
        frame.colorDescription = CaptureColorDescription{};
    }
    else
    {
        frame.colorDescription = {CaptureYcbcrMatrix::BT709, CaptureSampleRange::NARROW, CaptureChromaSiting::COSITED,
                                  CaptureChromaSiting::MIDPOINT};
    }
}

void CaptureRecordFixtures::makeExampleA(CapturedFrame &frame, uint32_t slot, uint64_t token)
{
    constexpr uint64_t kSize{kLumaBytes * 4};
    fillCommon(frame, kDrmFormatArgb8888, slot, token);
    frame.dmaBufObjects = {{kFirstObjectId, kSize}};
    frame.planes = {{0, kFirstObjectId, 0, kSize, kWidth * 4}};
    frame.exportedDmaBufs = {{kFirstObjectId, createFile(kSize)}};
}

void CaptureRecordFixtures::makeExampleB(CapturedFrame &frame, uint32_t slot, uint64_t token)
{
    constexpr uint64_t kSize{kLumaBytes + kLumaBytes / 2};
    constexpr uint64_t kPlaneOffset{kLumaBytes};
    fillCommon(frame, kDrmFormatNv12, slot, token);
    frame.dmaBufObjects = {{kFirstObjectId, kSize}};
    frame.planes = {{0, kFirstObjectId, 0, kPlaneOffset, kWidth},
                    {1, kFirstObjectId, kPlaneOffset, kSize - kPlaneOffset, kWidth}};
    frame.exportedDmaBufs = {{kFirstObjectId, createFile(kSize)}};
}

void CaptureRecordFixtures::makeExampleC(CapturedFrame &frame, uint32_t slot, uint64_t token)
{
    constexpr uint64_t kLumaSize{kLumaBytes};
    constexpr uint64_t kChromaSize{kLumaBytes / 2};
    fillCommon(frame, kDrmFormatNv12, slot, token);
    frame.dmaBufObjects = {{kFirstObjectId, kLumaSize}, {kSecondObjectId, kChromaSize}};
    frame.planes = {{0, kFirstObjectId, 0, kLumaSize, kWidth}, {1, kSecondObjectId, 0, kChromaSize, kWidth}};
    frame.exportedDmaBufs = {{kFirstObjectId, createFile(kLumaSize)}, {kSecondObjectId, createFile(kChromaSize)}};
}

void CaptureRecordFixtures::makeExampleD(CapturedFrame &frame, uint32_t slot, uint64_t token)
{
    constexpr uint64_t kQuarter{kLumaBytes / 4};
    constexpr uint64_t kFirstSize{kLumaBytes + kQuarter};
    constexpr uint64_t kPlaneOffset{kLumaBytes};
    fillCommon(frame, kDrmFormatYuv420, slot, token);
    frame.dmaBufObjects = {{kFirstObjectId, kFirstSize}, {kSecondObjectId, kQuarter}};
    frame.planes = {{0, kFirstObjectId, 0, kPlaneOffset, kWidth},
                    {1, kFirstObjectId, kPlaneOffset, kFirstSize - kPlaneOffset, kWidth / 2},
                    {2, kSecondObjectId, 0, kQuarter, kWidth / 2}};
    frame.exportedDmaBufs = {{kFirstObjectId, createFile(kFirstSize)}, {kSecondObjectId, createFile(kQuarter)}};
}

} // namespace firebolt::rialto::capture::test
