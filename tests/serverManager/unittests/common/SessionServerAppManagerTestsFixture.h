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

#ifndef SESSION_SERVER_APP_MANAGER_TESTS_FIXTURE_H_
#define SESSION_SERVER_APP_MANAGER_TESTS_FIXTURE_H_

#include "ControllerMock.h"
#include "EventThreadFactoryMock.h"
#include "EventThreadMock.h"
#include "HealthcheckServiceFactoryMock.h"
#include "HealthcheckServiceMock.h"
#include "ISessionServerAppManager.h"
#include "SessionServerAppFactoryMock.h"
#include "SessionServerAppMock.h"
#include "StateObserverMock.h"
#include <condition_variable>
#include <gtest/gtest.h>
#include <memory>
#include <mutex>
#include <string>

using testing::StrictMock;

class SessionServerAppManagerTests : public testing::Test
{
public:
    SessionServerAppManagerTests();
    virtual ~SessionServerAppManagerTests() = default;

    void sessionServerLaunchWillFail(const firebolt::rialto::common::SessionServerState &state);
    void preloadedSessionServerLaunchWillFail();
    void sessionServerConnectWillFail(const firebolt::rialto::common::SessionServerState &state);
    void preloadedSessionServerConnectWillFail();
    void sessionServerChangeStateWillFail(const firebolt::rialto::common::SessionServerState &state);
    void sessionServerWillLaunch(const firebolt::rialto::common::SessionServerState &state);
    void preloadedSessionServerWillLaunch();
    void preloadedSessionServerWillFailToConfigure(const firebolt::rialto::common::SessionServerState &state);
    void preloadedSessionServerWillBeConfigured(const firebolt::rialto::common::SessionServerState &state);
    void preloadedSessionServerWillCloseWithError();
    void sessionServerWillChangeState(const firebolt::rialto::common::SessionServerState &state);
    void sessionServerWillReturnAppSocketName(const std::string &socketName);
    void sessionServerWillChangeStateToUninitialized();
    void preloadedSessionServerWillChangeStateToUninitialized();
    void sessionServerWillChangeStateToInactive();
    void preloadedSessionServerWillSetConfiguration();
    void sessionServerWillFailToSetConfiguration();
    void preloadedSessionServerWillFailToSetConfiguration();
    void sessionServerWillSetLogLevels();
    void sessionServerWillFailToSetLogLevels();
    void sessionServerWillKillRunningApplication();
    void sessionServerWontBePreloaded();
    void newSessionServerWillBeLaunched();
    void healthcheckServiceWillHandleAck(bool success);
    void pingWillBeSentToRunningApps();
    void pingSendToRunningAppsWillFail();
    void clientWillBeRemoved();
    void sessionServerWillIndicateStateChange(const firebolt::rialto::common::SessionServerState &state);
    void sessionServerWillBeRestarted(const firebolt::rialto::common::SessionServerState &state);
    void sessionServerWillRestartWillBeSkipped();

    void triggerPreloadSessionServers();
    bool triggerInitiateApplication(const firebolt::rialto::common::SessionServerState &state);
    bool triggerSetSessionServerState(const firebolt::rialto::common::SessionServerState &newState);
    void triggerOnSessionServerStateChanged(const firebolt::rialto::common::SessionServerState &newState);
    void triggerOnAck(bool success);
    std::string triggerGetAppConnectionInfo();
    bool triggerSetLogLevel();
    void triggerSendPingEvents();
    void triggerRestartServer();

private:
    std::unique_ptr<rialto::servermanager::ipc::IController> m_controller;
    std::shared_ptr<StrictMock<rialto::servermanager::service::StateObserverMock>> m_stateObserver;
    std::unique_ptr<rialto::servermanager::common::ISessionServerApp> m_sessionServerApp;
    std::unique_ptr<rialto::servermanager::common::ISessionServerApp> m_secondSessionServerApp;
    std::unique_ptr<rialto::servermanager::common::ISessionServerAppFactory> m_sessionServerAppFactory;
    std::unique_ptr<rialto::servermanager::common::IHealthcheckServiceFactory> m_healthcheckServiceFactory;
    std::unique_ptr<rialto::servermanager::common::IHealthcheckService> m_healthcheckService;
    StrictMock<rialto::servermanager::ipc::ControllerMock> &m_controllerMock;
    StrictMock<rialto::servermanager::common::SessionServerAppMock> &m_sessionServerAppMock;
    StrictMock<rialto::servermanager::common::SessionServerAppFactoryMock> &m_sessionServerAppFactoryMock;
    StrictMock<rialto::servermanager::common::HealthcheckServiceFactoryMock> &m_healthcheckServiceFactoryMock;
    StrictMock<rialto::servermanager::common::HealthcheckServiceMock> &m_healthcheckServiceMock;
    std::unique_ptr<rialto::servermanager::common::ISessionServerAppManager> m_sut;
};

#endif // SESSION_SERVER_APP_MANAGER_TESTS_FIXTURE_H_
