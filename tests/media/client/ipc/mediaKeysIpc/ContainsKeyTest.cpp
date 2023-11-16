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

namespace
{
const std::vector<uint8_t> kKeyId{1, 2, 3};
} // namespace

MATCHER_P3(containsKeyRequestMatcher, mediaKeysHandle, keySessionId, keyId, "")
{
    const ::firebolt::rialto::ContainsKeyRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::ContainsKeyRequest *>(arg);
    bool keyIdMatch{kKeyId.size() == static_cast<size_t>(kRequest->key_id().size())};
    if (keyIdMatch)
    {
        for (size_t i = 0; i < kKeyId.size(); ++i)
        {
            keyIdMatch &= kRequest->key_id(i) == keyId[i];
        }
    }
    return ((kRequest->media_keys_handle() == mediaKeysHandle) && (kRequest->key_session_id() == keySessionId) &&
            keyIdMatch);
}

class RialtoClientMediaKeysIpcContainsKeyTest : public MediaKeysIpcTestBase
{
protected:
    RialtoClientMediaKeysIpcContainsKeyTest() { createMediaKeysIpc(); }

    ~RialtoClientMediaKeysIpcContainsKeyTest() { destroyMediaKeysIpc(); }

public:
    void setContainsKeyResponseTrue(google::protobuf::Message *response)
    {
        firebolt::rialto::ContainsKeyResponse *containsKeyResponse =
            dynamic_cast<firebolt::rialto::ContainsKeyResponse *>(response);
        containsKeyResponse->set_contains_key(true);
    }

    void setContainsKeyResponseFalse(google::protobuf::Message *response)
    {
        firebolt::rialto::ContainsKeyResponse *containsKeyResponse =
            dynamic_cast<firebolt::rialto::ContainsKeyResponse *>(response);
        containsKeyResponse->set_contains_key(false);
    }
};

/**
 * Test containsKey success.
 */
TEST_F(RialtoClientMediaKeysIpcContainsKeyTest, Success)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("containsKey"), m_controllerMock.get(),
                                           containsKeyRequestMatcher(m_mediaKeysHandle, m_kKeySessionId, kKeyId), _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcContainsKeyTest::setContainsKeyResponseTrue)));

    EXPECT_TRUE(m_mediaKeysIpc->containsKey(m_kKeySessionId, kKeyId));
}

/**
 * Test that containsKey returns false if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaKeysIpcContainsKeyTest, ChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_FALSE(m_mediaKeysIpc->containsKey(m_kKeySessionId, kKeyId));

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that containsKey returns false if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaKeysIpcContainsKeyTest, ReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("containsKey"), _, _, _, _))
        .WillOnce(WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcContainsKeyTest::setContainsKeyResponseTrue)));

    EXPECT_TRUE(m_mediaKeysIpc->containsKey(m_kKeySessionId, kKeyId));
}

/**
 * Test that containsKey returns false when ipc fails.
 */
TEST_F(RialtoClientMediaKeysIpcContainsKeyTest, Failure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("containsKey"), _, _, _, _));

    EXPECT_FALSE(m_mediaKeysIpc->containsKey(m_kKeySessionId, kKeyId));
}

/**
 * Test that containsKey returns false when api returns false.
 */
TEST_F(RialtoClientMediaKeysIpcContainsKeyTest, ReturnFalse)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("containsKey"), _, _, _, _))
        .WillOnce(WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcContainsKeyTest::setContainsKeyResponseFalse)));

    EXPECT_FALSE(m_mediaKeysIpc->containsKey(m_kKeySessionId, kKeyId));
}
