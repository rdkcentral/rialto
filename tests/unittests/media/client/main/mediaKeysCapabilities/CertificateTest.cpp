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
#include "MediaKeysCapabilitiesIpcFactoryMock.h"
#include "MediaKeysCapabilitiesIpcMock.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::client;

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::StrictMock;

class RialtoClientMediaKeysCapabilitiesCertificateTest : public ::testing::Test
{
protected:
    std::shared_ptr<IMediaKeysCapabilities> m_mediaKeysCapabilities;
    std::shared_ptr<StrictMock<MediaKeysCapabilitiesIpcMock>> m_mediaKeysCapabilitiesIpcMock;

    RialtoClientMediaKeysCapabilitiesCertificateTest()
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
 * Test that isServerCertificateSupported returns success if the IPC API succeeds.
 */
TEST_F(RialtoClientMediaKeysCapabilitiesCertificateTest, GetSupportedKeySystemsSuccess)
{
    EXPECT_CALL(*m_mediaKeysCapabilitiesIpcMock, isServerCertificateSupported()).WillOnce(Return(true));

    EXPECT_TRUE(m_mediaKeysCapabilities->isServerCertificateSupported());
}

/**
 * Test that isServerCertificateSupported returns failure if the IPC API fails.
 */
TEST_F(RialtoClientMediaKeysCapabilitiesCertificateTest, GetSupportedKeySystemsFailure)
{
    EXPECT_CALL(*m_mediaKeysCapabilitiesIpcMock, isServerCertificateSupported()).WillOnce(Return(false));

    EXPECT_FALSE(m_mediaKeysCapabilities->isServerCertificateSupported());
}
