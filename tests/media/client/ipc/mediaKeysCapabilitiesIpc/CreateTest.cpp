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

using ::testing::Return;

class RialtoClientCreateMediaKeysCapabilitiesIpcTest : public IpcModuleBase, public ::testing::Test
{
protected:
    std::unique_ptr<IMediaKeysCapabilities> m_mediaKeysCapabilitiesIpc;
};

/**
 * Test that a MediaKeysCapabilitiesIpc object can be created successfully.
 */
TEST_F(RialtoClientCreateMediaKeysCapabilitiesIpcTest, Create)
{
    expectInitIpc();

    EXPECT_NO_THROW(m_mediaKeysCapabilitiesIpc = std::make_unique<MediaKeysCapabilitiesIpc>(m_ipcClientFactoryMock));
    EXPECT_NE(m_mediaKeysCapabilitiesIpc, nullptr);
}

/**
 * Test that a MediaKeysCapabilitiesIpc object not created when the client has not been created.
 */
TEST_F(RialtoClientCreateMediaKeysCapabilitiesIpcTest, CreateNoIpcClient)
{
    EXPECT_CALL(*m_ipcClientFactoryMock, getIpcClient()).WillOnce(Return(nullptr));

    EXPECT_THROW(m_mediaKeysCapabilitiesIpc = std::make_unique<MediaKeysCapabilitiesIpc>(m_ipcClientFactoryMock),
                 std::runtime_error);
}

/**
 * Test that a MediaKeysCapabilitiesIpc object not created when the ipc channel has not been created.
 */
TEST_F(RialtoClientCreateMediaKeysCapabilitiesIpcTest, CreateNoIpcChannel)
{
    EXPECT_CALL(*m_ipcClientFactoryMock, getIpcClient()).WillOnce(Return(m_ipcClientMock));
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(nullptr));

    EXPECT_THROW(m_mediaKeysCapabilitiesIpc = std::make_unique<MediaKeysCapabilitiesIpc>(m_ipcClientFactoryMock),
                 std::runtime_error);
}

/**
 * Test that a MediaKeysCapabilitiesIpc object not created when the ipc channel is not connected.
 */
TEST_F(RialtoClientCreateMediaKeysCapabilitiesIpcTest, CreateIpcChannelDisconnected)
{
    EXPECT_CALL(*m_ipcClientFactoryMock, getIpcClient()).WillOnce(Return(m_ipcClientMock));
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock));
    EXPECT_CALL(*m_channelMock, isConnected()).WillOnce(Return(false));

    EXPECT_THROW(m_mediaKeysCapabilitiesIpc = std::make_unique<MediaKeysCapabilitiesIpc>(m_ipcClientFactoryMock),
                 std::runtime_error);
}
