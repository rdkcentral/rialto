/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Sky UK
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
using namespace firebolt::rialto::wrappers;

using ::testing::_;
using ::testing::ByMove;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::StrictMock;

namespace
{
const std::string kKeySystem{"com.netflix.playready"};
const std::vector<std::string> kRobustnessLevels{"HW_SECURE_ALL", "SW_SECURE_CRYPTO"};
} // namespace

class RialtoServerMediaKeysCapabilitiesRobustnessTest : public ::testing::Test
{
protected:
    std::shared_ptr<StrictMock<OcdmFactoryMock>> m_ocdmFactoryMock;
    std::shared_ptr<StrictMock<OcdmMock>> m_ocdmMock;
    std::shared_ptr<StrictMock<OcdmSystemFactoryMock>> m_ocdmSystemFactoryMock;
    std::shared_ptr<StrictMock<OcdmSystemMock>> m_ocdmSystem;
    StrictMock<OcdmSystemMock> *m_ocdmSystemMock;
    std::shared_ptr<IMediaKeysCapabilities> m_mediaKeysCapabilities;

    RialtoServerMediaKeysCapabilitiesRobustnessTest()
        : m_ocdmFactoryMock{std::make_shared<StrictMock<OcdmFactoryMock>>()},
          m_ocdmMock{std::make_shared<StrictMock<OcdmMock>>()},
          m_ocdmSystemFactoryMock{std::make_shared<StrictMock<OcdmSystemFactoryMock>>()},
          m_ocdmSystem{std::make_shared<StrictMock<OcdmSystemMock>>()}, m_ocdmSystemMock{m_ocdmSystem.get()}
    {
        EXPECT_CALL(*m_ocdmFactoryMock, getOcdm()).WillOnce(Return(m_ocdmMock));

        EXPECT_NO_THROW(m_mediaKeysCapabilities =
                            std::make_shared<MediaKeysCapabilities>(m_ocdmFactoryMock, m_ocdmSystemFactoryMock));
        EXPECT_NE(m_mediaKeysCapabilities, nullptr);
    }
};

/**
 * Test that getSupportedRobustnessLevels returns the levels from the ocdm system.
 */
TEST_F(RialtoServerMediaKeysCapabilitiesRobustnessTest, GetSupportedRobustnessLevelsSuccess)
{
    std::vector<std::string> levels;
    EXPECT_CALL(*m_ocdmSystemFactoryMock, createOcdmSystem(kKeySystem)).WillOnce(Return(ByMove(std::move(m_ocdmSystem))));
    EXPECT_CALL(*m_ocdmSystemMock, getSupportedRobustnessLevels(_))
        .WillOnce(DoAll(SetArgReferee<0>(kRobustnessLevels), Return(true)));

    EXPECT_TRUE(m_mediaKeysCapabilities->getSupportedRobustnessLevels(kKeySystem, levels));
    EXPECT_EQ(levels, kRobustnessLevels);
}

/**
 * Test that getSupportedRobustnessLevels returns false when the ocdm system returns false.
 */
TEST_F(RialtoServerMediaKeysCapabilitiesRobustnessTest, GetSupportedRobustnessLevelsFailure)
{
    std::vector<std::string> levels;
    EXPECT_CALL(*m_ocdmSystemFactoryMock, createOcdmSystem(kKeySystem)).WillOnce(Return(ByMove(std::move(m_ocdmSystem))));
    EXPECT_CALL(*m_ocdmSystemMock, getSupportedRobustnessLevels(_)).WillOnce(Return(false));

    EXPECT_FALSE(m_mediaKeysCapabilities->getSupportedRobustnessLevels(kKeySystem, levels));
}

/**
 * Test that getSupportedRobustnessLevels returns false if failure to create the ocdm system object.
 */
TEST_F(RialtoServerMediaKeysCapabilitiesRobustnessTest, OcdmSystemFailure)
{
    std::vector<std::string> levels;
    EXPECT_CALL(*m_ocdmSystemFactoryMock, createOcdmSystem(kKeySystem)).WillOnce(Return(ByMove(nullptr)));
    EXPECT_FALSE(m_mediaKeysCapabilities->getSupportedRobustnessLevels(kKeySystem, levels));
}
