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

#include "KeyIdMap.h"
#include "MediaKeys.h"
#include "MediaKeysClientMock.h"
#include "MediaKeysIpcFactoryMock.h"
#include "MediaKeysMock.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto;
using namespace firebolt::rialto::client;
using namespace firebolt::rialto::client;

using ::testing::_;
using ::testing::ByMove;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::StrictMock;

class RialtoClientMediaKeysNetflixKeySessionTest : public ::testing::Test
{
protected:
    std::shared_ptr<StrictMock<MediaKeysIpcFactoryMock>> m_mediaKeysIpcFactoryMock;
    std::unique_ptr<StrictMock<MediaKeysMock>> m_mediaKeysIpc;
    StrictMock<MediaKeysMock> *m_mediaKeysIpcMock;
    std::unique_ptr<IMediaKeys> m_mediaKeys;

    std::string m_keySystem{"com.netflix.playready"};
    int32_t m_keySessionId{123};
    MediaKeyErrorStatus m_mediaKeyErrorStatus{MediaKeyErrorStatus::OK};
    std::vector<uint8_t> m_keyId{1, 2, 3};

    RialtoClientMediaKeysNetflixKeySessionTest()
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
TEST_F(RialtoClientMediaKeysNetflixKeySessionTest, CreateKeySession)
{
    KeySessionType sessionType = KeySessionType::PERSISTENT_LICENCE;
    std::shared_ptr<StrictMock<MediaKeysClientMock>> mediaKeysClientMock =
        std::make_shared<StrictMock<MediaKeysClientMock>>();
    int32_t returnKeySessionId;

    EXPECT_CALL(*m_mediaKeysIpcMock, createKeySession(sessionType, _, _))
        .WillOnce(DoAll(SetArgReferee<2>(m_keySessionId), Return(m_mediaKeyErrorStatus)));

    EXPECT_EQ(m_mediaKeys->createKeySession(sessionType, mediaKeysClientMock, returnKeySessionId), m_mediaKeyErrorStatus);
    EXPECT_EQ(returnKeySessionId, m_keySessionId);
    // Update key should be possible, as keySession should be present in KeyIdMap
    EXPECT_TRUE(KeyIdMap::instance().updateKey(m_keySessionId, m_keyId));

    // Cleanup
    KeyIdMap::instance().erase(m_keySessionId);
    EXPECT_TRUE(KeyIdMap::instance().get(m_keySessionId).empty());
}

/**
 * Test that a SelectKeyId adds KeySession <-> KeyId mapping.
 */
TEST_F(RialtoClientMediaKeysNetflixKeySessionTest, SelectKeyId)
{
    // Simulate Add session before updating it
    KeyIdMap::instance().addSession(m_keySessionId);

    EXPECT_TRUE(KeyIdMap::instance().get(m_keySessionId).empty());
    EXPECT_EQ(m_mediaKeys->selectKeyId(m_keySessionId, m_keyId), MediaKeyErrorStatus::OK);
    EXPECT_FALSE(KeyIdMap::instance().get(m_keySessionId).empty());
    EXPECT_EQ(KeyIdMap::instance().get(m_keySessionId), m_keyId);

    // Cleanup
    KeyIdMap::instance().erase(m_keySessionId);
    EXPECT_TRUE(KeyIdMap::instance().get(m_keySessionId).empty());
}

/**
 * Test that a SelectKeyId fails to add KeySession <-> KeyId mapping.
 */
TEST_F(RialtoClientMediaKeysNetflixKeySessionTest, SelectKeyIdFailure)
{
    EXPECT_EQ(m_mediaKeys->selectKeyId(m_keySessionId, m_keyId), MediaKeyErrorStatus::FAIL);
}

/**
 * Test that a CloseKeySession forwards the request to IPC and returns the error status.
 */
TEST_F(RialtoClientMediaKeysNetflixKeySessionTest, CloseKeySession)
{
    // Simulate Add session before erasing it
    KeyIdMap::instance().addSession(m_keySessionId);
    EXPECT_CALL(*m_mediaKeysIpcMock, closeKeySession(m_keySessionId)).WillOnce(Return(m_mediaKeyErrorStatus));

    EXPECT_EQ(m_mediaKeys->closeKeySession(m_keySessionId), m_mediaKeyErrorStatus);

    // Update key should not be possible, as keySession should not exist in KeyIdMap
    EXPECT_FALSE(KeyIdMap::instance().updateKey(m_keySessionId, m_keyId));
}
