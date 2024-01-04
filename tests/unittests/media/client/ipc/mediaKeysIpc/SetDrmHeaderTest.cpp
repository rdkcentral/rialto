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
const std::vector<uint8_t> kDrmHeader{1, 2, 3};
} // namespace

class RialtoClientMediaKeysIpcSetDrmHeaderTest : public MediaKeysIpcTestBase
{
protected:
    RialtoClientMediaKeysIpcSetDrmHeaderTest() { createMediaKeysIpc(); }

    ~RialtoClientMediaKeysIpcSetDrmHeaderTest() { destroyMediaKeysIpc(); }

public:
    void setSetDrmHeaderResponseSuccess(google::protobuf::Message *response)
    {
        firebolt::rialto::SetDrmHeaderResponse *setDrmHeaderResponse =
            dynamic_cast<firebolt::rialto::SetDrmHeaderResponse *>(response);
        setDrmHeaderResponse->set_error_status(MediaKeysIpcTestBase::convertMediaKeyErrorStatus(MediaKeyErrorStatus::OK));
    }

    void setSetDrmHeaderResponseFailed(google::protobuf::Message *response)
    {
        firebolt::rialto::SetDrmHeaderResponse *setDrmHeaderResponse =
            dynamic_cast<firebolt::rialto::SetDrmHeaderResponse *>(response);
        setDrmHeaderResponse->set_error_status(MediaKeysIpcTestBase::convertMediaKeyErrorStatus(m_errorStatus));
    }
};

/**
 * Test setDrmHeader success.
 */
TEST_F(RialtoClientMediaKeysIpcSetDrmHeaderTest, Success)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setDrmHeader"), m_controllerMock.get(),
                                           setDrmHeaderRequestMatcher(m_mediaKeysHandle, m_kKeySessionId, kDrmHeader),
                                           _, m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcSetDrmHeaderTest::setSetDrmHeaderResponseSuccess)));

    EXPECT_EQ(m_mediaKeysIpc->setDrmHeader(m_kKeySessionId, kDrmHeader), MediaKeyErrorStatus::OK);
}

/**
 * Test that setDrmHeader fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaKeysIpcSetDrmHeaderTest, ChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_mediaKeysIpc->setDrmHeader(m_kKeySessionId, kDrmHeader), MediaKeyErrorStatus::FAIL);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that setDrmHeader fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaKeysIpcSetDrmHeaderTest, ReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setDrmHeader"), _, _, _, _))
        .WillOnce(WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcSetDrmHeaderTest::setSetDrmHeaderResponseSuccess)));

    EXPECT_EQ(m_mediaKeysIpc->setDrmHeader(m_kKeySessionId, kDrmHeader), MediaKeyErrorStatus::OK);
}

/**
 * Test that setDrmHeader fails when ipc fails.
 */
TEST_F(RialtoClientMediaKeysIpcSetDrmHeaderTest, Failure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setDrmHeader"), _, _, _, _));

    EXPECT_EQ(m_mediaKeysIpc->setDrmHeader(m_kKeySessionId, kDrmHeader), MediaKeyErrorStatus::FAIL);
}

/**
 * Test that setDrmHeader fails when api returns error.
 */
TEST_F(RialtoClientMediaKeysIpcSetDrmHeaderTest, ErrorReturn)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("setDrmHeader"), _, _, _, _))
        .WillOnce(WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcSetDrmHeaderTest::setSetDrmHeaderResponseFailed)));

    EXPECT_EQ(m_mediaKeysIpc->setDrmHeader(m_kKeySessionId, kDrmHeader), m_errorStatus);
}
