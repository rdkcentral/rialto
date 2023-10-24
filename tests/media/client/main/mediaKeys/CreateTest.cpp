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
#include "MediaKeysIpcFactoryMock.h"
#include "MediaKeysMock.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::client;

using ::testing::ByMove;
using ::testing::Return;
using ::testing::StrictMock;

class RialtoClientCreateMediaKeysTest : public ::testing::Test
{
protected:
    std::string m_keySystem = "keySystem";
    std::shared_ptr<StrictMock<MediaKeysIpcFactoryMock>> m_mediaKeysIpcFactoryMock;

    RialtoClientCreateMediaKeysTest()
        : m_mediaKeysIpcFactoryMock{std::make_shared<StrictMock<MediaKeysIpcFactoryMock>>()}
    {
    }
};

/**
 * Test that a MediaKeys object can be created successfully.
 */
TEST_F(RialtoClientCreateMediaKeysTest, Create)
{
    std::unique_ptr<IMediaKeys> mediaKeys;
    std::unique_ptr<StrictMock<MediaKeysMock>> mediaKeysIpcMock = std::make_unique<StrictMock<MediaKeysMock>>();

    EXPECT_CALL(*m_mediaKeysIpcFactoryMock, createMediaKeysIpc(m_keySystem))
        .WillOnce(Return(ByMove(std::move(mediaKeysIpcMock))));

    EXPECT_NO_THROW(mediaKeys = std::make_unique<MediaKeys>(m_keySystem, m_mediaKeysIpcFactoryMock));
    EXPECT_NE(mediaKeys, nullptr);
}

/**
 * Test that a MediaKeys object throws an exeption if failure occurs during construction.
 * In this case, createMediaKeysIpc fails, returning a nullptr.
 */
TEST_F(RialtoClientCreateMediaKeysTest, CreateMediaKeysIpcFailure)
{
    std::unique_ptr<IMediaKeys> mediaKeys;

    EXPECT_CALL(*m_mediaKeysIpcFactoryMock, createMediaKeysIpc(m_keySystem)).WillOnce(Return(ByMove(nullptr)));

    EXPECT_THROW(mediaKeys = std::make_unique<MediaKeys>(m_keySystem, m_mediaKeysIpcFactoryMock), std::runtime_error);
}
