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

namespace
{
const std::string kCdmKeySessionId{"0"};
} // namespace

class RialtoClientMediaKeysIpcGetCdmKeySessionIdTest : public MediaKeysIpcTestBase
{
protected:
    RialtoClientMediaKeysIpcGetCdmKeySessionIdTest() { createMediaKeysIpc(); }

    ~RialtoClientMediaKeysIpcGetCdmKeySessionIdTest() { destroyMediaKeysIpc(); }

public:
    void setGetCdmKeySessionIdResponseSuccess(google::protobuf::Message *response)
    {
        firebolt::rialto::GetCdmKeySessionIdResponse *getCdmKeySessionIdResponse =
            dynamic_cast<firebolt::rialto::GetCdmKeySessionIdResponse *>(response);
        getCdmKeySessionIdResponse->set_error_status(
            MediaKeysIpcTestBase::convertMediaKeyErrorStatus(MediaKeyErrorStatus::OK));
        getCdmKeySessionIdResponse->set_cdm_key_session_id(kCdmKeySessionId);
    }

    void setGetCdmKeySessionIdResponseFailed(google::protobuf::Message *response)
    {
        firebolt::rialto::GetCdmKeySessionIdResponse *getCdmKeySessionIdResponse =
            dynamic_cast<firebolt::rialto::GetCdmKeySessionIdResponse *>(response);
        getCdmKeySessionIdResponse->set_error_status(MediaKeysIpcTestBase::convertMediaKeyErrorStatus(m_errorStatus));
        getCdmKeySessionIdResponse->set_cdm_key_session_id("");
    }
};

/**
 * Test GetCdmKeySessionId success.
 */
TEST_F(RialtoClientMediaKeysIpcGetCdmKeySessionIdTest, Success)
{
    std::string responseCdmKeySessionId;
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getCdmKeySessionId"), m_controllerMock.get(),
                                           getCdmKeySessionIdRequestMatcher(m_mediaKeysHandle, m_kKeySessionId), _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(
            Invoke(this, &RialtoClientMediaKeysIpcGetCdmKeySessionIdTest::setGetCdmKeySessionIdResponseSuccess)));

    EXPECT_EQ(m_mediaKeysIpc->getCdmKeySessionId(m_kKeySessionId, responseCdmKeySessionId), MediaKeyErrorStatus::OK);
    EXPECT_EQ(kCdmKeySessionId, responseCdmKeySessionId);
}

/**
 * Test that GetCdmKeySessionId fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaKeysIpcGetCdmKeySessionIdTest, ChannelDisconnected)
{
    std::string responseCdmKeySessionId;

    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_mediaKeysIpc->getCdmKeySessionId(m_kKeySessionId, responseCdmKeySessionId), MediaKeyErrorStatus::FAIL);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that GetCdmKeySessionId fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaKeysIpcGetCdmKeySessionIdTest, ReconnectChannel)
{
    std::string responseCdmKeySessionId;

    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getCdmKeySessionId"), m_controllerMock.get(),
                                           getCdmKeySessionIdRequestMatcher(m_mediaKeysHandle, m_kKeySessionId), _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(
            Invoke(this, &RialtoClientMediaKeysIpcGetCdmKeySessionIdTest::setGetCdmKeySessionIdResponseSuccess)));

    EXPECT_EQ(m_mediaKeysIpc->getCdmKeySessionId(m_kKeySessionId, responseCdmKeySessionId), MediaKeyErrorStatus::OK);
    EXPECT_EQ(kCdmKeySessionId, responseCdmKeySessionId);
}

/**
 * Test that GetCdmKeySessionId fails when ipc fails.
 */
TEST_F(RialtoClientMediaKeysIpcGetCdmKeySessionIdTest, Failure)
{
    std::string responseCdmKeySessionId;

    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getCdmKeySessionId"), _, _, _, _));

    EXPECT_EQ(m_mediaKeysIpc->getCdmKeySessionId(m_kKeySessionId, responseCdmKeySessionId), MediaKeyErrorStatus::FAIL);
}

/**
 * Test that GetCdmKeySessionId fails when api returns error.
 */
TEST_F(RialtoClientMediaKeysIpcGetCdmKeySessionIdTest, ErrorReturn)
{
    std::string responseCdmKeySessionId;

    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getCdmKeySessionId"), m_controllerMock.get(),
                                           getCdmKeySessionIdRequestMatcher(m_mediaKeysHandle, m_kKeySessionId), _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(
            Invoke(this, &RialtoClientMediaKeysIpcGetCdmKeySessionIdTest::setGetCdmKeySessionIdResponseFailed)));

    EXPECT_EQ(m_mediaKeysIpc->getCdmKeySessionId(m_kKeySessionId, responseCdmKeySessionId), m_errorStatus);
}
