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

#include "IpcModuleBase.h"
#include "MediaKeysCapabilitiesIpc.h"
#include <gtest/gtest.h>
#include <memory>
#include <string>

using namespace firebolt::rialto::client;
using namespace firebolt::rialto::client::mock;

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::WithArgs;

class RialtoClientMediaKeysCapabilitiesIpcKeySystemsTest : public IpcModuleBase, public ::testing::Test
{
protected:
    std::unique_ptr<IMediaKeysCapabilities> m_mediaKeysCapabilitiesIpc;
    std::string m_keySystem = "keysystem";
    std::string m_version = "version";
    std::vector<std::string> m_supportedKeySystems = {"keysystem1", "keysystem2", "keysystem3"};

    RialtoClientMediaKeysCapabilitiesIpcKeySystemsTest()
    {
        expectInitIpc();

        EXPECT_NO_THROW(m_mediaKeysCapabilitiesIpc = std::make_unique<MediaKeysCapabilitiesIpc>(m_ipcClientFactoryMock));
        EXPECT_NE(m_mediaKeysCapabilitiesIpc, nullptr);
    }

public:
    void setGetSupportedKeySystemsResponse(google::protobuf::Message *response)
    {
        firebolt::rialto::GetSupportedKeySystemsResponse *getSupportedKeySystemResponse =
            dynamic_cast<firebolt::rialto::GetSupportedKeySystemsResponse *>(response);
        for (auto it = m_supportedKeySystems.begin(); it != m_supportedKeySystems.end(); it++)
        {
            getSupportedKeySystemResponse->add_key_systems(*it);
        }
    }

    void setSupportsKeySystemSupportedResponse(google::protobuf::Message *response)
    {
        firebolt::rialto::SupportsKeySystemResponse *supportsKeySystemResponse =
            dynamic_cast<firebolt::rialto::SupportsKeySystemResponse *>(response);
        supportsKeySystemResponse->set_is_supported(true);
    }

    void setSupportsKeySystemNotSupportedResponse(google::protobuf::Message *response)
    {
        firebolt::rialto::SupportsKeySystemResponse *supportsKeySystemResponse =
            dynamic_cast<firebolt::rialto::SupportsKeySystemResponse *>(response);
        supportsKeySystemResponse->set_is_supported(false);
    }

    void setGetSupportedKeySystemVersionResponse(google::protobuf::Message *response)
    {
        firebolt::rialto::GetSupportedKeySystemVersionResponse *getSupportedKeySystemVersionResponse =
            dynamic_cast<firebolt::rialto::GetSupportedKeySystemVersionResponse *>(response);
        getSupportedKeySystemVersionResponse->set_version(m_version);
    }
};

/**
 * Test that GetSupportedKeySystems can be called successfully.
 */
TEST_F(RialtoClientMediaKeysCapabilitiesIpcKeySystemsTest, GetSupportedKeySystemSuccess)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getSupportedKeySystems"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(
            Invoke(this, &RialtoClientMediaKeysCapabilitiesIpcKeySystemsTest::setGetSupportedKeySystemsResponse)));

    EXPECT_EQ(m_mediaKeysCapabilitiesIpc->getSupportedKeySystems(), m_supportedKeySystems);
}

/**
 * Test that GetSupportedKeySystems fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaKeysCapabilitiesIpcKeySystemsTest, GetSupportedKeySystemsChannelDisconnected)
{
    expectIpcApiCallDisconnected();

    EXPECT_EQ(m_mediaKeysCapabilitiesIpc->getSupportedKeySystems(), std::vector<std::string>{});
}

/**
 * Test that GetSupportedKeySystems fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaKeysCapabilitiesIpcKeySystemsTest, GetSupportedKeySystemsReconnectChannel)
{
    expectIpcApiCallReconnected();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getSupportedKeySystems"), _, _, _, _))
        .WillOnce(WithArgs<3>(
            Invoke(this, &RialtoClientMediaKeysCapabilitiesIpcKeySystemsTest::setGetSupportedKeySystemsResponse)));

    EXPECT_EQ(m_mediaKeysCapabilitiesIpc->getSupportedKeySystems(), m_supportedKeySystems);
}

/**
 * Test that GetSupportedKeySystems fails when ipc fails.
 */
TEST_F(RialtoClientMediaKeysCapabilitiesIpcKeySystemsTest, GetSupportedKeySystemsFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getSupportedKeySystems"), _, _, _, _));

    EXPECT_EQ(m_mediaKeysCapabilitiesIpc->getSupportedKeySystems(), std::vector<std::string>{});
}

/**
 * Test that SupportsKeySystem can be called successfully and ipc to return that the ketsystem is supported.
 */
