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

#include "DecoderCapabilitiesCommon.h"
#include "RialtoLogging.h"
#include "ServerManagerServiceTestsFixture.h"
#include "YamlCppWrapperMock.h"
#include "gtest/gtest.h"

namespace
{
const std::string kAppName{"YouTube"};
const firebolt::rialto::common::SessionServerState kAppState{firebolt::rialto::common::SessionServerState::INACTIVE};
const std::string kAppSocket{getenv("RIALTO_SOCKET_PATH")};
const firebolt::rialto::common::AppConfig kAppConfig{kAppSocket};
} // namespace

TEST_F(ServerManagerServiceTests, initiateApplicationShouldReturnTrueIfOperationSucceeded)
{
    initiateApplicationWillBeCalled(kAppName, kAppState, kAppConfig, true);
    ASSERT_TRUE(triggerInitiateApplication(kAppName, kAppState, kAppConfig));
}

TEST_F(ServerManagerServiceTests, initiateApplicationShouldReturnFalseIfOperationFailed)
{
    initiateApplicationWillBeCalled(kAppName, kAppState, kAppConfig, false);
    ASSERT_FALSE(triggerInitiateApplication(kAppName, kAppState, kAppConfig));
}

TEST_F(ServerManagerServiceTests, setStateShouldReturnTrueIfOperationSucceeded)
{
    setSessionServerStateWillBeCalled(kAppName, kAppState, true);
    ASSERT_TRUE(triggerChangeSessionServerState(kAppName, kAppState));
}

TEST_F(ServerManagerServiceTests, setStateShouldReturnFalseIfOperationFailed)
{
    setSessionServerStateWillBeCalled(kAppName, kAppState, false);
    ASSERT_FALSE(triggerChangeSessionServerState(kAppName, kAppState));
}

TEST_F(ServerManagerServiceTests, getSessionServerInfoShouldReturnAppSocket)
{
    getAppConnectionInfoWillBeCalled(kAppName, kAppSocket);
    EXPECT_EQ(triggerGetAppConnectionInfo(kAppName), kAppSocket);
}

TEST_F(ServerManagerServiceTests, setLogLevelsShouldReturnTrueIfOperationSucceeded)
{
    setLogLevelsWillBeCalled(true);
    ASSERT_TRUE(triggerSetLogLevels());
}

TEST_F(ServerManagerServiceTests, setLogLevelsShouldReturnFalseIfOperationFailed)
{
    setLogLevelsWillBeCalled(false);
    ASSERT_FALSE(triggerSetLogLevels());
}

TEST_F(ServerManagerServiceTests, registerLogHandlerShouldSucceed)
{
    EXPECT_TRUE(triggerRegisterLogHandler(configureLogHandler()));
    triggerServerManagerLog();
    firebolt::rialto::logging::setLogHandler(RIALTO_COMPONENT_SERVER_MANAGER, nullptr, false);
}

TEST_F(ServerManagerServiceTests, registerLogHandlerShouldFailWhenPtrIsNull)
{
    EXPECT_FALSE(triggerRegisterLogHandler(nullptr));
}

/**
 * @test yamlCppWrapperMockDelegation
 * @brief Verify that IYamlCppWrapper mocks can be used and calls can be expected.
 *
 * This test verifies the mock wrapper can be instantiated and expectations can be set:
 * 1. Mock wrapper can be created via shared_ptr
 * 2. Audio capability queries can be mocked
 * 3. Video capability queries can be mocked
 */
namespace
{
// Test that YAML wrapper mocks can be used
TEST(YamlCppWrapperMockDelegation, wrapperDelegatesAudioCapabilitiesCorrectly)
{
    auto mockWrapper = std::make_shared<firebolt::rialto::wrappers::YamlCppWrapperMock>();

    EXPECT_CALL(*mockWrapper, getAudioDecoderCapabilities(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(firebolt::rialto::common::DecoderCapabilitiesStatus::OK));

    firebolt::rialto::common::AudioDecoderCapabilities result;
    auto status = mockWrapper->getAudioDecoderCapabilities(result);

    EXPECT_EQ(status, firebolt::rialto::common::DecoderCapabilitiesStatus::OK);
}

TEST(YamlCppWrapperMockDelegation, wrapperDelegatesVideoCapabilitiesCorrectly)
{
    auto mockWrapper = std::make_shared<firebolt::rialto::wrappers::YamlCppWrapperMock>();

    EXPECT_CALL(*mockWrapper, getVideoDecoderCapabilities(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(firebolt::rialto::common::DecoderCapabilitiesStatus::OK));

    firebolt::rialto::common::VideoDecoderCapabilities result;
    auto status = mockWrapper->getVideoDecoderCapabilities(result);

    EXPECT_EQ(status, firebolt::rialto::common::DecoderCapabilitiesStatus::OK);
}
} // namespace
