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

#include "ServerManagerServiceTestsFixture.h"
#include "gtest/gtest.h"
#include "RialtoLogging.h"
#include "ServerManagerService.h"

namespace
{
const std::string APP_NAME{"YouTube"};
const firebolt::rialto::common::SessionServerState APP_STATE{firebolt::rialto::common::SessionServerState::INACTIVE};
const std::string APP_SOCKET{getenv("RIALTO_SOCKET_PATH")};
const firebolt::rialto::common::AppConfig APP_CONFIG{APP_SOCKET};
} // namespace

TEST_F(ServerManagerServiceTests, initiateApplicationShouldReturnTrueIfOperationSucceeded)
{
    initiateApplicationWillBeCalled(APP_NAME, APP_STATE, APP_CONFIG, true);
    ASSERT_TRUE(triggerInitiateApplication(APP_NAME, APP_STATE, APP_CONFIG));
}

TEST_F(ServerManagerServiceTests, initiateApplicationShouldReturnFalseIfOperationFailed)
{
    initiateApplicationWillBeCalled(APP_NAME, APP_STATE, APP_CONFIG, false);
    ASSERT_FALSE(triggerInitiateApplication(APP_NAME, APP_STATE, APP_CONFIG));
}

TEST_F(ServerManagerServiceTests, setStateShouldReturnTrueIfOperationSucceeded)
{
    setSessionServerStateWillBeCalled(APP_NAME, APP_STATE, true);
    ASSERT_TRUE(triggerChangeSessionServerState(APP_NAME, APP_STATE));
}

TEST_F(ServerManagerServiceTests, setStateShouldReturnFalseIfOperationFailed)
{
    setSessionServerStateWillBeCalled(APP_NAME, APP_STATE, false);
    ASSERT_FALSE(triggerChangeSessionServerState(APP_NAME, APP_STATE));
}

TEST_F(ServerManagerServiceTests, getSessionServerInfoShouldReturnAppSocket)
{
    getAppConnectionInfoWillBeCalled(APP_NAME, APP_SOCKET);
    EXPECT_EQ(triggerGetAppConnectionInfo(APP_NAME), APP_SOCKET);
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
    firebolt::rialto::logging::setLogLevels(RIALTO_COMPONENT_SERVER_MANAGER, static_cast<RIALTO_DEBUG_LEVEL>(RIALTO_DEBUG_LEVEL_DEFAULT | RIALTO_DEBUG_LEVEL_DEBUG | RIALTO_DEBUG_LEVEL_INFO));
    EXPECT_TRUE(triggerRegisterLogHandler(configureLogHandler()));
}

TEST_F(ServerManagerServiceTests, registerLogHandlerShouldFailWhenPtrIsNull)
{
    EXPECT_FALSE(triggerRegisterLogHandler(nullptr));
}
