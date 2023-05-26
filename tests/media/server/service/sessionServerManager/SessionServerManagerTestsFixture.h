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

#ifndef SESSION_SERVER_MANAGER_TESTS_FIXTURE_H_
#define SESSION_SERVER_MANAGER_TESTS_FIXTURE_H_

#include "ApplicationManagementServerMock.h"
#include "CdmServiceMock.h"
#include "ControlServiceMock.h"
#include "ISessionServerManager.h"
#include "IpcFactoryMock.h"
#include "PlaybackServiceMock.h"
#include "SessionManagementServerMock.h"
#include <gtest/gtest.h>
#include <memory>
#include <thread>

using testing::StrictMock;

class SessionServerManagerTests : public testing::Test
{
public:
    SessionServerManagerTests();
    ~SessionServerManagerTests() override;

    void willNotInitializeWithWrongNumberOfArgs();
    void willNotInitializeWithWrongSocket();
    void willNotInitializeWhenApplicationManagementServerFailsToInit();
    void willNotInitializeWhenApplicationManagementServerThrows();
    void willNotInitializeWhenApplicationManagementServerFailsToSendEvent();
    void willInitialize();
    void willFailToSetConfigurationWhenSessionManagementServerFailsToInit();
    void willFailToSetConfigurationWhenSessionManagementServerFailsToSetInitialState();
    void willSetConfiguration();
    void willFailToSetUnsupportedState();
    void willFailToSetStateActiveDueToPlaybackServiceError();
    void willFailToSetStateActiveDueToCdmServiceError();
    void willFailToSetStateActiveDueToSessionServerError();
    void willSetStateActive();
    void willFailToSetStateInactive();
    void willFailToSetStateInactiveAndGoBackToActive();
    void willSetStateInactive();
    void willFailToSetStateNotRunning();
    void willSetStateNotRunning();
    void willSetLogLevels();
    void willPing();
    void willFailToPing();

    void setStateShouldFail(const firebolt::rialto::common::SessionServerState &state);
    void setStateShouldSucceed(const firebolt::rialto::common::SessionServerState &state);
    void triggerStartService();
    void triggerSetLogLevels();
    void pingShouldSucceed();
    void pingShouldFail();

private:
    std::unique_ptr<firebolt::rialto::server::ipc::IApplicationManagementServer> m_applicationManagementServer;
    StrictMock<firebolt::rialto::server::ipc::ApplicationManagementServerMock> &m_applicationManagementServerMock;
    StrictMock<firebolt::rialto::server::ipc::IpcFactoryMock> m_ipcFactoryMock;
    StrictMock<firebolt::rialto::server::service::PlaybackServiceMock> m_playbackServiceMock;
    StrictMock<firebolt::rialto::server::service::CdmServiceMock> m_cdmServiceMock;
    StrictMock<firebolt::rialto::server::service::ControlServiceMock> m_controlServiceMock;
    std::unique_ptr<firebolt::rialto::server::ipc::ISessionManagementServer> m_sessionManagementServer;
    StrictMock<firebolt::rialto::server::ipc::SessionManagementServerMock> &m_sessionManagementServerMock;
    std::thread m_serviceThread;
    std::unique_ptr<firebolt::rialto::server::service::ISessionServerManager> m_sut;
};

#endif // SESSION_SERVER_MANAGER_TESTS_FIXTURE_H_
