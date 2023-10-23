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

InitDataType covertInitDataType(GenerateRequestRequest_InitDataType protoInitDataType)
{
    switch (protoInitDataType)
    {
    case GenerateRequestRequest_InitDataType::GenerateRequestRequest_InitDataType_CENC:
        return InitDataType::CENC;
    case GenerateRequestRequest_InitDataType::GenerateRequestRequest_InitDataType_KEY_IDS:
        return InitDataType::KEY_IDS;
    case GenerateRequestRequest_InitDataType::GenerateRequestRequest_InitDataType_WEBM:
        return InitDataType::WEBM;
    case GenerateRequestRequest_InitDataType::GenerateRequestRequest_InitDataType_DRMHEADER:
        return InitDataType::DRMHEADER;
    default:
        return InitDataType::UNKNOWN;
    }
}

MATCHER_P4(generateRequestRequestMatcher, mediaKeysHandle, keySessionId, initDataType, initData, "")
{
    const ::firebolt::rialto::GenerateRequestRequest *request =
        dynamic_cast<const ::firebolt::rialto::GenerateRequestRequest *>(arg);
    return ((request->media_keys_handle() == mediaKeysHandle) && (request->key_session_id() == keySessionId) &&
            (covertInitDataType(request->init_data_type()) == initDataType) &&
            (std::vector<std::uint8_t>{request->init_data().begin(), request->init_data().end()} == initData));
}

class RialtoClientMediaKeysIpcGenerateRequestTest : public MediaKeysIpcTestBase
{
protected:
    InitDataType m_initDataType = InitDataType::KEY_IDS;
    std::vector<std::uint8_t> m_initData{6, 7, 2};

    RialtoClientMediaKeysIpcGenerateRequestTest() { createMediaKeysIpc(); }

    ~RialtoClientMediaKeysIpcGenerateRequestTest() { destroyMediaKeysIpc(); }

public:
    void setGenerateRequestResponseSuccess(google::protobuf::Message *response)
    {
        firebolt::rialto::GenerateRequestResponse *generateRequestResponse =
            dynamic_cast<firebolt::rialto::GenerateRequestResponse *>(response);
        generateRequestResponse->set_error_status(
            MediaKeysIpcTestBase::convertMediaKeyErrorStatus(MediaKeyErrorStatus::OK));
    }

    void setGenerateRequestResponseFailed(google::protobuf::Message *response)
    {
        firebolt::rialto::GenerateRequestResponse *generateRequestResponse =
            dynamic_cast<firebolt::rialto::GenerateRequestResponse *>(response);
        generateRequestResponse->set_error_status(MediaKeysIpcTestBase::convertMediaKeyErrorStatus(m_errorStatus));
    }
};

/**
 * Test GenerateRequest success.
 */
TEST_F(RialtoClientMediaKeysIpcGenerateRequestTest, Success)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("generateRequest"), m_controllerMock.get(),
                           generateRequestRequestMatcher(m_mediaKeysHandle, m_kKeySessionId, m_initDataType, m_initData),
                           _, m_blockingClosureMock.get()))
        .WillOnce(
            WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcGenerateRequestTest::setGenerateRequestResponseSuccess)));

    EXPECT_EQ(m_mediaKeysIpc->generateRequest(m_kKeySessionId, m_initDataType, m_initData), MediaKeyErrorStatus::OK);
}

/**
 * Test that GenerateRequest fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaKeysIpcGenerateRequestTest, ChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_mediaKeysIpc->generateRequest(m_kKeySessionId, m_initDataType, m_initData), MediaKeyErrorStatus::FAIL);

    // Reattach channel on destroySession
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock)).RetiresOnSaturation();
    expectSubscribeEvents();
}

/**
 * Test that GenerateRequest fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaKeysIpcGenerateRequestTest, ReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("generateRequest"), _, _, _, _))
        .WillOnce(
            WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcGenerateRequestTest::setGenerateRequestResponseSuccess)));

    EXPECT_EQ(m_mediaKeysIpc->generateRequest(m_kKeySessionId, m_initDataType, m_initData), MediaKeyErrorStatus::OK);
}

/**
 * Test that GenerateRequest fails when ipc fails.
 */
TEST_F(RialtoClientMediaKeysIpcGenerateRequestTest, Failure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("generateRequest"), _, _, _, _));

    EXPECT_EQ(m_mediaKeysIpc->generateRequest(m_kKeySessionId, m_initDataType, m_initData), MediaKeyErrorStatus::FAIL);
}

/**
 * Test that GenerateRequest fails when api returns error.
 */
TEST_F(RialtoClientMediaKeysIpcGenerateRequestTest, ErrorReturn)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("generateRequest"), _, _, _, _))
        .WillOnce(
            WithArgs<3>(Invoke(this, &RialtoClientMediaKeysIpcGenerateRequestTest::setGenerateRequestResponseFailed)));

    EXPECT_EQ(m_mediaKeysIpc->generateRequest(m_kKeySessionId, m_initDataType, m_initData), m_errorStatus);
}
