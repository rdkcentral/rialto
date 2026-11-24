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

#include "GLVideoFrameCapture.h"
#include "VideoFrameCaptureIpcMock.h"

#include <gtest/gtest.h>

#include <vector>

using namespace firebolt::rialto;
using namespace firebolt::rialto::client;
using ::testing::_;
using ::testing::DoAll;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SetArgReferee;

namespace
{
constexpr std::uint32_t kCaptureId{12};

CaptureCapabilitiesReply serverReply()
{
    CaptureCapabilitiesReply reply;
    reply.maxConcurrentFrames = 3;
    reply.maxResourcesPerFrame = 1;
    reply.drmFourccs = {0x3231564e};
    reply.drmModifiers = {0};
    return reply;
}
} // namespace

class GlCaptureCapabilitiesTest : public ::testing::Test
{
protected:
    std::shared_ptr<NiceMock<VideoFrameCaptureIpcMock>> m_ipc{std::make_shared<NiceMock<VideoFrameCaptureIpcMock>>()};
    GLVideoFrameCapture m_sut{m_ipc, kCaptureId, nullptr, nullptr};
};

TEST_F(GlCaptureCapabilitiesTest, OkFillsTheCommonLimits)
{
    EXPECT_CALL(*m_ipc, getCapabilities(kCaptureId, GraphicsAPIType::GL, _))
        .WillOnce(DoAll(SetArgReferee<2>(serverReply()), Return(VideoFrameCaptureStatus::OK)));
    GLVideoFrameCaptureCaps caps{};
    caps.size = 0;
    caps.common.size = 0;
    VideoFrameCaptureStatus status{VideoFrameCaptureStatus::FATAL_ERROR};
    EXPECT_EQ(m_sut.getCapabilities(caps, status), VideoFrameCaptureStatus::OK);
    EXPECT_EQ(status, VideoFrameCaptureStatus::OK);
    EXPECT_EQ(caps.size, sizeof(GLVideoFrameCaptureCaps));
    EXPECT_EQ(caps.common.size, sizeof(VideoFrameCaptureCaps));
    EXPECT_EQ(caps.common.maxConcurrentFrames, 3u);
    EXPECT_EQ(caps.common.maxResourcesPerFrame, 1u);
    EXPECT_EQ(caps.common.supportsProtectedContent, 0u);
    EXPECT_EQ(caps.common.supportsHdr, 0u);
}

TEST_F(GlCaptureCapabilitiesTest, FailureLeavesTheRecordUnchangedAndStatusesAgree)
{
    for (VideoFrameCaptureStatus failure : {VideoFrameCaptureStatus::SESSION_NOT_READY,
                                            VideoFrameCaptureStatus::FATAL_ERROR, VideoFrameCaptureStatus::NO_RESOURCES})
    {
        EXPECT_CALL(*m_ipc, getCapabilities(kCaptureId, GraphicsAPIType::GL, _)).WillOnce(Return(failure));
        GLVideoFrameCaptureCaps caps{};
        caps.common.maxConcurrentFrames = 77;
        caps.reserved_1 = 5;
        VideoFrameCaptureStatus status{VideoFrameCaptureStatus::OK};
        EXPECT_EQ(m_sut.getCapabilities(caps, status), failure);
        EXPECT_EQ(status, failure);
        EXPECT_EQ(caps.common.maxConcurrentFrames, 77u);
        EXPECT_EQ(caps.reserved_1, 5u);
    }
}
