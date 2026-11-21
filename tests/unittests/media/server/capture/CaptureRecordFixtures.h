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

#ifndef FIREBOLT_RIALTO_CAPTURE_CAPTURE_RECORD_FIXTURES_H_
#define FIREBOLT_RIALTO_CAPTURE_CAPTURE_RECORD_FIXTURES_H_

#include "ICaptureSession.h"

#include <cstdint>
#include <vector>

namespace firebolt::rialto::capture::test
{
constexpr uint32_t kDrmFormatArgb8888{0x34325241}; // 'AR24'
constexpr uint32_t kDrmFormatNv12{0x3231564e};     // 'NV12'
constexpr uint32_t kDrmFormatYuv420{0x32315559};   // 'YU12'
constexpr uint64_t kDrmFormatModLinear{0};

constexpr uint32_t kWidth{1920};
constexpr uint32_t kHeight{1088};
constexpr uint32_t kVisibleHeight{1080};
constexpr uint64_t kLumaBytes{static_cast<uint64_t>(kWidth) * kHeight};

constexpr uint64_t kFirstObjectId{101};
constexpr uint64_t kSecondObjectId{205};

/**
 * @brief Builds valid capture records from SoC capture spec 5.2 (examples A to D) and 5.3 (NV12 linear).
 *
 * Every exported descriptor is a real memfd owned by the fixture. Descriptors still open (not -1) in the records
 * are closed when the fixture is destroyed, so tests may close them earlier and set them to -1.
 */
class CaptureRecordFixtures
{
public:
    CaptureRecordFixtures() = default;
    ~CaptureRecordFixtures();
    CaptureRecordFixtures(const CaptureRecordFixtures &) = delete;
    CaptureRecordFixtures &operator=(const CaptureRecordFixtures &) = delete;

    /** NV12 linear capabilities with the given slot limits. */
    static CaptureCapabilities nv12Capabilities(uint32_t minimumSlots = 1, uint32_t maximumSlots = 4);

    /** An NV12 pool template with \a slots layouts and zero object IDs. */
    static void makeNv12Pool(CapturePool &pool, uint32_t slots);

    /** Example A: one plane (ARGB8888) in one object, one descriptor. */
    void makeExampleA(CapturedFrame &frame, uint32_t slot = 0, uint64_t token = 1);

    /** Example B (and 5.3): NV12, two planes sharing one object, one descriptor. */
    void makeExampleB(CapturedFrame &frame, uint32_t slot = 0, uint64_t token = 1);

    /** Example C (and 5.3): NV12, two planes in two objects, two descriptors. */
    void makeExampleC(CapturedFrame &frame, uint32_t slot = 0, uint64_t token = 1);

    /** Example D: YUV420, planes 0 and 1 sharing one object and plane 2 in another, two descriptors. */
    void makeExampleD(CapturedFrame &frame, uint32_t slot = 0, uint64_t token = 1);

    /** Creates a memfd of \a sizeBytes owned by the fixture. */
    int createFile(uint64_t sizeBytes);

private:
    void fillCommon(CapturedFrame &frame, uint32_t drmFormat, uint32_t slot, uint64_t token);

    std::vector<int> m_files;
};

} // namespace firebolt::rialto::capture::test

#endif // FIREBOLT_RIALTO_CAPTURE_CAPTURE_RECORD_FIXTURES_H_
