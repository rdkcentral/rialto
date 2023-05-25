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

MATCHER_P(getDrmStoreHashRequestMatcher, mediaKeysHandle, "")
{
    const ::firebolt::rialto::GetDrmStoreHashRequest *request =
        dynamic_cast<const ::firebolt::rialto::GetDrmStoreHashRequest *>(arg);
    return (request->media_keys_handle() == mediaKeysHandle);
}

class RialtoClientMediaKeysIpcGetDrmStoreHashTest : public MediaKeysIpcTestBase
{
protected:
    RialtoClientMediaKeysIpcGetDrmStoreHashTest() { createMediaKeysIpc(); }

    ~RialtoClientMediaKeysIpcGetDrmStoreHashTest() { destroyMediaKeysIpc(); }

public:
    void setGetDrmStoreHashResponseSuccess(google::protobuf::Message *response)
    {
        firebolt::rialto::GetDrmStoreHashResponse *getDrmStoreHashResponse =
            dynamic_cast<firebolt::rialto::GetDrmStoreHashResponse *>(response);
        getDrmStoreHashResponse->set_error_status(
            MediaKeysIpcTestBase::convertMediaKeyErrorStatus(MediaKeyErrorStatus::OK));
    }

    void setGetDrmStoreHashResponseFailed(google::protobuf::Message *response)
    {
        firebolt::rialto::GetDrmStoreHashResponse *getDrmStoreHashResponse =
            dynamic_cast<firebolt::rialto::GetDrmStoreHashResponse *>(response);
        getDrmStoreHashResponse->set_error_status(MediaKeysIpcTestBase::convertMediaKeyErrorStatus(m_errorStatus));
    }
};

/**
 * Test getDrmStoreHash success.
 */
TEST_F(RialtoClientMediaKeysIpcGetDrmStoreHashTest, Success)
{
    std::vector<unsigned char> drmStoreHash;
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("getDrmStoreHash"), m_controllerMock.get(),
                           getDrmStoreHashRequestMatcher(m_mediaKeysHandle), _, m_blockingClosureMock.get()))
        .WillOnce(
            WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcGetDrmStoreHashTest::setGetDrmStoreHashResponseSuccess)));

    EXPECT_EQ(m_mediaKeysIpc->getDrmStoreHash(drmStoreHash), MediaKeyErrorStatus::OK);
}

/**
 * Test that getDrmStoreHash fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaKeysIpcGetDrmStoreHashTest, ChannelDisconnected)
{
    std::vector<unsigned char> drmStoreHash;
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_mediaKeysIpc->getDrmStoreHash(drmStoreHash), MediaKeyErrorStatus::FAIL);

    // Reattach channel on destroySession
    EXPECT_CALL(m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that getDrmStoreHash fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaKeysIpcGetDrmStoreHashTest, ReconnectChannel)
{
    std::vector<unsigned char> drmStoreHash;
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getDrmStoreHash"), _, _, _, _))
        .WillOnce(
            WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcGetDrmStoreHashTest::setGetDrmStoreHashResponseSuccess)));

    EXPECT_EQ(m_mediaKeysIpc->getDrmStoreHash(drmStoreHash), MediaKeyErrorStatus::OK);
}

/**
 * Test that getDrmStoreHash fails when ipc fails.
 */
TEST_F(RialtoClientMediaKeysIpcGetDrmStoreHashTest, Failure)
{
    std::vector<unsigned char> drmStoreHash;
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getDrmStoreHash"), _, _, _, _));

    EXPECT_EQ(m_mediaKeysIpc->getDrmStoreHash(drmStoreHash), MediaKeyErrorStatus::FAIL);
}

/**
 * Test that getDrmStoreHash fails when api returns error.
 */
TEST_F(RialtoClientMediaKeysIpcGetDrmStoreHashTest, ErrorReturn)
{
    std::vector<unsigned char> drmStoreHash;
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getDrmStoreHash"), _, _, _, _))
        .WillOnce(
            WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcGetDrmStoreHashTest::setGetDrmStoreHashResponseFailed)));

    EXPECT_EQ(m_mediaKeysIpc->getDrmStoreHash(drmStoreHash), m_errorStatus);
}
