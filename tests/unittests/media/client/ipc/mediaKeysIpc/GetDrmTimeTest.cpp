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

MATCHER_P(getDrmTimeRequestMatcher, mediaKeysHandle, "")
{
    const ::firebolt::rialto::GetDrmTimeRequest *request =
        dynamic_cast<const ::firebolt::rialto::GetDrmTimeRequest *>(arg);
    return (request->media_keys_handle() == mediaKeysHandle);
}

class RialtoClientMediaKeysIpcGetDrmTimeTest : public MediaKeysIpcTestBase
{
protected:
    RialtoClientMediaKeysIpcGetDrmTimeTest() { createMediaKeysIpc(); }

    ~RialtoClientMediaKeysIpcGetDrmTimeTest() { destroyMediaKeysIpc(); }

public:
    void setGetDrmTimeResponseSuccess(google::protobuf::Message *response)
    {
        firebolt::rialto::GetDrmTimeResponse *getDrmTimeResponse =
            dynamic_cast<firebolt::rialto::GetDrmTimeResponse *>(response);
        getDrmTimeResponse->set_error_status(MediaKeysIpcTestBase::convertMediaKeyErrorStatus(MediaKeyErrorStatus::OK));
    }

    void setGetDrmTimeResponseFailed(google::protobuf::Message *response)
    {
        firebolt::rialto::GetDrmTimeResponse *getDrmTimeResponse =
            dynamic_cast<firebolt::rialto::GetDrmTimeResponse *>(response);
        getDrmTimeResponse->set_error_status(MediaKeysIpcTestBase::convertMediaKeyErrorStatus(m_errorStatus));
    }
};

/**
 * Test getDrmTime success.
 */
TEST_F(RialtoClientMediaKeysIpcGetDrmTimeTest, Success)
{
    uint64_t drmTime;
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getDrmTime"), m_controllerMock.get(),
                                           getDrmTimeRequestMatcher(m_mediaKeysHandle), _, m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcGetDrmTimeTest::setGetDrmTimeResponseSuccess)));

    EXPECT_EQ(m_mediaKeysIpc->getDrmTime(drmTime), MediaKeyErrorStatus::OK);
}

/**
 * Test that getDrmTime fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaKeysIpcGetDrmTimeTest, ChannelDisconnected)
{
    uint64_t drmTime;
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_mediaKeysIpc->getDrmTime(drmTime), MediaKeyErrorStatus::FAIL);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that getDrmTime fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaKeysIpcGetDrmTimeTest, ReconnectChannel)
{
    uint64_t drmTime;
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getDrmTime"), _, _, _, _))
        .WillOnce(WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcGetDrmTimeTest::setGetDrmTimeResponseSuccess)));

    EXPECT_EQ(m_mediaKeysIpc->getDrmTime(drmTime), MediaKeyErrorStatus::OK);
}

/**
 * Test that getDrmTime fails when ipc fails.
 */
TEST_F(RialtoClientMediaKeysIpcGetDrmTimeTest, Failure)
{
    uint64_t drmTime;
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getDrmTime"), _, _, _, _));

    EXPECT_EQ(m_mediaKeysIpc->getDrmTime(drmTime), MediaKeyErrorStatus::FAIL);
}

/**
 * Test that getDrmTime fails when api returns error.
 */
TEST_F(RialtoClientMediaKeysIpcGetDrmTimeTest, ErrorReturn)
{
    uint64_t drmTime;
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getDrmTime"), _, _, _, _))
        .WillOnce(WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcGetDrmTimeTest::setGetDrmTimeResponseFailed)));

    EXPECT_EQ(m_mediaKeysIpc->getDrmTime(drmTime), m_errorStatus);
}
