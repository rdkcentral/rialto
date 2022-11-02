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
#include "MediaKeysCapabilitiesIpcFactoryMock.h"
#include "MediaKeysCapabilitiesIpcMock.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::client;
using namespace firebolt::rialto::client::mock;

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::StrictMock;

class RialtoClientMediaKeysCapabilitiesKeySystemsTest : public ::testing::Test
{
protected:
    std::shared_ptr<IMediaKeysCapabilities> m_mediaKeysCapabilities;
    std::shared_ptr<StrictMock<MediaKeysCapabilitiesIpcMock>> m_mediaKeysCapabilitiesIpcMock;

    std::string m_keySystems = "keysystem";
    std::string m_version = "version";
    std::vector<std::string> m_supportedKeySystems = {"keysystem1", "keysystem2", "keysystem3"};

    RialtoClientMediaKeysCapabilitiesKeySystemsTest()
        : m_mediaKeysCapabilitiesIpcMock{std::make_shared<StrictMock<MediaKeysCapabilitiesIpcMock>>()}
    {
        std::shared_ptr<StrictMock<MediaKeysCapabilitiesIpcFactoryMock>> mediaKeysCapabilitiesIpcFactoryMock =
            std::make_shared<StrictMock<MediaKeysCapabilitiesIpcFactoryMock>>();

        EXPECT_CALL(*mediaKeysCapabilitiesIpcFactoryMock, getMediaKeysCapabilitiesIpc())
            .WillOnce(Return(m_mediaKeysCapabilitiesIpcMock));

        EXPECT_NO_THROW(
            m_mediaKeysCapabilities = std::make_shared<MediaKeysCapabilities>(mediaKeysCapabilitiesIpcFactoryMock));
        EXPECT_NE(m_mediaKeysCapabilities, nullptr);
    }
};

/**
 * Test that GetSupportedKeySystems returns success if the IPC API succeeds.
 */
TEST_F(RialtoClientMediaKeysCapabilitiesKeySystemsTest, GetSupportedKeySystemsSuccess)
{
    EXPECT_CALL(*m_mediaKeysCapabilitiesIpcMock, getSupportedKeySystems()).WillOnce(Return(m_supportedKeySystems));

    EXPECT_EQ(m_mediaKeysCapabilities->getSupportedKeySystems(), m_supportedKeySystems);
}

/**
 * Test that GetSupportedKeySystems returns failure if the IPC API fails.
 */
TEST_F(RialtoClientMediaKeysCapabilitiesKeySystemsTest, GetSupportedKeySystemsFailure)
{
    EXPECT_CALL(*m_mediaKeysCapabilitiesIpcMock, getSupportedKeySystems()).WillOnce(Return(std::vector<std::string>{}));

    EXPECT_EQ(m_mediaKeysCapabilities->getSupportedKeySystems(), std::vector<std::string>{});
}

/**
 * Test that SupportsKeySystem returns success if the IPC API succeeds.
 */
TEST_F(RialtoClientMediaKeysCapabilitiesKeySystemsTest, SupportsKeySystemSuccess)
{
    EXPECT_CALL(*m_mediaKeysCapabilitiesIpcMock, supportsKeySystem(m_keySystems)).WillOnce(Return(true));

    EXPECT_EQ(m_mediaKeysCapabilities->supportsKeySystem(m_keySystems), true);
}

/**
 * Test that SupportsKeySystem returns failure if the IPC API fails.
 */
TEST_F(RialtoClientMediaKeysCapabilitiesKeySystemsTest, SupportsKeySystemFailure)
{
    EXPECT_CALL(*m_mediaKeysCapabilitiesIpcMock, supportsKeySystem(m_keySystems)).WillOnce(Return(false));

    EXPECT_EQ(m_mediaKeysCapabilities->supportsKeySystem(m_keySystems), false);
}

/**
 * Test that GetSupportedKeySystemVersion returns success if the IPC API succeeds.
 */
TEST_F(RialtoClientMediaKeysCapabilitiesKeySystemsTest, GetSupportedKeySystemVersionSuccess)
{
    std::string returnVersion = "";

    EXPECT_CALL(*m_mediaKeysCapabilitiesIpcMock, getSupportedKeySystemVersion(m_keySystems, _))
        .WillOnce(DoAll(SetArgReferee<1>(m_version), Return(true)));

    EXPECT_EQ(m_mediaKeysCapabilities->getSupportedKeySystemVersion(m_keySystems, returnVersion), true);
    EXPECT_EQ(returnVersion, m_version);
}

/**
 * Test that GetSupportedKeySystemVersion returns failure if the IPC API fails.
 */
TEST_F(RialtoClientMediaKeysCapabilitiesKeySystemsTest, GetSupportedKeySystemVersionFailure)
{
    std::string returnVersion = "";

    EXPECT_CALL(*m_mediaKeysCapabilitiesIpcMock, getSupportedKeySystemVersion(m_keySystems, _))
        .WillOnce(DoAll(SetArgReferee<1>(m_version), Return(false)));

    EXPECT_EQ(m_mediaKeysCapabilities->getSupportedKeySystemVersion(m_keySystems, returnVersion), false);
}
