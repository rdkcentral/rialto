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

MATCHER_P(getKeyStoreHashRequestMatcher, mediaKeysHandle, "")
{
    const ::firebolt::rialto::GetKeyStoreHashRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::GetKeyStoreHashRequest *>(arg);
    return (kRequest->media_keys_handle() == mediaKeysHandle);
}

class RialtoClientMediaKeysIpcGetKeyStoreHashTest : public MediaKeysIpcTestBase
{
protected:
    RialtoClientMediaKeysIpcGetKeyStoreHashTest() { createMediaKeysIpc(); }

    ~RialtoClientMediaKeysIpcGetKeyStoreHashTest() { destroyMediaKeysIpc(); }

public:
    void setGetKeyStoreHashResponseSuccess(google::protobuf::Message *response)
    {
        firebolt::rialto::GetKeyStoreHashResponse *getKeyStoreHashResponse =
            dynamic_cast<firebolt::rialto::GetKeyStoreHashResponse *>(response);
        getKeyStoreHashResponse->set_error_status(
            MediaKeysIpcTestBase::convertMediaKeyErrorStatus(MediaKeyErrorStatus::OK));
    }

    void setGetKeyStoreHashResponseFailed(google::protobuf::Message *response)
    {
        firebolt::rialto::GetKeyStoreHashResponse *getKeyStoreHashResponse =
            dynamic_cast<firebolt::rialto::GetKeyStoreHashResponse *>(response);
        getKeyStoreHashResponse->set_error_status(MediaKeysIpcTestBase::convertMediaKeyErrorStatus(m_errorStatus));
    }
};

/**
 * Test getKeyStoreHash success.
 */
TEST_F(RialtoClientMediaKeysIpcGetKeyStoreHashTest, Success)
{
    std::vector<unsigned char> drmStoreHash;
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("getKeyStoreHash"), m_controllerMock.get(),
                           getKeyStoreHashRequestMatcher(m_mediaKeysHandle), _, m_blockingClosureMock.get()))
        .WillOnce(
            WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcGetKeyStoreHashTest::setGetKeyStoreHashResponseSuccess)));

    EXPECT_EQ(m_mediaKeysIpc->getKeyStoreHash(drmStoreHash), MediaKeyErrorStatus::OK);
}

/**
 * Test that getKeyStoreHash fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaKeysIpcGetKeyStoreHashTest, ChannelDisconnected)
{
    std::vector<unsigned char> drmStoreHash;
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_mediaKeysIpc->getKeyStoreHash(drmStoreHash), MediaKeyErrorStatus::FAIL);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that getKeyStoreHash fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaKeysIpcGetKeyStoreHashTest, ReconnectChannel)
{
    std::vector<unsigned char> drmStoreHash;
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getKeyStoreHash"), _, _, _, _))
        .WillOnce(
            WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcGetKeyStoreHashTest::setGetKeyStoreHashResponseSuccess)));

    EXPECT_EQ(m_mediaKeysIpc->getKeyStoreHash(drmStoreHash), MediaKeyErrorStatus::OK);
}

/**
 * Test that getKeyStoreHash fails when ipc fails.
 */
TEST_F(RialtoClientMediaKeysIpcGetKeyStoreHashTest, Failure)
{
    std::vector<unsigned char> drmStoreHash;
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getKeyStoreHash"), _, _, _, _));

    EXPECT_EQ(m_mediaKeysIpc->getKeyStoreHash(drmStoreHash), MediaKeyErrorStatus::FAIL);
}

/**
 * Test that getKeyStoreHash fails when api returns error.
 */
TEST_F(RialtoClientMediaKeysIpcGetKeyStoreHashTest, ErrorReturn)
{
    std::vector<unsigned char> drmStoreHash;
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getKeyStoreHash"), _, _, _, _))
        .WillOnce(
            WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcGetKeyStoreHashTest::setGetKeyStoreHashResponseFailed)));

    EXPECT_EQ(m_mediaKeysIpc->getKeyStoreHash(drmStoreHash), m_errorStatus);
}
