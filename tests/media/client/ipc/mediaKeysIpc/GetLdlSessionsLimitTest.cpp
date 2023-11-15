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

MATCHER_P(getLdlSessionsLimitRequestMatcher, mediaKeysHandle, "")
{
    const ::firebolt::rialto::GetLdlSessionsLimitRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::GetLdlSessionsLimitRequest *>(arg);
    return (kRequest->media_keys_handle() == mediaKeysHandle);
}

class RialtoClientMediaKeysIpcGetLdlSessionsLimitTest : public MediaKeysIpcTestBase
{
protected:
    RialtoClientMediaKeysIpcGetLdlSessionsLimitTest() { createMediaKeysIpc(); }

    ~RialtoClientMediaKeysIpcGetLdlSessionsLimitTest() { destroyMediaKeysIpc(); }

public:
    void setGetLdlSessionsLimitResponseSuccess(google::protobuf::Message *response)
    {
        firebolt::rialto::GetLdlSessionsLimitResponse *getLdlSessionsLimitResponse =
            dynamic_cast<firebolt::rialto::GetLdlSessionsLimitResponse *>(response);
        getLdlSessionsLimitResponse->set_error_status(
            MediaKeysIpcTestBase::convertMediaKeyErrorStatus(MediaKeyErrorStatus::OK));
    }

    void setGetLdlSessionsLimitResponseFailed(google::protobuf::Message *response)
    {
        firebolt::rialto::GetLdlSessionsLimitResponse *getLdlSessionsLimitResponse =
            dynamic_cast<firebolt::rialto::GetLdlSessionsLimitResponse *>(response);
        getLdlSessionsLimitResponse->set_error_status(MediaKeysIpcTestBase::convertMediaKeyErrorStatus(m_errorStatus));
    }
};

/**
 * Test getLdlSessionsLimit success.
 */
TEST_F(RialtoClientMediaKeysIpcGetLdlSessionsLimitTest, Success)
{
    std::uint32_t ldlSessionsLimit;
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("getLdlSessionsLimit"), m_controllerMock.get(),
                           getLdlSessionsLimitRequestMatcher(m_mediaKeysHandle), _, m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(
            Invoke(this, &RialtoClientMediaKeysIpcGetLdlSessionsLimitTest::setGetLdlSessionsLimitResponseSuccess)));

    EXPECT_EQ(m_mediaKeysIpc->getLdlSessionsLimit(ldlSessionsLimit), MediaKeyErrorStatus::OK);
}

/**
 * Test that getLdlSessionsLimit fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaKeysIpcGetLdlSessionsLimitTest, ChannelDisconnected)
{
    std::uint32_t ldlSessionsLimit;
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_mediaKeysIpc->getLdlSessionsLimit(ldlSessionsLimit), MediaKeyErrorStatus::FAIL);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that getLdlSessionsLimit fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaKeysIpcGetLdlSessionsLimitTest, ReconnectChannel)
{
    std::uint32_t ldlSessionsLimit;
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getLdlSessionsLimit"), _, _, _, _))
        .WillOnce(WithArgs<3>(
            Invoke(this, &RialtoClientMediaKeysIpcGetLdlSessionsLimitTest::setGetLdlSessionsLimitResponseSuccess)));

    EXPECT_EQ(m_mediaKeysIpc->getLdlSessionsLimit(ldlSessionsLimit), MediaKeyErrorStatus::OK);
}

/**
 * Test that getLdlSessionsLimit fails when ipc fails.
 */
TEST_F(RialtoClientMediaKeysIpcGetLdlSessionsLimitTest, Failure)
{
    std::uint32_t ldlSessionsLimit;
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getLdlSessionsLimit"), _, _, _, _));

    EXPECT_EQ(m_mediaKeysIpc->getLdlSessionsLimit(ldlSessionsLimit), MediaKeyErrorStatus::FAIL);
}

/**
 * Test that getLdlSessionsLimit fails when api returns error.
 */
TEST_F(RialtoClientMediaKeysIpcGetLdlSessionsLimitTest, ErrorReturn)
{
    std::uint32_t ldlSessionsLimit;
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getLdlSessionsLimit"), _, _, _, _))
        .WillOnce(WithArgs<3>(
            Invoke(this, &RialtoClientMediaKeysIpcGetLdlSessionsLimitTest::setGetLdlSessionsLimitResponseFailed)));

    EXPECT_EQ(m_mediaKeysIpc->getLdlSessionsLimit(ldlSessionsLimit), m_errorStatus);
}
