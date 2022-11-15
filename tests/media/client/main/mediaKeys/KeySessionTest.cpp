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

#include "MediaKeys.h"
#include "MediaKeysClientMock.h"
#include "MediaKeysIpcFactoryMock.h"
#include "MediaKeysMock.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::client;

using ::testing::_;
using ::testing::ByMove;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::StrictMock;

class RialtoClientMediaKeysKeySessionTest : public ::testing::Test
{
protected:
    std::shared_ptr<StrictMock<MediaKeysIpcFactoryMock>> m_mediaKeysIpcFactoryMock;
    std::unique_ptr<StrictMock<MediaKeysMock>> m_mediaKeysIpc;
    StrictMock<MediaKeysMock> *m_mediaKeysIpcMock;
    std::unique_ptr<IMediaKeys> m_mediaKeys;

    std::string m_keySystem{"keySystem"};
    int32_t m_keySessionId{123};
    MediaKeyErrorStatus m_mediaKeyErrorStatus{MediaKeyErrorStatus::OK};

    RialtoClientMediaKeysKeySessionTest()
        : m_mediaKeysIpcFactoryMock{std::make_shared<StrictMock<MediaKeysIpcFactoryMock>>()},
          m_mediaKeysIpc{std::make_unique<StrictMock<MediaKeysMock>>()}, m_mediaKeysIpcMock{m_mediaKeysIpc.get()}
    {
        EXPECT_CALL(*m_mediaKeysIpcFactoryMock, createMediaKeysIpc(m_keySystem))
            .WillOnce(Return(ByMove(std::move(m_mediaKeysIpc))));

        EXPECT_NO_THROW(m_mediaKeys = std::make_unique<MediaKeys>(m_keySystem, m_mediaKeysIpcFactoryMock));
        EXPECT_NE(m_mediaKeys, nullptr);
    }
};

/**
 * Test that a CreateKeySession forwards the request to IPC and returns the key session id and error status.
 */
TEST_F(RialtoClientMediaKeysKeySessionTest, CreateKeySession)
{
    KeySessionType sessionType = KeySessionType::PERSISTENT_LICENCE;
    std::shared_ptr<StrictMock<MediaKeysClientMock>> mediaKeysClientMock =
        std::make_shared<StrictMock<MediaKeysClientMock>>();
    bool isLDL = false;
    int32_t returnKeySessionId;

    EXPECT_CALL(*m_mediaKeysIpcMock, createKeySession(sessionType, _, isLDL, _))
        .WillOnce(DoAll(SetArgReferee<3>(m_keySessionId), Return(m_mediaKeyErrorStatus)));

    EXPECT_EQ(m_mediaKeys->createKeySession(sessionType, mediaKeysClientMock, isLDL, returnKeySessionId),
              m_mediaKeyErrorStatus);
    EXPECT_EQ(returnKeySessionId, m_keySessionId);
}

/**
 * Test that a GenerateRequest forwards the request to IPC and returns the error status.
 */
TEST_F(RialtoClientMediaKeysKeySessionTest, GenerateRequest)
{
    InitDataType initDataType = InitDataType::KEY_IDS;
    std::vector<uint8_t> initData{7, 8, 9};

    EXPECT_CALL(*m_mediaKeysIpcMock, generateRequest(m_keySessionId, initDataType, initData))
        .WillOnce(Return(m_mediaKeyErrorStatus));

    EXPECT_EQ(m_mediaKeys->generateRequest(m_keySessionId, initDataType, initData), m_mediaKeyErrorStatus);
}

/**
 * Test that a LoadSession forwards the request to IPC and returns the error status.
 */
TEST_F(RialtoClientMediaKeysKeySessionTest, LoadSession)
{
    EXPECT_CALL(*m_mediaKeysIpcMock, loadSession(m_keySessionId)).WillOnce(Return(m_mediaKeyErrorStatus));

    EXPECT_EQ(m_mediaKeys->loadSession(m_keySessionId), m_mediaKeyErrorStatus);
}

/**
 * Test that a UpdateSession forwards the request to IPC and returns the error status.
 */
TEST_F(RialtoClientMediaKeysKeySessionTest, UpdateSession)
{
    std::vector<uint8_t> responseData{10, 11, 12};

    EXPECT_CALL(*m_mediaKeysIpcMock, updateSession(m_keySessionId, responseData)).WillOnce(Return(m_mediaKeyErrorStatus));

    EXPECT_EQ(m_mediaKeys->updateSession(m_keySessionId, responseData), m_mediaKeyErrorStatus);
}

/**
 * Test that a CloseKeySession forwards the request to IPC and returns the error status.
 */
TEST_F(RialtoClientMediaKeysKeySessionTest, CloseKeySession)
{
    EXPECT_CALL(*m_mediaKeysIpcMock, closeKeySession(m_keySessionId)).WillOnce(Return(m_mediaKeyErrorStatus));

    EXPECT_EQ(m_mediaKeys->closeKeySession(m_keySessionId), m_mediaKeyErrorStatus);
}

/**
 * Test that a RemoveKeySession forwards the request to IPC and returns the error status.
 */
TEST_F(RialtoClientMediaKeysKeySessionTest, RemoveKeySession)
{
    EXPECT_CALL(*m_mediaKeysIpcMock, removeKeySession(m_keySessionId)).WillOnce(Return(m_mediaKeyErrorStatus));

    EXPECT_EQ(m_mediaKeys->removeKeySession(m_keySessionId), m_mediaKeyErrorStatus);
}

/**
 * Test that a GetCdmKeySessionId forwards the request to IPC and returns the error status.
 */
TEST_F(RialtoClientMediaKeysKeySessionTest, GetCdmKeySessionIdSession)
{
    std::string cdmKeySessionId;
    EXPECT_CALL(*m_mediaKeysIpcMock, getCdmKeySessionId(m_keySessionId, _)).WillOnce(Return(m_mediaKeyErrorStatus));

    EXPECT_EQ(m_mediaKeys->getCdmKeySessionId(m_keySessionId, cdmKeySessionId), m_mediaKeyErrorStatus);
}
