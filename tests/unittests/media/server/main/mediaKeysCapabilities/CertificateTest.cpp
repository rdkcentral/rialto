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
#include <array>
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
} // namespace

class RialtoServerMediaKeysCapabilitiesCertificateTest : public ::testing::Test
{
protected:
    std::shared_ptr<StrictMock<OcdmFactoryMock>> m_ocdmFactoryMock;
    std::shared_ptr<StrictMock<OcdmMock>> m_ocdmMock;
    std::shared_ptr<StrictMock<OcdmSystemFactoryMock>> m_ocdmSystemFactoryMock;
    std::shared_ptr<StrictMock<OcdmSystemMock>> m_ocdmSystem;
    StrictMock<OcdmSystemMock> *m_ocdmSystemMock;
    std::shared_ptr<IMediaKeysCapabilities> m_mediaKeysCapabilities;

    RialtoServerMediaKeysCapabilitiesCertificateTest()
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
 * Test that a isServerCertificateSupported returns true if certificate is supported.
 */
TEST_F(RialtoServerMediaKeysCapabilitiesCertificateTest, SupportsCertificate)
{
    EXPECT_CALL(*m_ocdmSystemFactoryMock, createOcdmSystem(kKeySystem)).WillOnce(Return(ByMove(std::move(m_ocdmSystem))));
    EXPECT_CALL(*m_ocdmSystemMock, supportsServerCertificate()).WillOnce(Return(true));

    EXPECT_TRUE(m_mediaKeysCapabilities->isServerCertificateSupported(kKeySystem));
}

/**
 * Test that a isServerCertificateSupported returns false if certificate is not supported.
 */
TEST_F(RialtoServerMediaKeysCapabilitiesCertificateTest, DoesNotSupportCertificate)
{
    EXPECT_CALL(*m_ocdmSystemFactoryMock, createOcdmSystem(kKeySystem)).WillOnce(Return(ByMove(std::move(m_ocdmSystem))));
    EXPECT_CALL(*m_ocdmSystemMock, supportsServerCertificate()).WillOnce(Return(false));

    EXPECT_FALSE(m_mediaKeysCapabilities->isServerCertificateSupported(kKeySystem));
}

/**
 * Test that a isServerCertificateSupported returns false if failure to create the ocdm system object.
 */
TEST_F(RialtoServerMediaKeysCapabilitiesCertificateTest, OcdmSystemFailure)
{
    EXPECT_CALL(*m_ocdmSystemFactoryMock, createOcdmSystem(kKeySystem)).WillOnce(Return(ByMove(nullptr)));
    EXPECT_FALSE(m_mediaKeysCapabilities->isServerCertificateSupported(kKeySystem));
}
