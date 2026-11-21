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

#include "EglLibraryMock.h"
#include "GLVideoFrameCapture.h"
#include "IVideoFrameCapture.h"
#include "MediaPipelineTestBase.h"
#include "VideoFrameCaptureFactory.h"
#include "VideoFrameCaptureIpcMock.h"

using firebolt::rialto::GraphicsAPIType;
using firebolt::rialto::IVideoFrameCaptureFactory;
using firebolt::rialto::client::EglLibraryMock;
using firebolt::rialto::client::GLVideoFrameCapture;
using firebolt::rialto::client::VideoFrameCaptureFactory;
using firebolt::rialto::client::VideoFrameCaptureIpcFactoryMock;
using firebolt::rialto::client::VideoFrameCaptureIpcMock;

class RialtoClientVideoFrameCaptureTest : public MediaPipelineTestBase
{
protected:
    virtual void SetUp()
    {
        MediaPipelineTestBase::SetUp();

        createMediaPipeline();
    }

    virtual void TearDown()
    {
        destroyMediaPipeline();

        MediaPipelineTestBase::TearDown();
    }

    std::shared_ptr<StrictMock<EglLibraryMock>> m_eglLibraryMock{std::make_shared<StrictMock<EglLibraryMock>>()};
    std::shared_ptr<StrictMock<VideoFrameCaptureIpcFactoryMock>> m_ipcFactoryMock{
        std::make_shared<StrictMock<VideoFrameCaptureIpcFactoryMock>>()};
    std::shared_ptr<StrictMock<VideoFrameCaptureIpcMock>> m_ipcMock{
        std::make_shared<StrictMock<VideoFrameCaptureIpcMock>>()};
    VideoFrameCaptureFactory m_factory{m_eglLibraryMock, m_ipcFactoryMock};
};

/**
 * Test that the public factory exists.
 */
TEST_F(RialtoClientVideoFrameCaptureTest, CreateFactory)
{
    EXPECT_NE(IVideoFrameCaptureFactory::createFactory(), nullptr);
}

/**
 * Test that no capture is registered when EGL cannot be loaded.
 */
TEST_F(RialtoClientVideoFrameCaptureTest, NoCaptureWithoutEgl)
{
    EXPECT_CALL(*m_eglLibraryMock, load()).WillOnce(Return(false));
    EXPECT_EQ(m_factory.createGLVideoFrameCapture(nullptr, nullptr), nullptr);
}

/**
 * Test that no capture is created when the server cannot be reached.
 */
TEST_F(RialtoClientVideoFrameCaptureTest, NoCaptureWithoutServerConnection)
{
    EXPECT_CALL(*m_eglLibraryMock, load()).WillOnce(Return(true));
    EXPECT_CALL(*m_ipcFactoryMock, createVideoFrameCaptureIpc()).WillOnce(Return(nullptr));
    EXPECT_EQ(m_factory.createGLVideoFrameCapture(nullptr, nullptr), nullptr);
}

/**
 * Test that no capture is created when the server does not register it.
 */
TEST_F(RialtoClientVideoFrameCaptureTest, NoCaptureWhenServerRefuses)
{
    EXPECT_CALL(*m_eglLibraryMock, load()).WillOnce(Return(true));
    EXPECT_CALL(*m_ipcFactoryMock, createVideoFrameCaptureIpc()).WillOnce(Return(m_ipcMock));
    EXPECT_CALL(*m_ipcMock, createVideoFrameCapture(GraphicsAPIType::GL, _)).WillOnce(Return(false));
    EXPECT_EQ(m_factory.createGLVideoFrameCapture(nullptr, nullptr), nullptr);
}

/**
 * Test that a registered GL capture is returned, gives no frame yet, and is unregistered when dropped.
 */
TEST_F(RialtoClientVideoFrameCaptureTest, CreateAndDestroyGlCapture)
{
    constexpr std::uint32_t kCaptureId{7};
    EXPECT_CALL(*m_eglLibraryMock, load()).WillOnce(Return(true));
    EXPECT_CALL(*m_ipcFactoryMock, createVideoFrameCaptureIpc()).WillOnce(Return(m_ipcMock));
    EXPECT_CALL(*m_ipcMock, createVideoFrameCapture(GraphicsAPIType::GL, _))
        .WillOnce(DoAll(SetArgReferee<1>(kCaptureId), Return(true)));

    std::shared_ptr<firebolt::rialto::IGLVideoFrameCapture> capture =
        m_factory.createGLVideoFrameCapture(nullptr, nullptr);
    ASSERT_NE(capture, nullptr);
    EXPECT_EQ(capture->apiType(), GraphicsAPIType::GL);
    EXPECT_EQ(std::dynamic_pointer_cast<GLVideoFrameCapture>(capture)->captureId(), kCaptureId);

    firebolt::rialto::client::CaptureAcquireResult noFrame;
    noFrame.status = firebolt::rialto::client::CaptureAcquireStatus::NO_FRAME;
    EXPECT_CALL(*m_ipcMock, acquireCurrentFrame(kCaptureId)).WillOnce(Return(noFrame));
    const auto kAcquisition = capture->acquireCurrentFrame();
    EXPECT_FALSE(kAcquisition.changed);
    EXPECT_EQ(kAcquisition.frame, nullptr);

    EXPECT_CALL(*m_ipcMock, destroyVideoFrameCapture(kCaptureId)).WillOnce(Return(true));
    capture.reset();
}

/**
 * Test that attaching and detaching a capture fail on a pipeline, without any IPC call.
 */
TEST_F(RialtoClientVideoFrameCaptureTest, AttachAndDetachFail)
{
    EXPECT_FALSE(m_mediaPipeline->attachVideoFrameCapture(nullptr));
    EXPECT_FALSE(m_mediaPipeline->detachVideoFrameCapture());
}
