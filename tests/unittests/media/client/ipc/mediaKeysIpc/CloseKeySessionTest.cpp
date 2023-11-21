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

MATCHER_P2(closeKeySessionRequestMatcher, mediaKeysHandle, keySessionId, "")
{
    const ::firebolt::rialto::CloseKeySessionRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::CloseKeySessionRequest *>(arg);
    return ((kRequest->media_keys_handle() == mediaKeysHandle) && (kRequest->key_session_id() == keySessionId));
}

class RialtoClientMediaKeysIpcCloseKeySessionTest : public MediaKeysIpcTestBase
{
protected:
    RialtoClientMediaKeysIpcCloseKeySessionTest()
    {
        createMediaKeysIpc();
        createKeySession();
    }

    ~RialtoClientMediaKeysIpcCloseKeySessionTest() { destroyMediaKeysIpc(); }

public:
    void setCloseKeySessionResponseSuccess(google::protobuf::Message *response)
    {
        firebolt::rialto::CloseKeySessionResponse *closeKeySessionResponse =
            dynamic_cast<firebolt::rialto::CloseKeySessionResponse *>(response);
        closeKeySessionResponse->set_error_status(
            MediaKeysIpcTestBase::convertMediaKeyErrorStatus(MediaKeyErrorStatus::OK));
    }

    void setCloseKeySessionResponseFailed(google::protobuf::Message *response)
    {
        firebolt::rialto::CloseKeySessionResponse *closeKeySessionResponse =
            dynamic_cast<firebolt::rialto::CloseKeySessionResponse *>(response);
        closeKeySessionResponse->set_error_status(MediaKeysIpcTestBase::convertMediaKeyErrorStatus(m_errorStatus));
    }
};

/**
 * Test CloseKeySession success.
 */
TEST_F(RialtoClientMediaKeysIpcCloseKeySessionTest, Success)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("closeKeySession"), m_controllerMock.get(),
                                           closeKeySessionRequestMatcher(m_mediaKeysHandle, m_kKeySessionId), _,
                                           m_blockingClosureMock.get()))
        .WillOnce(
            WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcCloseKeySessionTest::setCloseKeySessionResponseSuccess)));

    EXPECT_EQ(m_mediaKeysIpc->closeKeySession(m_kKeySessionId), MediaKeyErrorStatus::OK);

    // Check client object has been removed, no call to the client mock
    EXPECT_CALL(*m_eventThreadMock, addImpl(_)).WillOnce(Invoke([](std::function<void()> &&func) { func(); }));
    m_licenseRenewalCb(createLicenseRenewalEvent());
}

/**
 * Test that CloseKeySession fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaKeysIpcCloseKeySessionTest, ChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_mediaKeysIpc->closeKeySession(m_kKeySessionId), MediaKeyErrorStatus::FAIL);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that CloseKeySession fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaKeysIpcCloseKeySessionTest, ReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("closeKeySession"), _, _, _, _))
        .WillOnce(
            WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcCloseKeySessionTest::setCloseKeySessionResponseSuccess)));

    EXPECT_EQ(m_mediaKeysIpc->closeKeySession(m_kKeySessionId), MediaKeyErrorStatus::OK);
}

/**
 * Test that CloseKeySession fails when ipc fails.
 */
TEST_F(RialtoClientMediaKeysIpcCloseKeySessionTest, Failure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("closeKeySession"), _, _, _, _));

    EXPECT_EQ(m_mediaKeysIpc->closeKeySession(m_kKeySessionId), MediaKeyErrorStatus::FAIL);
}

/**
 * Test that CloseKeySession fails when api returns error.
 */
TEST_F(RialtoClientMediaKeysIpcCloseKeySessionTest, ErrorReturn)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("closeKeySession"), _, _, _, _))
        .WillOnce(
            WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcCloseKeySessionTest::setCloseKeySessionResponseFailed)));

    EXPECT_EQ(m_mediaKeysIpc->closeKeySession(m_kKeySessionId), m_errorStatus);
}
