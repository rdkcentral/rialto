/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Sky UK
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

#ifndef SESSION_SERVER_APP_TESTS_FIXTURE_H_
#define SESSION_SERVER_APP_TESTS_FIXTURE_H_

#include "LinuxWrapperMock.h"
#include "NamedSocketMock.h"
#include "SessionServerApp.h"
#include "SessionServerAppManagerMock.h"
#include "TimerFactoryMock.h"
#include "TimerMock.h"
#include <gtest/gtest.h>
#include <memory>

class SessionServerAppTests : public testing::Test
{
public:
    SessionServerAppTests() = default;
    ~SessionServerAppTests() override = default;

    void createPreloadedAppSut();
    void createAppSut(const firebolt::rialto::common::AppConfig &appConfig);
    void createAppSutWithDisabledTimer(const firebolt::rialto::common::AppConfig &appConfig);

    void willFailToInitialiseSockets() const;
    void launchingAppWillTimeout();
    void willFailToLaunchApp() const;
    void willLaunchApp() const;
    void willStartTimer();
    void willKillAppOnDestruction() const;
    void willCancelStartupTimer() const;
    void willConfigurePreloadedServer();
    void willGetSessionManagementSocketFd() const;
    void timerWillBeInactive() const;
    void timerWillBeActive() const;

    bool triggerConfigure(const firebolt::rialto::common::AppConfig &appConfig) const;
    void triggerGetSessionManagementSocketFd() const;
    void triggerReleaseNamedSocket() const;

private:
    std::shared_ptr<testing::StrictMock<firebolt::rialto::wrappers::LinuxWrapperMock>> m_linuxWrapper{
        std::make_shared<testing::StrictMock<firebolt::rialto::wrappers::LinuxWrapperMock>>()};
    testing::StrictMock<firebolt::rialto::wrappers::LinuxWrapperMock> &m_linuxWrapperMock{*m_linuxWrapper};
    std::shared_ptr<testing::StrictMock<firebolt::rialto::server::TimerFactoryMock>> m_timerFactoryMock{
        std::make_shared<testing::StrictMock<firebolt::rialto::server::TimerFactoryMock>>()};
    std::unique_ptr<testing::StrictMock<firebolt::rialto::server::TimerMock>> m_timer{
        std::make_unique<testing::StrictMock<firebolt::rialto::server::TimerMock>>()};
    testing::StrictMock<firebolt::rialto::server::TimerMock> &m_timerMock{*m_timer};
    testing::StrictMock<rialto::servermanager::common::SessionServerAppManagerMock> m_sessionServerAppManagerMock;
    std::unique_ptr<testing::StrictMock<firebolt::rialto::ipc::NamedSocketMock>> m_namedSocket{
        std::make_unique<testing::StrictMock<firebolt::rialto::ipc::NamedSocketMock>>()};
    testing::StrictMock<firebolt::rialto::ipc::NamedSocketMock> &m_namedSocketMock{*m_namedSocket};

protected:
    std::unique_ptr<rialto::servermanager::common::SessionServerApp> m_sut;
};

#endif // SESSION_SERVER_APP_TESTS_FIXTURE_H_
