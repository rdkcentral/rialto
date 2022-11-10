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

MATCHER_P3(selectKeyIdRequestMatcher, mediaKeysHandle, keySessionId, keyId, "")
{
    const ::firebolt::rialto::SelectKeyIdRequest *request =
        dynamic_cast<const ::firebolt::rialto::SelectKeyIdRequest *>(arg);
    bool keyIdMatch{kKeyId.size() == static_cast<size_t>(request->key_id().size())};
    if (keyIdMatch)
    {
        for (size_t i = 0; i < kKeyId.size(); ++i)
        {
            keyIdMatch &= request->key_id(i) == keyId[i];
        }
    }
    return ((request->media_keys_handle() == mediaKeysHandle) && (request->key_session_id() == keySessionId) &&
            keyIdMatch);
}

class RialtoClientMediaKeysIpcSelectKeyIdTest : public MediaKeysIpcTestBase
{
protected:
    RialtoClientMediaKeysIpcSelectKeyIdTest() { createMediaKeysIpc(); }

    ~RialtoClientMediaKeysIpcSelectKeyIdTest() { destroyMediaKeysIpc(); }

public:
    void setSelectKeyIdResponseSuccess(google::protobuf::Message *response)
    {
        firebolt::rialto::SelectKeyIdResponse *selectKeyIdResponse =
            dynamic_cast<firebolt::rialto::SelectKeyIdResponse *>(response);
        selectKeyIdResponse->set_error_status(MediaKeysIpcTestBase::convertMediaKeyErrorStatus(MediaKeyErrorStatus::OK));
    }

    void setSelectKeyIdResponseFailed(google::protobuf::Message *response)
    {
        firebolt::rialto::SelectKeyIdResponse *selectKeyIdResponse =
            dynamic_cast<firebolt::rialto::SelectKeyIdResponse *>(response);
        selectKeyIdResponse->set_error_status(MediaKeysIpcTestBase::convertMediaKeyErrorStatus(m_errorStatus));
    }
};

/**
 * Test selectKeyId success.
 */
TEST_F(RialtoClientMediaKeysIpcSelectKeyIdTest, Success)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("selectKeyId"), m_controllerMock.get(),
                                           selectKeyIdRequestMatcher(m_mediaKeysHandle, m_keySessionId, kKeyId), _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcSelectKeyIdTest::setSelectKeyIdResponseSuccess)));

    EXPECT_EQ(m_mediaKeysIpc->selectKeyId(m_keySessionId, kKeyId), MediaKeyErrorStatus::OK);
}

/**
 * Test that selectKeyId fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaKeysIpcSelectKeyIdTest, ChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_mediaKeysIpc->selectKeyId(m_keySessionId, kKeyId), MediaKeyErrorStatus::FAIL);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that selectKeyId fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaKeysIpcSelectKeyIdTest, ReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("selectKeyId"), _, _, _, _))
        .WillOnce(WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcSelectKeyIdTest::setSelectKeyIdResponseSuccess)));

    EXPECT_EQ(m_mediaKeysIpc->selectKeyId(m_keySessionId, kKeyId), MediaKeyErrorStatus::OK);
}

/**
 * Test that selectKeyId fails when ipc fails.
 */
TEST_F(RialtoClientMediaKeysIpcSelectKeyIdTest, Failure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("selectKeyId"), _, _, _, _));

    EXPECT_EQ(m_mediaKeysIpc->selectKeyId(m_keySessionId, kKeyId), MediaKeyErrorStatus::FAIL);
}

/**
 * Test that selectKeyId fails when api returns error.
 */
TEST_F(RialtoClientMediaKeysIpcSelectKeyIdTest, ErrorReturn)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("selectKeyId"), _, _, _, _))
        .WillOnce(WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcSelectKeyIdTest::setSelectKeyIdResponseFailed)));

    EXPECT_EQ(m_mediaKeysIpc->selectKeyId(m_keySessionId, kKeyId), m_errorStatus);
}
