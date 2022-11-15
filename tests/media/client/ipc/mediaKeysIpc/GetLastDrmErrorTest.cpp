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

MATCHER_P2(getLastDrmErrorRequestMatcher, mediaKeysHandle, keySessionId, "")
{
    const ::firebolt::rialto::GetLastDrmErrorRequest *request =
        dynamic_cast<const ::firebolt::rialto::GetLastDrmErrorRequest *>(arg);
    return ((request->media_keys_handle() == mediaKeysHandle) && (request->key_session_id() == keySessionId));
}

class RialtoClientMediaKeysIpcGetLastDrmErrorTest : public MediaKeysIpcTestBase
{
protected:
    RialtoClientMediaKeysIpcGetLastDrmErrorTest() { createMediaKeysIpc(); }

    ~RialtoClientMediaKeysIpcGetLastDrmErrorTest() { destroyMediaKeysIpc(); }

public:
    void setGetLastDrmErrorResponseSuccess(google::protobuf::Message *response)
    {
        firebolt::rialto::GetLastDrmErrorResponse *getLastDrmErrorResponse =
            dynamic_cast<firebolt::rialto::GetLastDrmErrorResponse *>(response);
        getLastDrmErrorResponse->set_error_status(
            MediaKeysIpcTestBase::convertMediaKeyErrorStatus(MediaKeyErrorStatus::OK));
    }

    void setGetLastDrmErrorResponseFailed(google::protobuf::Message *response)
    {
        firebolt::rialto::GetLastDrmErrorResponse *getLastDrmErrorResponse =
            dynamic_cast<firebolt::rialto::GetLastDrmErrorResponse *>(response);
        getLastDrmErrorResponse->set_error_status(MediaKeysIpcTestBase::convertMediaKeyErrorStatus(m_errorStatus));
    }
};

/**
 * Test getLastDrmError success.
 */
TEST_F(RialtoClientMediaKeysIpcGetLastDrmErrorTest, Success)
{
    std::uint32_t lastDrmError;
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getLastDrmError"), m_controllerMock.get(),
                                           getLastDrmErrorRequestMatcher(m_mediaKeysHandle, m_keySessionId), _,
                                           m_blockingClosureMock.get()))
        .WillOnce(
            WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcGetLastDrmErrorTest::setGetLastDrmErrorResponseSuccess)));

    EXPECT_EQ(m_mediaKeysIpc->getLastDrmError(m_keySessionId, lastDrmError), MediaKeyErrorStatus::OK);
}

/**
 * Test that getLastDrmError fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaKeysIpcGetLastDrmErrorTest, ChannelDisconnected)
{
    std::uint32_t lastDrmError;
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_mediaKeysIpc->getLastDrmError(m_keySessionId, lastDrmError), MediaKeyErrorStatus::FAIL);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that getLastDrmError fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaKeysIpcGetLastDrmErrorTest, ReconnectChannel)
{
    std::uint32_t lastDrmError;
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getLastDrmError"), _, _, _, _))
        .WillOnce(
            WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcGetLastDrmErrorTest::setGetLastDrmErrorResponseSuccess)));

    EXPECT_EQ(m_mediaKeysIpc->getLastDrmError(m_keySessionId, lastDrmError), MediaKeyErrorStatus::OK);
}

/**
 * Test that getLastDrmError fails when ipc fails.
 */
TEST_F(RialtoClientMediaKeysIpcGetLastDrmErrorTest, Failure)
{
    std::uint32_t lastDrmError;
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getLastDrmError"), _, _, _, _));

    EXPECT_EQ(m_mediaKeysIpc->getLastDrmError(m_keySessionId, lastDrmError), MediaKeyErrorStatus::FAIL);
}

/**
 * Test that getLastDrmError fails when api returns error.
 */
TEST_F(RialtoClientMediaKeysIpcGetLastDrmErrorTest, ErrorReturn)
{
    std::uint32_t lastDrmError;
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getLastDrmError"), _, _, _, _))
        .WillOnce(
            WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcGetLastDrmErrorTest::setGetLastDrmErrorResponseFailed)));

    EXPECT_EQ(m_mediaKeysIpc->getLastDrmError(m_keySessionId, lastDrmError), m_errorStatus);
}
