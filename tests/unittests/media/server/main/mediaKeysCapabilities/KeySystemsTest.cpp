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
#include <array>
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;

using ::testing::_;
using ::testing::ByMove;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::StrictMock;

class RialtoServerMediaKeysCapabilitiesKeySystemsTest : public ::testing::Test
{
protected:
    std::shared_ptr<StrictMock<OcdmFactoryMock>> m_ocdmFactoryMock;
    std::shared_ptr<StrictMock<OcdmMock>> m_ocdmMock;
    std::shared_ptr<StrictMock<OcdmSystemFactoryMock>> m_ocdmSystemFactoryMock;
    std::unique_ptr<StrictMock<OcdmSystemMock>> m_ocdmSystem;
    StrictMock<OcdmSystemMock> *m_ocdmSystemMock;
    std::shared_ptr<IMediaKeysCapabilities> m_mediaKeysCapabilities;

    const std::array<std::string, 3> m_supportedKeySystems{"com.widevine.alpha", "com.microsoft.playready",
                                                           "com.netflix.playready"};
    const std::string m_version{"123"};

    RialtoServerMediaKeysCapabilitiesKeySystemsTest()
        : m_ocdmFactoryMock{std::make_shared<StrictMock<OcdmFactoryMock>>()},
          m_ocdmMock{std::make_shared<StrictMock<OcdmMock>>()},
          m_ocdmSystemFactoryMock{std::make_shared<StrictMock<OcdmSystemFactoryMock>>()},
          m_ocdmSystem{std::make_unique<StrictMock<OcdmSystemMock>>()}, m_ocdmSystemMock{m_ocdmSystem.get()}
    {
        EXPECT_CALL(*m_ocdmFactoryMock, getOcdm()).WillOnce(Return(m_ocdmMock));

        EXPECT_NO_THROW(m_mediaKeysCapabilities =
                            std::make_shared<MediaKeysCapabilities>(m_ocdmFactoryMock, m_ocdmSystemFactoryMock));
        EXPECT_NE(m_mediaKeysCapabilities, nullptr);
    }
};

/**
 * Test that a GetSupportedKeySystems returns the correct supported key systems.
 */
TEST_F(RialtoServerMediaKeysCapabilitiesKeySystemsTest, GetSupportedKeySystems)
{
    EXPECT_CALL(*m_ocdmMock, isTypeSupported(m_supportedKeySystems[0])).WillOnce(Return(MediaKeyErrorStatus::OK));
    EXPECT_CALL(*m_ocdmMock, isTypeSupported(m_supportedKeySystems[1])).WillOnce(Return(MediaKeyErrorStatus::NOT_SUPPORTED));
    EXPECT_CALL(*m_ocdmMock, isTypeSupported(m_supportedKeySystems[2])).WillOnce(Return(MediaKeyErrorStatus::OK));

    std::vector<std::string> expectedReturnKeySystems{m_supportedKeySystems[0], m_supportedKeySystems[2]};
    EXPECT_EQ(m_mediaKeysCapabilities->getSupportedKeySystems(), expectedReturnKeySystems);
}

/**
 * Test that a SupportsKeySystem returns true if key system supported.
 */
TEST_F(RialtoServerMediaKeysCapabilitiesKeySystemsTest, DoesSupportKeySystem)
{
    EXPECT_CALL(*m_ocdmMock, isTypeSupported(m_supportedKeySystems[0])).WillOnce(Return(MediaKeyErrorStatus::OK));

    EXPECT_EQ(m_mediaKeysCapabilities->supportsKeySystem(m_supportedKeySystems[0]), true);
}

/**
 * Test that a SupportsKeySystem returns false if key system not supported.
 */
TEST_F(RialtoServerMediaKeysCapabilitiesKeySystemsTest, DoesNotSupportKeySystem)
{
    EXPECT_CALL(*m_ocdmMock, isTypeSupported(m_supportedKeySystems[0])).WillOnce(Return(MediaKeyErrorStatus::NOT_SUPPORTED));

    EXPECT_EQ(m_mediaKeysCapabilities->supportsKeySystem(m_supportedKeySystems[0]), false);
}

/**
 * Test that a GetSupportedKeySystemVersion returns true and sets the version on success.
 */
TEST_F(RialtoServerMediaKeysCapabilitiesKeySystemsTest, GetSupportedKeySystemVersionSuccess)
{
    std::string returnVersion;
    EXPECT_CALL(*m_ocdmSystemFactoryMock, createOcdmSystem(m_supportedKeySystems[0]))
        .WillOnce(Return(ByMove(std::move(m_ocdmSystem))));
    EXPECT_CALL(*m_ocdmSystemMock, getVersion(_))
        .WillOnce(DoAll(SetArgReferee<0>(m_version), Return(MediaKeyErrorStatus::OK)));

    EXPECT_EQ(m_mediaKeysCapabilities->getSupportedKeySystemVersion(m_supportedKeySystems[0], returnVersion), true);
    EXPECT_EQ(returnVersion, m_version);
}

/**
 * Test that a GetSupportedKeySystemVersion returns false and version string is empty if failure to
 * create the ocdm system object.
 */
TEST_F(RialtoServerMediaKeysCapabilitiesKeySystemsTest, GetSupportedKeySystemVersionOcdmSystemFailure)
{
    std::string returnVersion;
    EXPECT_CALL(*m_ocdmSystemFactoryMock, createOcdmSystem(m_supportedKeySystems[0])).WillOnce(Return(ByMove(nullptr)));

    EXPECT_EQ(m_mediaKeysCapabilities->getSupportedKeySystemVersion(m_supportedKeySystems[0], returnVersion), false);
    EXPECT_EQ(returnVersion, "");
}

/**
 * Test that a GetSupportedKeySystemVersion returns false and version string is empty if getVersion fails.
 */
TEST_F(RialtoServerMediaKeysCapabilitiesKeySystemsTest, GetSupportedKeySystemVersionGetVersionFailure)
{
    std::string returnVersion;
    EXPECT_CALL(*m_ocdmSystemFactoryMock, createOcdmSystem(m_supportedKeySystems[0]))
        .WillOnce(Return(ByMove(std::move(m_ocdmSystem))));
    EXPECT_CALL(*m_ocdmSystemMock, getVersion(_))
        .WillOnce(DoAll(SetArgReferee<0>(m_version), Return(MediaKeyErrorStatus::FAIL)));

    EXPECT_EQ(m_mediaKeysCapabilities->getSupportedKeySystemVersion(m_supportedKeySystems[0], returnVersion), false);
    EXPECT_EQ(returnVersion, "");
}
