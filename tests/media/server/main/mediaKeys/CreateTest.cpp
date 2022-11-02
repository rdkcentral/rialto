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

#include "MediaKeysTestBase.h"

class RialtoServerCreateMediaKeysTest : public MediaKeysTestBase
{
};

/**
 * Test that a MediaKeysServerInternal object can be created successfully.
 */
TEST_F(RialtoServerCreateMediaKeysTest, Create)
{
    EXPECT_CALL(*m_ocdmSystemFactoryMock, createOcdmSystem(kWidevineKeySystem))
        .WillOnce(Return(ByMove(std::move(m_ocdmSystem))));

    EXPECT_NO_THROW(m_mediaKeys = std::make_unique<MediaKeysServerInternal>(kWidevineKeySystem, m_ocdmSystemFactoryMock,
                                                                            m_mediaKeySessionFactoryMock));
    EXPECT_NE(m_mediaKeys, nullptr);
}

/**
 * Test that a MediaKeysServerInternal object throws an exeption if failure occurs during construction.
 * In this case, createOcdmSystem fails, returning a nullptr.
 */
TEST_F(RialtoServerCreateMediaKeysTest, CreateOcdmSystemFailure)
{
    EXPECT_CALL(*m_ocdmSystemFactoryMock, createOcdmSystem(kWidevineKeySystem)).WillOnce(Return(ByMove(std::move(nullptr))));

    EXPECT_THROW(m_mediaKeys = std::make_unique<MediaKeysServerInternal>(kWidevineKeySystem, m_ocdmSystemFactoryMock,
                                                                         m_mediaKeySessionFactoryMock),
                 std::runtime_error);
}
