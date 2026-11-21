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

#include "VideoFrameCaptureIpc.h"
#include "IpcModuleBase.h"
#include <gtest/gtest.h>

using ::testing::Invoke;
using ::testing::Return;
using ::testing::WithArgs;

class VideoFrameCaptureIpcTest : public IpcModuleBase, public ::testing::Test
{
protected:
    std::unique_ptr<VideoFrameCaptureIpc> m_sut;

    void createVideoFrameCaptureIpc()
    {
        expectInitIpc();
        EXPECT_NO_THROW(m_sut = std::make_unique<VideoFrameCaptureIpc>(*m_ipcClientMock));
    }

public:
    void setCreateResponse(google::protobuf::Message *response, firebolt::rialto::VideoFrameCaptureStatus status,
                           bool withId)
    {
        auto *createResponse = dynamic_cast<firebolt::rialto::CreateVideoFrameCaptureResponse *>(response);
        createResponse->set_status(status);
        if (withId)
            createResponse->set_capture_id(7);
    }
};

TEST_F(VideoFrameCaptureIpcTest, ShouldThrowWhenChannelCannotBeAttached)
{
    expectInitIpcButAttachChannelFailure();
    EXPECT_THROW(m_sut = std::make_unique<VideoFrameCaptureIpc>(*m_ipcClientMock), std::runtime_error);
}

TEST_F(VideoFrameCaptureIpcTest, ShouldMapSuccessToCaptureId)
{
    createVideoFrameCaptureIpc();
    expectIpcApiCallSuccess();
    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("createVideoFrameCapture"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(
            Invoke([this](google::protobuf::Message *response)
                   { setCreateResponse(response, firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_OK, true); })));

    std::uint32_t captureId{0};
    EXPECT_TRUE(m_sut->createVideoFrameCapture(firebolt::rialto::GraphicsAPIType::GL, captureId));
    EXPECT_EQ(captureId, 7u);
}

TEST_F(VideoFrameCaptureIpcTest, ShouldMapUnsupportedToFailure)
{
    createVideoFrameCaptureIpc();
    expectIpcApiCallSuccess();
    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("createVideoFrameCapture"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(
            Invoke([this](google::protobuf::Message *response)
                   { setCreateResponse(response, firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_UNSUPPORTED, false); })));

    std::uint32_t captureId{0};
    EXPECT_FALSE(m_sut->createVideoFrameCapture(firebolt::rialto::GraphicsAPIType::GL, captureId));
}

TEST_F(VideoFrameCaptureIpcTest, ShouldMapFailedCallToFailure)
{
    createVideoFrameCaptureIpc();
    expectIpcApiCallFailure();
    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("createVideoFrameCapture"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()));

    std::uint32_t captureId{0};
    EXPECT_FALSE(m_sut->createVideoFrameCapture(firebolt::rialto::GraphicsAPIType::GL, captureId));
}

TEST_F(VideoFrameCaptureIpcTest, ShouldFailCreateWhenDisconnected)
{
    createVideoFrameCaptureIpc();
    expectIpcApiCallDisconnected();

    std::uint32_t captureId{0};
    EXPECT_FALSE(m_sut->createVideoFrameCapture(firebolt::rialto::GraphicsAPIType::GL, captureId));
}

TEST_F(VideoFrameCaptureIpcTest, ShouldDestroy)
{
    createVideoFrameCaptureIpc();
    expectIpcApiCallSuccess();
    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("destroyVideoFrameCapture"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()));
    EXPECT_TRUE(m_sut->destroyVideoFrameCapture(7));
}

TEST_F(VideoFrameCaptureIpcTest, ShouldFailDestroyWhenCallFails)
{
    createVideoFrameCaptureIpc();
    expectIpcApiCallFailure();
    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("destroyVideoFrameCapture"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()));
    EXPECT_FALSE(m_sut->destroyVideoFrameCapture(7));
}
