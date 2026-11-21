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

#ifndef CAPTURE_CORE_TEST_BASE_H_
#define CAPTURE_CORE_TEST_BASE_H_

#include "CaptureCore.h"
#include "FrameImporterMock.h"
#include "VideoFrameCaptureIpcMock.h"

#include <gtest/gtest.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cstdint>
#include <memory>
#include <vector>

using namespace firebolt::rialto;
using namespace firebolt::rialto::client;
using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::SetArgReferee;
using ::testing::StrictMock;

class CaptureCoreTestBase : public ::testing::Test
{
protected:
    static constexpr std::uint32_t kCaptureId{5};
    static constexpr std::uint32_t kPoolId{2};

    // An OK acquisition of an NV12 frame in one object, with a real descriptor.
    CaptureAcquireResult okResult(std::uint32_t slot, std::uint64_t token, std::uint64_t objectId = 101)
    {
        CaptureAcquireResult result;
        result.status = CaptureAcquireStatus::OK;
        CaptureFrameDescription &description = result.description;
        description.poolId = kPoolId;
        description.slotIndex = slot;
        description.acquisitionToken = token;
        description.presentationTimeNs = 40000000;
        description.visibleWidth = 1280;
        description.visibleHeight = 720;
        description.drmFormat = 0x3231564e;
        description.backingWidth = 1920;
        description.backingHeight = 1088;
        description.objects = {{objectId, 4096}};
        description.planes = {{0, objectId, 0, 2048, 1920}, {1, objectId, 2048, 2048, 1920}};
        description.colour = {2, 2, 1, 2};
        result.fds = {{objectId, memfd_create("capture-core-test", MFD_CLOEXEC)}};
        result.pool = CapturePoolDescription{kPoolId, 3};
        return result;
    }

    static CaptureAcquireResult statusResult(CaptureAcquireStatus status)
    {
        CaptureAcquireResult result;
        result.status = status;
        return result;
    }

    // The importer borrows descriptors (like EGL) and returns a new image.
    void expectImport(int times = 1)
    {
        EXPECT_CALL(*m_importer, importFrame(_, _, _))
            .Times(times)
            .WillRepeatedly(Invoke(
                [this](const CaptureFrameDescription &, const std::vector<CaptureObjectFd> &, bool &fdsConsumed)
                {
                    fdsConsumed = false;
                    return std::make_shared<NiceMock<ImportedImageMock>>();
                }));
    }

    std::shared_ptr<StrictMock<VideoFrameCaptureIpcMock>> m_ipc{std::make_shared<StrictMock<VideoFrameCaptureIpcMock>>()};
    std::shared_ptr<NiceMock<FrameImporterMock>> m_importer{std::make_shared<NiceMock<FrameImporterMock>>()};
    std::shared_ptr<CaptureCore> m_core{std::make_shared<CaptureCore>(m_ipc, kCaptureId, m_importer)};

    void SetUp() override
    {
        // tests that do not check releases still release every frame they acquire
        EXPECT_CALL(*m_ipc, releaseFrame(kCaptureId, _, _, _)).Times(::testing::AnyNumber()).WillRepeatedly(Return(true));
    }
};

#endif // CAPTURE_CORE_TEST_BASE_H_
