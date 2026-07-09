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
#include <vector>

using namespace firebolt::rialto::client;

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::WithArgs;

namespace
{
const std::string kKeySystem{"com.netflix.playready"};
const std::vector<std::string> kRobustnessLevels{"HW_SECURE_ALL", "SW_SECURE_CRYPTO"};
} // namespace

class RialtoClientMediaKeysCapabilitiesIpcRobustnessTest : public IpcModuleBase, public ::testing::Test
{
protected:
    std::unique_ptr<IMediaKeysCapabilities> m_mediaKeysCapabilitiesIpc;

    RialtoClientMediaKeysCapabilitiesIpcRobustnessTest()
    {
        expectInitIpc();

        EXPECT_NO_THROW(m_mediaKeysCapabilitiesIpc = std::make_unique<MediaKeysCapabilitiesIpc>(*m_ipcClientMock));
        EXPECT_NE(m_mediaKeysCapabilitiesIpc, nullptr);
    }

public:
    void setRobustnessLevelsInResponse(google::protobuf::Message *response)
    {
        firebolt::rialto::GetSupportedRobustnessLevelsResponse *robustnessResponse =
            dynamic_cast<firebolt::rialto::GetSupportedRobustnessLevelsResponse *>(response);
        for (const auto &level : kRobustnessLevels)
        {
            robustnessResponse->add_robustness_levels(level);
        }
    }
};

/**
 * Test that getSupportedRobustnessLevels succeeds and populates the levels from the IPC response.
 */
TEST_F(RialtoClientMediaKeysCapabilitiesIpcRobustnessTest, GetSupportedRobustnessLevelsSuccess)
{
    std::vector<std::string> levels;
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getSupportedRobustnessLevels"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(
            Invoke(this, &RialtoClientMediaKeysCapabilitiesIpcRobustnessTest::setRobustnessLevelsInResponse)));

    EXPECT_TRUE(m_mediaKeysCapabilitiesIpc->getSupportedRobustnessLevels(kKeySystem, levels));
    EXPECT_EQ(levels, kRobustnessLevels);
}

/**
 * Test that getSupportedRobustnessLevels fails if the ipc channel disconnected.
 */
TEST_F(RialtoClientMediaKeysCapabilitiesIpcRobustnessTest, GetSupportedRobustnessLevelsChannelDisconnected)
{
    std::vector<std::string> levels;
    expectIpcApiCallDisconnected();

    EXPECT_FALSE(m_mediaKeysCapabilitiesIpc->getSupportedRobustnessLevels(kKeySystem, levels));
}

/**
 * Test that getSupportedRobustnessLevels fails if the ipc channel disconnected and succeeds on reconnect.
 */
TEST_F(RialtoClientMediaKeysCapabilitiesIpcRobustnessTest, GetSupportedRobustnessLevelsReconnectChannel)
{
    std::vector<std::string> levels;
    expectIpcApiCallReconnected();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getSupportedRobustnessLevels"), _, _, _, _))
        .WillOnce(WithArgs<3>(
            Invoke(this, &RialtoClientMediaKeysCapabilitiesIpcRobustnessTest::setRobustnessLevelsInResponse)));

    EXPECT_TRUE(m_mediaKeysCapabilitiesIpc->getSupportedRobustnessLevels(kKeySystem, levels));
    EXPECT_EQ(levels, kRobustnessLevels);
}

/**
 * Test that getSupportedRobustnessLevels fails when the ipc controller reports failure.
 */
TEST_F(RialtoClientMediaKeysCapabilitiesIpcRobustnessTest, GetSupportedRobustnessLevelsFailure)
{
    std::vector<std::string> levels;
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getSupportedRobustnessLevels"), _, _, _, _));

    EXPECT_FALSE(m_mediaKeysCapabilitiesIpc->getSupportedRobustnessLevels(kKeySystem, levels));
}
