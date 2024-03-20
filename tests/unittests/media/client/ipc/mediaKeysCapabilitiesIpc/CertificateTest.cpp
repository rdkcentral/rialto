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

#include "IpcModuleBase.h"
#include "MediaKeysCapabilitiesIpc.h"
#include <gtest/gtest.h>
#include <memory>
#include <string>

using namespace firebolt::rialto::client;

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::WithArgs;

class RialtoClientMediaKeysCapabilitiesIpcCertificateTest : public IpcModuleBase, public ::testing::Test
{
protected:
    std::unique_ptr<IMediaKeysCapabilities> m_mediaKeysCapabilitiesIpc;

    RialtoClientMediaKeysCapabilitiesIpcCertificateTest()
    {
        expectInitIpc();

        EXPECT_NO_THROW(m_mediaKeysCapabilitiesIpc = std::make_unique<MediaKeysCapabilitiesIpc>(*m_ipcClientMock));
        EXPECT_NE(m_mediaKeysCapabilitiesIpc, nullptr);
    }

public:
    void setServerCertificateSupported(google::protobuf::Message *response)
    {
        firebolt::rialto::IsServerCertificateSupportedResponse *isServerCertificateSupportedResponse =
            dynamic_cast<firebolt::rialto::IsServerCertificateSupportedResponse *>(response);
        isServerCertificateSupportedResponse->set_is_supported(true);
    }

    void setServerCertificateNotSupported(google::protobuf::Message *response)
    {
        firebolt::rialto::IsServerCertificateSupportedResponse *isServerCertificateSupportedResponse =
            dynamic_cast<firebolt::rialto::IsServerCertificateSupportedResponse *>(response);
        isServerCertificateSupportedResponse->set_is_supported(false);
    }
};

/**
 * Test that IsServerCertificateSupported can be called successfully and ipc to return that the certificate is supported.
 */
TEST_F(RialtoClientMediaKeysCapabilitiesIpcCertificateTest, SupportsKeySystemSuccessSupported)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("isServerCertificateSupported"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(
            Invoke(this, &RialtoClientMediaKeysCapabilitiesIpcCertificateTest::setServerCertificateSupported)));

    EXPECT_TRUE(m_mediaKeysCapabilitiesIpc->isServerCertificateSupported());
}

/**
 * Test that IsServerCertificateSupported can be called successfully and ipc to return that the certificate is not supported.
 */
TEST_F(RialtoClientMediaKeysCapabilitiesIpcCertificateTest, SupportsKeySystemSuccessNotSupported)
{
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("isServerCertificateSupported"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(
            Invoke(this, &RialtoClientMediaKeysCapabilitiesIpcCertificateTest::setServerCertificateNotSupported)));

    EXPECT_FALSE(m_mediaKeysCapabilitiesIpc->isServerCertificateSupported());
}

/**
 * Test that IsServerCertificateSupported fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaKeysCapabilitiesIpcCertificateTest, SupportsKeySystemChannelDisconnected)
{
    expectIpcApiCallDisconnected();

    EXPECT_FALSE(m_mediaKeysCapabilitiesIpc->isServerCertificateSupported());
}

/**
 * Test that IsServerCertificateSupported fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(RialtoClientMediaKeysCapabilitiesIpcCertificateTest, SupportsKeySystemReconnectChannel)
{
    expectIpcApiCallReconnected();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("isServerCertificateSupported"), _, _, _, _))
        .WillOnce(WithArgs<3>(
            Invoke(this, &RialtoClientMediaKeysCapabilitiesIpcCertificateTest::setServerCertificateSupported)));

    EXPECT_TRUE(m_mediaKeysCapabilitiesIpc->isServerCertificateSupported());
}

/**
 * Test that IsServerCertificateSupported fails when ipc fails.
 */
TEST_F(RialtoClientMediaKeysCapabilitiesIpcCertificateTest, SupportsKeySystemFailure)
{
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("isServerCertificateSupported"), _, _, _, _));

    EXPECT_FALSE(m_mediaKeysCapabilitiesIpc->isServerCertificateSupported());
}
