/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Sky UK
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

MATCHER_P4(loadRequestMatcher, sessionId, type, mimeType, url, "")
{
    const ::firebolt::rialto::LoadRequest *kRequest = dynamic_cast<const ::firebolt::rialto::LoadRequest *>(arg);
    return ((kRequest->session_id() == sessionId) && (kRequest->type() == type) &&
            (kRequest->mime_type() == mimeType) && (kRequest->url() == url));
}

class RialtoClientMediaPipelineIpcLoadTest : public MediaPipelineIpcTestBase
{
protected:
    MediaType m_type = MediaType::MSE;
    firebolt::rialto::LoadRequest_MediaType m_protoType = firebolt::rialto::LoadRequest_MediaType_MSE;
    const std::string m_mimeType = "mime";
    const std::string m_url = "mse://1";

    virtual void SetUp()
    {
        MediaPipelineIpcTestBase::SetUp();

        createMediaPipelineIpc();
    }

    virtual void TearDown()
    {
        destroyMediaPipelineIpc();

        MediaPipelineIpcTestBase::TearDown();
    }
};

/**
 * Test that Load can be called successfully.
 */
TEST_F(RialtoClientMediaPipelineIpcLoadTest, Success)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("load"), m_controllerMock.get(),
                                           loadRequestMatcher(m_sessionId, m_protoType, m_mimeType, m_url), _,
                                           m_blockingClosureMock.get()));

    EXPECT_EQ(m_mediaPipelineIpc->load(m_type, m_mimeType, m_url), true);
}

/**
 * Test that Load fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcLoadTest, ChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_mediaPipelineIpc->load(m_type, m_mimeType, m_url), false);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that Load fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaPipelineIpcLoadTest, ReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("load"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->load(m_type, m_mimeType, m_url), true);
}

/**
 * Test that Load fails when ipc fails.
 */
TEST_F(RialtoClientMediaPipelineIpcLoadTest, LoadFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("load"), _, _, _, _));

    EXPECT_EQ(m_mediaPipelineIpc->load(m_type, m_mimeType, m_url), false);
}
