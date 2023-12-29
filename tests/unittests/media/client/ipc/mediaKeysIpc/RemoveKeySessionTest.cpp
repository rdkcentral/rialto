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
#include "MediaKeysProtoRequestMatchers.h"

class RialtoClientMediaKeysIpcRemoveKeySessionTest : public MediaKeysIpcTestBase
{
protected:
    RialtoClientMediaKeysIpcRemoveKeySessionTest() { createMediaKeysIpc(); }

    ~RialtoClientMediaKeysIpcRemoveKeySessionTest() { destroyMediaKeysIpc(); }

public:
    void setRemoveKeySessionResponseSuccess(google::protobuf::Message *response)
    {
        firebolt::rialto::RemoveKeySessionResponse *removeKeySessionResponse =
            dynamic_cast<firebolt::rialto::RemoveKeySessionResponse *>(response);
        removeKeySessionResponse->set_error_status(
            MediaKeysIpcTestBase::convertMediaKeyErrorStatus(MediaKeyErrorStatus::OK));
    }

    void setRemoveKeySessionResponseFailed(google::protobuf::Message *response)
    {
        firebolt::rialto::RemoveKeySessionResponse *removeKeySessionResponse =
            dynamic_cast<firebolt::rialto::RemoveKeySessionResponse *>(response);
        removeKeySessionResponse->set_error_status(MediaKeysIpcTestBase::convertMediaKeyErrorStatus(m_errorStatus));
    }
};

/**
 * Test RemoveKeySession success.
 */
TEST_F(RialtoClientMediaKeysIpcRemoveKeySessionTest, Success)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("removeKeySession"), m_controllerMock.get(),
                                           removeKeySessionRequestMatcher(m_mediaKeysHandle, m_kKeySessionId), _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(
            Invoke(this, &RialtoClientMediaKeysIpcRemoveKeySessionTest::setRemoveKeySessionResponseSuccess)));

    EXPECT_EQ(m_mediaKeysIpc->removeKeySession(m_kKeySessionId), MediaKeyErrorStatus::OK);
}

/**
 * Test that RemoveKeySession fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaKeysIpcRemoveKeySessionTest, ChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_mediaKeysIpc->removeKeySession(m_kKeySessionId), MediaKeyErrorStatus::FAIL);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that RemoveKeySession fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaKeysIpcRemoveKeySessionTest, ReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("removeKeySession"), _, _, _, _))
        .WillOnce(WithArgs<3>(
            Invoke(this, &RialtoClientMediaKeysIpcRemoveKeySessionTest::setRemoveKeySessionResponseSuccess)));

    EXPECT_EQ(m_mediaKeysIpc->removeKeySession(m_kKeySessionId), MediaKeyErrorStatus::OK);
}

/**
 * Test that RemoveKeySession fails when ipc fails.
 */
TEST_F(RialtoClientMediaKeysIpcRemoveKeySessionTest, Failure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("removeKeySession"), _, _, _, _));

    EXPECT_EQ(m_mediaKeysIpc->removeKeySession(m_kKeySessionId), MediaKeyErrorStatus::FAIL);
}

/**
 * Test that RemoveKeySession fails when api returns error.
 */
TEST_F(RialtoClientMediaKeysIpcRemoveKeySessionTest, ErrorReturn)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("removeKeySession"), _, _, _, _))
        .WillOnce(
            WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcRemoveKeySessionTest::setRemoveKeySessionResponseFailed)));

    EXPECT_EQ(m_mediaKeysIpc->removeKeySession(m_kKeySessionId), m_errorStatus);
}
