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

#include "MediaKeysIpcTestBase.h"
#include "MediaKeysProtoRequestMatcher.h"

class RialtoClientMediaKeysIpcLoadSessionTest : public MediaKeysIpcTestBase
{
protected:
    RialtoClientMediaKeysIpcLoadSessionTest() { createMediaKeysIpc(); }

    ~RialtoClientMediaKeysIpcLoadSessionTest() { destroyMediaKeysIpc(); }

public:
    void setLoadSessionResponseSuccess(google::protobuf::Message *response)
    {
        firebolt::rialto::LoadSessionResponse *loadSessionResponse =
            dynamic_cast<firebolt::rialto::LoadSessionResponse *>(response);
        loadSessionResponse->set_error_status(MediaKeysIpcTestBase::convertMediaKeyErrorStatus(MediaKeyErrorStatus::OK));
    }

    void setLoadSessionResponseFailed(google::protobuf::Message *response)
    {
        firebolt::rialto::LoadSessionResponse *loadSessionResponse =
            dynamic_cast<firebolt::rialto::LoadSessionResponse *>(response);
        loadSessionResponse->set_error_status(MediaKeysIpcTestBase::convertMediaKeyErrorStatus(m_errorStatus));
    }
};

/**
 * Test LoadSession success.
 */
TEST_F(RialtoClientMediaKeysIpcLoadSessionTest, Success)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("loadSession"), m_controllerMock.get(),
                                           loadSessionRequestMatcher(m_mediaKeysHandle, m_kKeySessionId), _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcLoadSessionTest::setLoadSessionResponseSuccess)));

    EXPECT_EQ(m_mediaKeysIpc->loadSession(m_kKeySessionId), MediaKeyErrorStatus::OK);
}

/**
 * Test that LoadSession fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaKeysIpcLoadSessionTest, ChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_mediaKeysIpc->loadSession(m_kKeySessionId), MediaKeyErrorStatus::FAIL);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that LoadSession fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaKeysIpcLoadSessionTest, ReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("loadSession"), _, _, _, _))
        .WillOnce(WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcLoadSessionTest::setLoadSessionResponseSuccess)));

    EXPECT_EQ(m_mediaKeysIpc->loadSession(m_kKeySessionId), MediaKeyErrorStatus::OK);
}

/**
 * Test that LoadSession fails when ipc fails.
 */
TEST_F(RialtoClientMediaKeysIpcLoadSessionTest, Failure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("loadSession"), _, _, _, _));

    EXPECT_EQ(m_mediaKeysIpc->loadSession(m_kKeySessionId), MediaKeyErrorStatus::FAIL);
}

/**
 * Test that LoadSession fails when api returns error.
 */
TEST_F(RialtoClientMediaKeysIpcLoadSessionTest, ErrorReturn)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("loadSession"), _, _, _, _))
        .WillOnce(WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcLoadSessionTest::setLoadSessionResponseFailed)));

    EXPECT_EQ(m_mediaKeysIpc->loadSession(m_kKeySessionId), m_errorStatus);
}
