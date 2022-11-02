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

MATCHER_P3(updateSessionRequestMatcher, mediaKeysHandle, keySessionId, requestData, "")
{
    const ::firebolt::rialto::UpdateSessionRequest *request =
        dynamic_cast<const ::firebolt::rialto::UpdateSessionRequest *>(arg);
    return ((request->media_keys_handle() == mediaKeysHandle) && (request->key_session_id() == keySessionId) &&
            (std::vector<std::uint8_t>{request->response_data().begin(), request->response_data().end()} == requestData));
}

class RialtoClientMediaKeysIpcUpdateSessionTest : public MediaKeysIpcTestBase
{
protected:
    std::vector<std::uint8_t> m_requestData{54, 6, 1};

    RialtoClientMediaKeysIpcUpdateSessionTest() { createMediaKeysIpc(); }

    ~RialtoClientMediaKeysIpcUpdateSessionTest() { destroyMediaKeysIpc(); }

public:
    void setUpdateSessionResponseSuccess(google::protobuf::Message *response)
    {
        firebolt::rialto::UpdateSessionResponse *updateSessionResponse =
            dynamic_cast<firebolt::rialto::UpdateSessionResponse *>(response);
        updateSessionResponse->set_error_status(MediaKeysIpcTestBase::convertMediaKeyErrorStatus(MediaKeyErrorStatus::OK));
    }

    void setUpdateSessionResponseFailed(google::protobuf::Message *response)
    {
        firebolt::rialto::UpdateSessionResponse *updateSessionResponse =
            dynamic_cast<firebolt::rialto::UpdateSessionResponse *>(response);
        updateSessionResponse->set_error_status(MediaKeysIpcTestBase::convertMediaKeyErrorStatus(m_errorStatus));
    }
};

/**
 * Test UpdateSession success.
 */
TEST_F(RialtoClientMediaKeysIpcUpdateSessionTest, Success)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("updateSession"), m_controllerMock.get(),
                                           updateSessionRequestMatcher(m_mediaKeysHandle, m_keySessionId, m_requestData),
                                           _, m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcUpdateSessionTest::setUpdateSessionResponseSuccess)));

    EXPECT_EQ(m_mediaKeysIpc->updateSession(m_keySessionId, m_requestData), MediaKeyErrorStatus::OK);
}

/**
 * Test that UpdateSession fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaKeysIpcUpdateSessionTest, ChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_mediaKeysIpc->updateSession(m_keySessionId, m_requestData), MediaKeyErrorStatus::FAIL);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that UpdateSession fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaKeysIpcUpdateSessionTest, ReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("updateSession"), _, _, _, _))
        .WillOnce(WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcUpdateSessionTest::setUpdateSessionResponseSuccess)));

    EXPECT_EQ(m_mediaKeysIpc->updateSession(m_keySessionId, m_requestData), MediaKeyErrorStatus::OK);
}

/**
 * Test that UpdateSession fails when ipc fails.
 */
TEST_F(RialtoClientMediaKeysIpcUpdateSessionTest, Failure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("updateSession"), _, _, _, _));

    EXPECT_EQ(m_mediaKeysIpc->updateSession(m_keySessionId, m_requestData), MediaKeyErrorStatus::FAIL);
}

/**
 * Test that UpdateSession fails when api returns error.
 */
TEST_F(RialtoClientMediaKeysIpcUpdateSessionTest, ErrorReturn)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("updateSession"), _, _, _, _))
        .WillOnce(WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcUpdateSessionTest::setUpdateSessionResponseFailed)));

    EXPECT_EQ(m_mediaKeysIpc->updateSession(m_keySessionId, m_requestData), m_errorStatus);
}
