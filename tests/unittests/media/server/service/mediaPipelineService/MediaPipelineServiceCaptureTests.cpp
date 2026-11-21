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

#include "CaptureHalLibraryFactoryMock.h"
#include "CaptureHalLibraryMock.h"
#include "MediaPipelineServiceTestsFixture.h"
#include "VideoFrameCaptureService.h"
#include "VideoFrameCaptureServiceMock.h"

using firebolt::rialto::server::service::VideoFrameCaptureService;
using firebolt::rialto::server::service::VideoFrameCaptureServiceMock;
using firebolt::rialto::server::service::VideoFrameCaptureServiceStatus;
using firebolt::rialto::wrappers::CaptureHalLibraryFactoryMock;
using firebolt::rialto::wrappers::CaptureHalLibraryMock;
using ::testing::_;
using ::testing::DoAll;
using ::testing::NiceMock;
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

TEST_F(MediaPipelineServiceCaptureTests, shouldDropCaptureFromItsSessionOnly)
{
    initSession();
    expectCaptureService();
    EXPECT_CALL(m_captureServiceMock, bindCapture(kCaptureClient, kCaptureId, kSession)).WillOnce(Return(true));
    EXPECT_CALL(m_mediaPipelineMock, attachCapture(kCaptureId)).WillOnce(Return(true));
    ASSERT_TRUE(m_sut->attachVideoFrameCapture(kSession, kCaptureClient, kCaptureId));

    m_sut->dropVideoFrameCapture(kSession, kCaptureId + 1); // not attached here: nothing happens
    EXPECT_CALL(m_mediaPipelineMock, dropCapture(kCaptureId));
    m_sut->dropVideoFrameCapture(kSession, kCaptureId);
    m_sut->dropVideoFrameCapture(kSession, kCaptureId); // idempotent
}

class MediaPipelineServiceImplicitDetachTests : public MediaPipelineServiceTests
{
protected:
    MediaPipelineServiceImplicitDetachTests()
    {
        auto library = std::make_shared<NiceMock<CaptureHalLibraryMock>>();
        ON_CALL(*library, isAvailable()).WillByDefault(Return(true));
        auto factory = std::make_shared<NiceMock<CaptureHalLibraryFactoryMock>>();
        ON_CALL(*factory, getCaptureHalLibrary()).WillByDefault(Return(library));
        m_captureService = std::make_unique<VideoFrameCaptureService>(factory);
    }

    void attachRegisteredCapture()
    {
        EXPECT_CALL(m_playbackServiceMock, getVideoFrameCaptureService()).WillRepeatedly(ReturnRef(*m_captureService));
        ASSERT_EQ(m_captureService->createCapture(kCaptureClient, firebolt::rialto::GraphicsAPIType::GL, m_captureId),
                  VideoFrameCaptureServiceStatus::OK);
        EXPECT_CALL(m_mediaPipelineMock, attachCapture(m_captureId)).WillOnce(Return(true));
        ASSERT_TRUE(m_sut->attachVideoFrameCapture(kSession, kCaptureClient, m_captureId));
        EXPECT_FALSE(m_captureService->bindCapture(kCaptureClient, m_captureId, kMissingSession)); // bound
    }

    std::unique_ptr<VideoFrameCaptureService> m_captureService;
    std::uint32_t m_captureId{0};
};

TEST_F(MediaPipelineServiceImplicitDetachTests, destroyingThePipelineLeavesTheCaptureAttachable)
{
    initSession();
    attachRegisteredCapture();

    EXPECT_TRUE(m_sut->destroySession(kSession));
    EXPECT_TRUE(m_captureService->bindCapture(kCaptureClient, m_captureId, kMissingSession)); // free again
}

TEST_F(MediaPipelineServiceImplicitDetachTests, destroyingTheBoundCaptureDetachesItFromThePipeline)
{
    initSession();
    attachRegisteredCapture();
    m_captureService->setBoundCaptureRemovedHandler([this](int sessionId, std::uint32_t captureId)
                                                    { m_sut->dropVideoFrameCapture(sessionId, captureId); });

    EXPECT_CALL(m_mediaPipelineMock, dropCapture(m_captureId));
    EXPECT_TRUE(m_captureService->destroyCapture(kCaptureClient, m_captureId));

    // the session can take a new capture
    std::uint32_t newCaptureId{0};
    ASSERT_EQ(m_captureService->createCapture(kCaptureClient, firebolt::rialto::GraphicsAPIType::GL, newCaptureId),
              VideoFrameCaptureServiceStatus::OK);
    EXPECT_CALL(m_mediaPipelineMock, attachCapture(newCaptureId)).WillOnce(Return(true));
    EXPECT_TRUE(m_sut->attachVideoFrameCapture(kSession, kCaptureClient, newCaptureId));
}
