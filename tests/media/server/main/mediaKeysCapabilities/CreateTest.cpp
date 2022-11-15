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

#include "MediaKeysCapabilities.h"
#include "OcdmFactoryMock.h"
#include "OcdmMock.h"
#include "OcdmSystemFactoryMock.h"
#include "OcdmSystemMock.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;

using ::testing::Return;
using ::testing::StrictMock;

class RialtoServerCreateMediaKeysCapabilitiesTest : public ::testing::Test
{
protected:
    std::shared_ptr<StrictMock<OcdmFactoryMock>> m_ocdmFactoryMock;
    std::shared_ptr<StrictMock<OcdmMock>> m_ocdmMock;
    std::shared_ptr<StrictMock<OcdmSystemFactoryMock>> m_ocdmSystemFactoryMock;

    RialtoServerCreateMediaKeysCapabilitiesTest()
        : m_ocdmFactoryMock{std::make_shared<StrictMock<OcdmFactoryMock>>()},
          m_ocdmMock{std::make_shared<StrictMock<OcdmMock>>()}, m_ocdmSystemFactoryMock{
                                                                    std::make_shared<StrictMock<OcdmSystemFactoryMock>>()}
    {
    }
};

/**
 * Test that a MediaKeysCapabilities object can be created successfully.
 */
TEST_F(RialtoServerCreateMediaKeysCapabilitiesTest, Create)
{
    std::shared_ptr<IMediaKeysCapabilities> mediaKeysCapabilities;

    EXPECT_CALL(*m_ocdmFactoryMock, getOcdm()).WillOnce(Return(m_ocdmMock));

    EXPECT_NO_THROW(
        mediaKeysCapabilities = std::make_shared<MediaKeysCapabilities>(m_ocdmFactoryMock, m_ocdmSystemFactoryMock));
    EXPECT_NE(mediaKeysCapabilities, nullptr);
}

/**
 * Test that a MediaKeysCapabilities object throws an exeption if failure occurs during construction.
 * In this case, getOcdm fails, returning a nullptr.
 */
TEST_F(RialtoServerCreateMediaKeysCapabilitiesTest, GetOcdmSystemFailure)
{
    std::shared_ptr<IMediaKeysCapabilities> mediaKeysCapabilities;

    EXPECT_CALL(*m_ocdmFactoryMock, getOcdm()).WillOnce(Return(nullptr));

    EXPECT_THROW(mediaKeysCapabilities =
                     std::make_shared<MediaKeysCapabilities>(m_ocdmFactoryMock, m_ocdmSystemFactoryMock),
                 std::runtime_error);
}
