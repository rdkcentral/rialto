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

MATCHER_P(deleteDrmStoreRequestMatcher, mediaKeysHandle, "")
{
    const ::firebolt::rialto::DeleteDrmStoreRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::DeleteDrmStoreRequest *>(arg);
    return (kRequest->media_keys_handle() == mediaKeysHandle);
}

class RialtoClientMediaKeysIpcDeleteDrmStoreTest : public MediaKeysIpcTestBase
{
protected:
    RialtoClientMediaKeysIpcDeleteDrmStoreTest() { createMediaKeysIpc(); }

    ~RialtoClientMediaKeysIpcDeleteDrmStoreTest() { destroyMediaKeysIpc(); }

public:
    void setDeleteDrmStoreResponseSuccess(google::protobuf::Message *response)
    {
        firebolt::rialto::DeleteDrmStoreResponse *deleteDrmStoreResponse =
            dynamic_cast<firebolt::rialto::DeleteDrmStoreResponse *>(response);
        deleteDrmStoreResponse->set_error_status(
            MediaKeysIpcTestBase::convertMediaKeyErrorStatus(MediaKeyErrorStatus::OK));
    }

    void setDeleteDrmStoreResponseFailed(google::protobuf::Message *response)
    {
        firebolt::rialto::DeleteDrmStoreResponse *deleteDrmStoreResponse =
            dynamic_cast<firebolt::rialto::DeleteDrmStoreResponse *>(response);
        deleteDrmStoreResponse->set_error_status(MediaKeysIpcTestBase::convertMediaKeyErrorStatus(m_errorStatus));
    }
};

/**
 * Test deleteDrmStore success.
 */
TEST_F(RialtoClientMediaKeysIpcDeleteDrmStoreTest, Success)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("deleteDrmStore"), m_controllerMock.get(),
                           deleteDrmStoreRequestMatcher(m_mediaKeysHandle), _, m_blockingClosureMock.get()))
        .WillOnce(
            WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcDeleteDrmStoreTest::setDeleteDrmStoreResponseSuccess)));

    EXPECT_EQ(m_mediaKeysIpc->deleteDrmStore(), MediaKeyErrorStatus::OK);
}

/**
 * Test that deleteDrmStore fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaKeysIpcDeleteDrmStoreTest, ChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_mediaKeysIpc->deleteDrmStore(), MediaKeyErrorStatus::FAIL);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that deleteDrmStore fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaKeysIpcDeleteDrmStoreTest, ReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("deleteDrmStore"), _, _, _, _))
        .WillOnce(
            WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcDeleteDrmStoreTest::setDeleteDrmStoreResponseSuccess)));

    EXPECT_EQ(m_mediaKeysIpc->deleteDrmStore(), MediaKeyErrorStatus::OK);
}

/**
 * Test that deleteDrmStore fails when ipc fails.
 */
TEST_F(RialtoClientMediaKeysIpcDeleteDrmStoreTest, Failure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("deleteDrmStore"), _, _, _, _));

    EXPECT_EQ(m_mediaKeysIpc->deleteDrmStore(), MediaKeyErrorStatus::FAIL);
}

/**
 * Test that deleteDrmStore fails when api returns error.
 */
TEST_F(RialtoClientMediaKeysIpcDeleteDrmStoreTest, ErrorReturn)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("deleteDrmStore"), _, _, _, _))
        .WillOnce(WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcDeleteDrmStoreTest::setDeleteDrmStoreResponseFailed)));

    EXPECT_EQ(m_mediaKeysIpc->deleteDrmStore(), m_errorStatus);
}
