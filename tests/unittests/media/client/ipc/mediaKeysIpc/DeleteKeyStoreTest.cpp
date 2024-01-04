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

class RialtoClientMediaKeysIpcDeleteKeyStoreTest : public MediaKeysIpcTestBase
{
protected:
    RialtoClientMediaKeysIpcDeleteKeyStoreTest() { createMediaKeysIpc(); }

    ~RialtoClientMediaKeysIpcDeleteKeyStoreTest() { destroyMediaKeysIpc(); }

public:
    void setDeleteKeyStoreResponseSuccess(google::protobuf::Message *response)
    {
        firebolt::rialto::DeleteKeyStoreResponse *deleteKeyStoreResponse =
            dynamic_cast<firebolt::rialto::DeleteKeyStoreResponse *>(response);
        deleteKeyStoreResponse->set_error_status(
            MediaKeysIpcTestBase::convertMediaKeyErrorStatus(MediaKeyErrorStatus::OK));
    }

    void setDeleteKeyStoreResponseFailed(google::protobuf::Message *response)
    {
        firebolt::rialto::DeleteKeyStoreResponse *deleteKeyStoreResponse =
            dynamic_cast<firebolt::rialto::DeleteKeyStoreResponse *>(response);
        deleteKeyStoreResponse->set_error_status(MediaKeysIpcTestBase::convertMediaKeyErrorStatus(m_errorStatus));
    }
};

/**
 * Test deleteKeyStore success.
 */
TEST_F(RialtoClientMediaKeysIpcDeleteKeyStoreTest, Success)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("deleteKeyStore"), m_controllerMock.get(),
                           deleteKeyStoreRequestMatcher(m_mediaKeysHandle), _, m_blockingClosureMock.get()))
        .WillOnce(
            WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcDeleteKeyStoreTest::setDeleteKeyStoreResponseSuccess)));

    EXPECT_EQ(m_mediaKeysIpc->deleteKeyStore(), MediaKeyErrorStatus::OK);
}

/**
 * Test that deleteKeyStore fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaKeysIpcDeleteKeyStoreTest, ChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_mediaKeysIpc->deleteKeyStore(), MediaKeyErrorStatus::FAIL);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that deleteKeyStore fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaKeysIpcDeleteKeyStoreTest, ReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("deleteKeyStore"), _, _, _, _))
        .WillOnce(
            WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcDeleteKeyStoreTest::setDeleteKeyStoreResponseSuccess)));

    EXPECT_EQ(m_mediaKeysIpc->deleteKeyStore(), MediaKeyErrorStatus::OK);
}

/**
 * Test that deleteKeyStore fails when ipc fails.
 */
TEST_F(RialtoClientMediaKeysIpcDeleteKeyStoreTest, Failure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("deleteKeyStore"), _, _, _, _));

    EXPECT_EQ(m_mediaKeysIpc->deleteKeyStore(), MediaKeyErrorStatus::FAIL);
}

/**
 * Test that deleteKeyStore fails when api returns error.
 */
TEST_F(RialtoClientMediaKeysIpcDeleteKeyStoreTest, ErrorReturn)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("deleteKeyStore"), _, _, _, _))
        .WillOnce(WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcDeleteKeyStoreTest::setDeleteKeyStoreResponseFailed)));

    EXPECT_EQ(m_mediaKeysIpc->deleteKeyStore(), m_errorStatus);
}
