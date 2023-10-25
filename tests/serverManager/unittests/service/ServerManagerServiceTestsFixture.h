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

#ifndef SERVER_MANAGER_SERVICE_TESTS_FIXTURE_H_
#define SERVER_MANAGER_SERVICE_TESTS_FIXTURE_H_

#include "IServerManagerService.h"
#include "SessionServerAppManagerMock.h"
#include <gtest/gtest.h>
#include <memory>
#include <string>

using testing::StrictMock;

class ServerManagerServiceTests : public testing::Test
{
public:
    ServerManagerServiceTests();
    virtual ~ServerManagerServiceTests() = default;

    void initiateApplicationWillBeCalled(const std::string &appId,
                                         const firebolt::rialto::common::SessionServerState &state,
                                         const firebolt::rialto::common::AppConfig &appConfig, bool returnValue);
    void setSessionServerStateWillBeCalled(const std::string &appId,
                                           const firebolt::rialto::common::SessionServerState &state, bool returnValue);
    void getAppConnectionInfoWillBeCalled(const std::string &appId, const std::string &returnValue);
    void setLogLevelsWillBeCalled(bool returnValue);
    std::shared_ptr<rialto::servermanager::service::ILogHandler> configureLogHandler();

    bool triggerInitiateApplication(const std::string &appId, const firebolt::rialto::common::SessionServerState &state,
                                    const firebolt::rialto::common::AppConfig &appConfig);
    bool triggerChangeSessionServerState(const std::string &appId,
                                         const firebolt::rialto::common::SessionServerState &state);
    std::string triggerGetAppConnectionInfo(const std::string &appId);
    bool triggerSetLogLevels();
    bool triggerRegisterLogHandler(const std::shared_ptr<rialto::servermanager::service::ILogHandler> &handler);
    void triggerServerManagerLog();

private:
    StrictMock<rialto::servermanager::common::SessionServerAppManagerMock> m_appManager;
    std::unique_ptr<rialto::servermanager::service::IServerManagerService> m_sut;
};

#endif // SERVER_MANAGER_SERVICE_TESTS_FIXTURE_H_
