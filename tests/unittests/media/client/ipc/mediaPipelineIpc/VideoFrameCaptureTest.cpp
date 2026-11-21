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

#include "MediaPipelineIpcTestBase.h"
#include "mediapipelinemodule.pb.h"

using ::testing::WithArgs;

class RialtoClientMediaPipelineIpcVideoFrameCaptureTest : public MediaPipelineIpcTestBase
{
protected:
    static constexpr uint32_t kCaptureId{6};

    void SetUp() override
    {
        MediaPipelineIpcTestBase::SetUp();
        createMediaPipelineIpc();
    }

    void TearDown() override
    {
        destroyMediaPipelineIpc();
        MediaPipelineIpcTestBase::TearDown();
    }
};

TEST_F(RialtoClientMediaPipelineIpcVideoFrameCaptureTest, AttachSuccess)
{
    expectIpcApiCallSuccess();
    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("attachVideoFrameCapture"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<2>(Invoke(
            [this](const google::protobuf::Message *message)
            {
                auto *request = dynamic_cast<const firebolt::rialto::AttachVideoFrameCaptureRequest *>(message);
                ASSERT_NE(request, nullptr);
                EXPECT_EQ(request->session_id(), m_sessionId);
                EXPECT_EQ(request->capture_id(), kCaptureId);
            })));
    EXPECT_TRUE(m_mediaPipelineIpc->attachVideoFrameCapture(kCaptureId));
}

/**
 * The server refused, or it is an old server that does not implement the method: both fail the call.
 */
TEST_F(RialtoClientMediaPipelineIpcVideoFrameCaptureTest, AttachFailsWhenTheCallFails)
{
    expectIpcApiCallFailure();
    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("attachVideoFrameCapture"), _, _, _, _));
    EXPECT_FALSE(m_mediaPipelineIpc->attachVideoFrameCapture(kCaptureId));
}

TEST_F(RialtoClientMediaPipelineIpcVideoFrameCaptureTest, AttachFailsWhenDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();
    EXPECT_FALSE(m_mediaPipelineIpc->attachVideoFrameCapture(kCaptureId));

    // reattach the channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

TEST_F(RialtoClientMediaPipelineIpcVideoFrameCaptureTest, DetachSuccess)
{
    expectIpcApiCallSuccess();
    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("detachVideoFrameCapture"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<2>(Invoke(
            [this](const google::protobuf::Message *message)
            {
                auto *request = dynamic_cast<const firebolt::rialto::DetachVideoFrameCaptureRequest *>(message);
                ASSERT_NE(request, nullptr);
                EXPECT_EQ(request->session_id(), m_sessionId);
            })));
    EXPECT_TRUE(m_mediaPipelineIpc->detachVideoFrameCapture());
}

TEST_F(RialtoClientMediaPipelineIpcVideoFrameCaptureTest, DetachFailsWhenTheCallFails)
{
    expectIpcApiCallFailure();
    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("detachVideoFrameCapture"), _, _, _, _));
    EXPECT_FALSE(m_mediaPipelineIpc->detachVideoFrameCapture());
}
