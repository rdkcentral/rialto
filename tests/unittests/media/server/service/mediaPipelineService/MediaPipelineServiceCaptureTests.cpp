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

#include "MediaPipelineServiceTestsFixture.h"
#include "VideoFrameCaptureServiceMock.h"

using firebolt::rialto::server::service::VideoFrameCaptureServiceMock;
using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::SetArgReferee;
using ::testing::StrictMock;

namespace
{
constexpr int kSession{0}; // the fixture's session
constexpr int kMissingSession{42};
constexpr std::uint64_t kCaptureClient{3};
constexpr std::uint32_t kCaptureId{8};
} // namespace

class MediaPipelineServiceCaptureTests : public MediaPipelineServiceTests
{
protected:
    void expectCaptureService()
    {
        EXPECT_CALL(m_playbackServiceMock, getVideoFrameCaptureService()).WillRepeatedly(ReturnRef(m_captureServiceMock));
    }

    StrictMock<VideoFrameCaptureServiceMock> m_captureServiceMock;
};

TEST_F(MediaPipelineServiceCaptureTests, shouldBindThenAttach)
{
    initSession();
    expectCaptureService();
    EXPECT_CALL(m_captureServiceMock, bindCapture(kCaptureClient, kCaptureId, kSession)).WillOnce(Return(true));
    EXPECT_CALL(m_mediaPipelineMock, attachCapture(kCaptureId)).WillOnce(Return(true));
    EXPECT_TRUE(m_sut->attachVideoFrameCapture(kSession, kCaptureClient, kCaptureId));
}

TEST_F(MediaPipelineServiceCaptureTests, shouldFailToAttachToMissingSession)
{
    initSession();
    EXPECT_FALSE(m_sut->attachVideoFrameCapture(kMissingSession, kCaptureClient, kCaptureId));
}

TEST_F(MediaPipelineServiceCaptureTests, shouldFailToAttachForeignOrBoundCapture)
{
    initSession();
    expectCaptureService();
    EXPECT_CALL(m_captureServiceMock, bindCapture(kCaptureClient, kCaptureId, kSession)).WillOnce(Return(false));
    EXPECT_FALSE(m_sut->attachVideoFrameCapture(kSession, kCaptureClient, kCaptureId)); // the session is not asked
}

TEST_F(MediaPipelineServiceCaptureTests, shouldUnbindWhenSessionRejectsAttach)
{
    initSession();
    expectCaptureService();
    EXPECT_CALL(m_captureServiceMock, bindCapture(kCaptureClient, kCaptureId, kSession)).WillOnce(Return(true));
    EXPECT_CALL(m_mediaPipelineMock, attachCapture(kCaptureId)).WillOnce(Return(false));
    EXPECT_CALL(m_captureServiceMock, unbindCapture(kCaptureClient, kCaptureId));
    EXPECT_FALSE(m_sut->attachVideoFrameCapture(kSession, kCaptureClient, kCaptureId));
}

TEST_F(MediaPipelineServiceCaptureTests, shouldDetachAndUnbind)
{
    initSession();
    expectCaptureService();
    EXPECT_CALL(m_captureServiceMock, bindCapture(kCaptureClient, kCaptureId, kSession)).WillOnce(Return(true));
    EXPECT_CALL(m_mediaPipelineMock, attachCapture(kCaptureId)).WillOnce(Return(true));
    ASSERT_TRUE(m_sut->attachVideoFrameCapture(kSession, kCaptureClient, kCaptureId));

    EXPECT_CALL(m_mediaPipelineMock, detachCapture(_)).WillOnce(DoAll(SetArgReferee<0>(kCaptureId), Return(true)));
    EXPECT_CALL(m_captureServiceMock, unbindCapture(kCaptureClient, kCaptureId));
    EXPECT_TRUE(m_sut->detachVideoFrameCapture(kSession));
}

TEST_F(MediaPipelineServiceCaptureTests, shouldKeepBindingWhenSessionRejectsDetach)
{
    initSession();
    EXPECT_CALL(m_mediaPipelineMock, detachCapture(_)).WillOnce(Return(false));
    EXPECT_FALSE(m_sut->detachVideoFrameCapture(kSession)); // nothing is unbound
}

TEST_F(MediaPipelineServiceCaptureTests, shouldFailToDetachFromMissingSession)
{
    initSession();
    EXPECT_FALSE(m_sut->detachVideoFrameCapture(kMissingSession));
}
