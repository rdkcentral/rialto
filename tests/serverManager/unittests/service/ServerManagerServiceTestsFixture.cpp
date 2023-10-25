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
#include "LogHandlerMock.h"
#include "Matchers.h"
#include "RialtoServerManagerLogging.h"
#include "ServerManagerService.h"
#include "ServiceContextMock.h"
#include <string>
#include <utility>

using testing::_;
using testing::AtLeast;
using testing::Return;
using testing::ReturnRef;

namespace
{
constexpr unsigned kNumOfPreloadedServers{2};
const std::string kLogText{"RialtoServerManager Test Log"};
} // namespace

ServerManagerServiceTests::ServerManagerServiceTests()
{
    auto serviceContext{std::make_unique<StrictMock<rialto::servermanager::service::ServiceContextMock>>()};
    EXPECT_CALL(*serviceContext, getSessionServerAppManager).Times(AtLeast(0)).WillRepeatedly(ReturnRef(m_appManager));
    EXPECT_CALL(m_appManager, preloadSessionServers(kNumOfPreloadedServers));
    m_sut = std::make_unique<rialto::servermanager::service::ServerManagerService>(std::move(serviceContext),
                                                                                   kNumOfPreloadedServers);
}

void ServerManagerServiceTests::initiateApplicationWillBeCalled(const std::string &appId,
                                                                const firebolt::rialto::common::SessionServerState &state,
                                                                const firebolt::rialto::common::AppConfig &appConfig,
                                                                bool returnValue)
{
    EXPECT_CALL(m_appManager, initiateApplication(appId, state, appConfig)).WillOnce(Return(returnValue));
}

void ServerManagerServiceTests::setSessionServerStateWillBeCalled(
    const std::string &appId, const firebolt::rialto::common::SessionServerState &state, bool returnValue)
{
    EXPECT_CALL(m_appManager, setSessionServerState(appId, state)).WillOnce(Return(returnValue));
}

void ServerManagerServiceTests::getAppConnectionInfoWillBeCalled(const std::string &appId, const std::string &returnValue)
{
    EXPECT_CALL(m_appManager, getAppConnectionInfo(appId)).WillOnce(Return(returnValue));
}

void ServerManagerServiceTests::setLogLevelsWillBeCalled(bool returnValue)
{
    EXPECT_CALL(m_appManager, setLogLevels(_)).WillOnce(Return(returnValue));
}

std::shared_ptr<rialto::servermanager::service::ILogHandler> ServerManagerServiceTests::configureLogHandler()
{
    auto logHandler = std::make_shared<StrictMock<rialto::servermanager::service::LogHandlerMock>>();
    // Expect print in destructor:
    EXPECT_CALL(*logHandler, log(rialto::servermanager::service::ILogHandler::Level::Info,
                                 "ServerManagerServiceTestsFixture.cpp", 117, "triggerServerManagerLog", kLogText));
    return logHandler;
}

bool ServerManagerServiceTests::triggerInitiateApplication(const std::string &appId,
                                                           const firebolt::rialto::common::SessionServerState &state,
                                                           const firebolt::rialto::common::AppConfig &appConfig)
{
    EXPECT_TRUE(m_sut);
    return m_sut->initiateApplication(appId, state, appConfig);
}

bool ServerManagerServiceTests::triggerChangeSessionServerState(const std::string &appId,
                                                                const firebolt::rialto::common::SessionServerState &state)
{
    EXPECT_TRUE(m_sut);
    return m_sut->changeSessionServerState(appId, state);
}

std::string ServerManagerServiceTests::triggerGetAppConnectionInfo(const std::string &appId)
{
    EXPECT_TRUE(m_sut);
    return m_sut->getAppConnectionInfo(appId);
}

bool ServerManagerServiceTests::triggerSetLogLevels()
{
    EXPECT_TRUE(m_sut);
    return m_sut->setLogLevels(rialto::servermanager::service::LoggingLevels());
}

bool ServerManagerServiceTests::triggerRegisterLogHandler(
    const std::shared_ptr<rialto::servermanager::service::ILogHandler> &handler)
{
    EXPECT_TRUE(m_sut);
    return m_sut->registerLogHandler(handler);
}

void ServerManagerServiceTests::triggerServerManagerLog()
{
    RIALTO_SERVER_MANAGER_LOG_INFO("%s", kLogText.c_str());
}
