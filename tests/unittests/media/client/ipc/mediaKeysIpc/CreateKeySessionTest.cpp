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
#include "MediaKeysProtoUtils.h"

class RialtoClientMediaKeysIpcCreateKeySessionTest : public MediaKeysIpcTestBase
{
protected:
    KeySessionType m_keySessionType = KeySessionType::PERSISTENT_LICENCE;

    RialtoClientMediaKeysIpcCreateKeySessionTest() { createMediaKeysIpc(); }

    ~RialtoClientMediaKeysIpcCreateKeySessionTest() { destroyMediaKeysIpc(); }

public:
    void setCreateKeySessionResponseFailed(google::protobuf::Message *response)
    {
        firebolt::rialto::CreateKeySessionResponse *createKeySessionResponse =
            dynamic_cast<firebolt::rialto::CreateKeySessionResponse *>(response);
        createKeySessionResponse->set_key_session_id(m_kKeySessionId);
        createKeySessionResponse->set_error_status(MediaKeysIpcTestBase::convertMediaKeyErrorStatus(m_errorStatus));
    }
};

/**
 * Test CreateKeySession success.
 */
TEST_F(RialtoClientMediaKeysIpcCreateKeySessionTest, Success)
{
    int32_t returnKeySessionid;
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("createKeySession"), m_controllerMock.get(),
                           createKeySessionRequestMatcher(m_mediaKeysHandle, convertKeySessionType(m_keySessionType)),
                           _, m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(
            Invoke(this, &RialtoClientMediaKeysIpcCreateKeySessionTest::setCreateKeySessionResponseSuccess)));

    EXPECT_EQ(m_mediaKeysIpc->createKeySession(m_keySessionType, m_mediaKeysClientMock, returnKeySessionid),
              MediaKeyErrorStatus::OK);
    EXPECT_EQ(returnKeySessionid, m_kKeySessionId);

    // Check client object has been stored
    EXPECT_CALL(*m_eventThreadMock, addImpl(_)).WillOnce(Invoke([](std::function<void()> &&func) { func(); }));
    EXPECT_CALL(*m_mediaKeysClientMock, onLicenseRenewal(_, _));
    m_licenseRenewalCb(createLicenseRenewalEvent());
}

/**
 * Test that CreateKeySession fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaKeysIpcCreateKeySessionTest, ChannelDisconnected)
{
    int32_t returnKeySessionid;
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_mediaKeysIpc->createKeySession(m_keySessionType, m_mediaKeysClientMock, returnKeySessionid),
              MediaKeyErrorStatus::FAIL);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that CreateKeySession fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaKeysIpcCreateKeySessionTest, ReconnectChannel)
{
    int32_t returnKeySessionid;
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("createKeySession"), _, _, _, _))
        .WillOnce(WithArgs<3>(
            Invoke(this, &RialtoClientMediaKeysIpcCreateKeySessionTest::setCreateKeySessionResponseSuccess)));

    EXPECT_EQ(m_mediaKeysIpc->createKeySession(m_keySessionType, m_mediaKeysClientMock, returnKeySessionid),
              MediaKeyErrorStatus::OK);
}

/**
 * Test that CreateKeySession fails when ipc fails.
 */
TEST_F(RialtoClientMediaKeysIpcCreateKeySessionTest, Failure)
{
    int32_t returnKeySessionid;
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("createKeySession"), _, _, _, _));

    EXPECT_EQ(m_mediaKeysIpc->createKeySession(m_keySessionType, m_mediaKeysClientMock, returnKeySessionid),
              MediaKeyErrorStatus::FAIL);
}

/**
 * Test that CreateKeySession fails when api returns error.
 */
TEST_F(RialtoClientMediaKeysIpcCreateKeySessionTest, ErrorReturn)
{
    int32_t returnKeySessionid;
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("createKeySession"), _, _, _, _))
        .WillOnce(
            WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcCreateKeySessionTest::setCreateKeySessionResponseFailed)));

    EXPECT_EQ(m_mediaKeysIpc->createKeySession(m_keySessionType, m_mediaKeysClientMock, returnKeySessionid),
              m_errorStatus);
}