TEST_F(RialtoClientMediaKeysCapabilitiesIpcKeySystemsTest, SupportsKeySystemSuccessSupported)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("supportsKeySystem"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(
            Invoke(this, &RialtoClientMediaKeysCapabilitiesIpcKeySystemsTest::setSupportsKeySystemSupportedResponse)));

    EXPECT_EQ(m_mediaKeysCapabilitiesIpc->supportsKeySystem(m_keySystem), true);
}

/**
 * Test that SupportsKeySystem can be called successfully and ipc to return that the ketsystem is not supported.
 */
TEST_F(RialtoClientMediaKeysCapabilitiesIpcKeySystemsTest, SupportsKeySystemSuccessNotSupported)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("supportsKeySystem"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(
            Invoke(this, &RialtoClientMediaKeysCapabilitiesIpcKeySystemsTest::setSupportsKeySystemNotSupportedResponse)));

    EXPECT_EQ(m_mediaKeysCapabilitiesIpc->supportsKeySystem(m_keySystem), false);
}

/**
 * Test that SupportsKeySystem fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaKeysCapabilitiesIpcKeySystemsTest, SupportsKeySystemChannelDisconnected)
{
    expectIpcApiCallDisconnected();

    EXPECT_EQ(m_mediaKeysCapabilitiesIpc->supportsKeySystem(m_keySystem), false);
}

/**
 * Test that SupportsKeySystem fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaKeysCapabilitiesIpcKeySystemsTest, SupportsKeySystemReconnectChannel)
{
    expectIpcApiCallReconnected();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("supportsKeySystem"), _, _, _, _))
        .WillOnce(WithArgs<3>(
            Invoke(this, &RialtoClientMediaKeysCapabilitiesIpcKeySystemsTest::setSupportsKeySystemSupportedResponse)));

    EXPECT_EQ(m_mediaKeysCapabilitiesIpc->supportsKeySystem(m_keySystem), true);
}

/**
 * Test that SupportsKeySystem fails when ipc fails.
 */
TEST_F(RialtoClientMediaKeysCapabilitiesIpcKeySystemsTest, SupportsKeySystemFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("supportsKeySystem"), _, _, _, _));

    EXPECT_EQ(m_mediaKeysCapabilitiesIpc->supportsKeySystem(m_keySystem), false);
}

/**
 * Test that GetSupportedKeySystemVersion can be called successfully.
 */
TEST_F(RialtoClientMediaKeysCapabilitiesIpcKeySystemsTest, GetSupportedKeySystemVersionSuccess)
{
    std::string returnVersion;

    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getSupportedKeySystemVersion"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(
            Invoke(this, &RialtoClientMediaKeysCapabilitiesIpcKeySystemsTest::setGetSupportedKeySystemVersionResponse)));

    EXPECT_EQ(m_mediaKeysCapabilitiesIpc->getSupportedKeySystemVersion(m_keySystem, returnVersion), true);
    EXPECT_EQ(returnVersion, m_version);
}

/**
 * Test that GetSupportedKeySystemVersion fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaKeysCapabilitiesIpcKeySystemsTest, GetSupportedKeySystemVersionChannelDisconnected)
{
    std::string returnVersion;

    expectIpcApiCallDisconnected();

    EXPECT_EQ(m_mediaKeysCapabilitiesIpc->getSupportedKeySystemVersion(m_keySystem, returnVersion), false);
}

/**
 * Test that GetSupportedKeySystemVersion fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaKeysCapabilitiesIpcKeySystemsTest, GetSupportedKeySystemVersionReconnectChannel)
{
    std::string returnVersion;

    expectIpcApiCallReconnected();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getSupportedKeySystemVersion"), _, _, _, _))
        .WillOnce(WithArgs<3>(
            Invoke(this, &RialtoClientMediaKeysCapabilitiesIpcKeySystemsTest::setGetSupportedKeySystemVersionResponse)));

    EXPECT_EQ(m_mediaKeysCapabilitiesIpc->getSupportedKeySystemVersion(m_keySystem, returnVersion), true);
    EXPECT_EQ(returnVersion, m_version);
}

/**
 * Test that GetSupportedKeySystemVersion fails when ipc fails.
 */
TEST_F(RialtoClientMediaKeysCapabilitiesIpcKeySystemsTest, GetSupportedKeySystemVersionFailure)
{
    std::string returnVersion;

    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getSupportedKeySystemVersion"), _, _, _, _));

    EXPECT_EQ(m_mediaKeysCapabilitiesIpc->getSupportedKeySystemVersion(m_keySystem, returnVersion), false);
}
