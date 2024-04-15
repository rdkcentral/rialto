/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Sky UK
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

class RialtoClientMediaKeysIpcReleaseKeySessionTest : public MediaKeysIpcTestBase
{
protected:
    RialtoClientMediaKeysIpcReleaseKeySessionTest()
    {
        createMediaKeysIpc();
        createKeySession();
    }

    ~RialtoClientMediaKeysIpcReleaseKeySessionTest() { destroyMediaKeysIpc(); }

public:
    void setReleaseKeySessionResponseSuccess(google::protobuf::Message *response)
    {
        firebolt::rialto::ReleaseKeySessionResponse *releaseKeySessionResponse =
            dynamic_cast<firebolt::rialto::ReleaseKeySessionResponse *>(response);
        releaseKeySessionResponse->set_error_status(
            MediaKeysIpcTestBase::convertMediaKeyErrorStatus(MediaKeyErrorStatus::OK));
    }

    void setReleaseKeySessionResponseFailed(google::protobuf::Message *response)
    {
        firebolt::rialto::ReleaseKeySessionResponse *releaseKeySessionResponse =
            dynamic_cast<firebolt::rialto::ReleaseKeySessionResponse *>(response);
        releaseKeySessionResponse->set_error_status(MediaKeysIpcTestBase::convertMediaKeyErrorStatus(m_errorStatus));
    }
};

/**
 * Test ReleaseKeySession success.
 */
TEST_F(RialtoClientMediaKeysIpcReleaseKeySessionTest, Success)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("releaseKeySession"), m_controllerMock.get(),
                                           releaseKeySessionRequestMatcher(m_mediaKeysHandle, m_kKeySessionId), _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(
            Invoke(this, &RialtoClientMediaKeysIpcReleaseKeySessionTest::setReleaseKeySessionResponseSuccess)));

    EXPECT_EQ(m_mediaKeysIpc->releaseKeySession(m_kKeySessionId), MediaKeyErrorStatus::OK);

    // Check client object has been removed, no call to the client mock
    EXPECT_CALL(*m_eventThreadMock, addImpl(_)).WillOnce(Invoke([](std::function<void()> &&func) { func(); }));
    m_licenseRenewalCb(createLicenseRenewalEvent());
}

/**
 * Test that ReleaseKeySession fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaKeysIpcReleaseKeySessionTest, ChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_mediaKeysIpc->releaseKeySession(m_kKeySessionId), MediaKeyErrorStatus::FAIL);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that ReleaseKeySession fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaKeysIpcReleaseKeySessionTest, ReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("releaseKeySession"), _, _, _, _))
        .WillOnce(WithArgs<3>(
            Invoke(this, &RialtoClientMediaKeysIpcReleaseKeySessionTest::setReleaseKeySessionResponseSuccess)));

    EXPECT_EQ(m_mediaKeysIpc->releaseKeySession(m_kKeySessionId), MediaKeyErrorStatus::OK);
}

/**
 * Test that ReleaseKeySession fails when ipc fails.
 */
TEST_F(RialtoClientMediaKeysIpcReleaseKeySessionTest, Failure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("releaseKeySession"), _, _, _, _));

    EXPECT_EQ(m_mediaKeysIpc->releaseKeySession(m_kKeySessionId), MediaKeyErrorStatus::FAIL);
}

/**
 * Test that ReleaseKeySession fails when api returns error.
 */
TEST_F(RialtoClientMediaKeysIpcReleaseKeySessionTest, ErrorReturn)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("releaseKeySession"), _, _, _, _))
        .WillOnce(WithArgs<3>(
            Invoke(this, &RialtoClientMediaKeysIpcReleaseKeySessionTest::setReleaseKeySessionResponseFailed)));

    EXPECT_EQ(m_mediaKeysIpc->releaseKeySession(m_kKeySessionId), m_errorStatus);
}
