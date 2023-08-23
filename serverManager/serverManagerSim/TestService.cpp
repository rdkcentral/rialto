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

#include "TestService.h"
#include "HttpRequest.h"
#include "RialtoLogging.h"
#include "ServerManagerServiceFactory.h"
#include "commands/CommandFactory.h"

namespace
{
constexpr char SERVER_URL[]{"0.0.0.0:9008"};

void handleRequest(mg_connection *conn, const mg_request_info *request_info, void *user_data)
{
    auto *testService = reinterpret_cast<rialto::servermanager::TestService *>(user_data);
    rialto::servermanager::HttpRequest request{conn, request_info};
    auto command = rialto::servermanager::createCommand(*testService, request);
    command->run();
}
} // namespace

namespace rialto::servermanager
{
TestService::TestService(const firebolt::rialto::common::ServerManagerConfig &config)
    : m_stateObserver{std::make_shared<StateObserver>()},
      m_serverManagerService{rialto::servermanager::service::create(m_stateObserver, config)},
      m_mgServerCtx{mg_start()}, m_isStopped{false}
{
    mg_set_option(m_mgServerCtx, "ports", SERVER_URL);
    mg_bind_to_uri(m_mgServerCtx, "*", &handleRequest, this);
}

TestService::~TestService()
{
    mg_stop(m_mgServerCtx);
}

void TestService::run()
{
    std::unique_lock<std::mutex> lock{m_serverMutex};
    m_serverCv.wait(lock, [this] { return m_isStopped; });
}

void TestService::shutdown()
{
    std::unique_lock<std::mutex> lock{m_serverMutex};
    m_isStopped = true;
    m_serverCv.notify_one();
}

bool TestService::setState(const std::string &appName, const firebolt::rialto::common::SessionServerState &state,
                           const firebolt::rialto::common::AppConfig &appConfig)
{
    fprintf(stderr, "______LOGCHECK1");
    if (state == firebolt::rialto::common::SessionServerState::ACTIVE)
    {
        fprintf(stderr, "______LOGCHECK2");
        std::string activeApp = m_stateObserver->getActiveApp();
        if (!activeApp.empty() && activeApp != appName &&
            !m_serverManagerService->changeSessionServerState(activeApp,
                                                              firebolt::rialto::common::SessionServerState::INACTIVE))
        {
            fprintf(stderr, "Failed to switch active: %s to inactive", activeApp.c_str());
            return false;
        }
    }
    if (getState(appName) == firebolt::rialto::common::SessionServerState::NOT_RUNNING)
    {
        fprintf(stderr, "______LOGCHECK3");
        return m_serverManagerService->initiateApplication(appName, state, appConfig);
    }
    fprintf(stderr, "______LOGCHECK4");
    return m_serverManagerService->changeSessionServerState(appName, state);
}

firebolt::rialto::common::SessionServerState TestService::getState(const std::string &appName)
{
    return m_stateObserver->getCurrentState(appName);
}

std::string TestService::getAppInfo(const std::string &appName)
{
    return m_serverManagerService->getAppConnectionInfo(appName);
}

bool TestService::setLogLevels(const service::LoggingLevels &logLevels)
{
    return m_serverManagerService->setLogLevels(logLevels);
}
} // namespace rialto::servermanager
