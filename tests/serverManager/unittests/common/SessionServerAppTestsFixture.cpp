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

#include "SessionServerAppTestsFixture.h"
#include <list>
#include <string>

namespace
{
const std::list<std::string> kEnvironmentVariablesWithLogPath{"var1", "RIALTO_LOG_PATH=/tmp/log"};
const std::list<std::string> kEnvironmentVariables{"var1", "var2"};
const std::string kSessionServerPath{"/usr/bin/RialtoServer"};
constexpr std::chrono::milliseconds kSessionServerStartupTimeout{100};
constexpr std::chrono::milliseconds kKillTimeout{1000};
constexpr unsigned int kSocketPermissions{0777};
const std::string kAppName{"YouTube"};
constexpr auto kInitialState{firebolt::rialto::common::SessionServerState::ACTIVE};
constexpr int kMaxPlaybackSessions{2};
constexpr int kMaxWebAudioPlayers{1};
const std::array<int, 2> kSocketPair{1, 2};
const int kDuplicatedSocket{3};
constexpr pid_t kPid{123};
MATCHER_P(CharStrMatcher, expectedStr, "")
{
    std::string actualStr = arg;
    return expectedStr == actualStr;
}

} // namespace

using testing::_;
using testing::AtLeast;
using testing::ByMove;
using testing::DoAll;
using testing::InvokeArgument;
using testing::Return;
using testing::SetArrayArgument;
using testing::StrictMock;

void SessionServerAppTests::createPreloadedAppSut()
{
    m_sut = std::make_unique<rialto::servermanager::common::SessionServerApp>(std::move(m_linuxWrapper),
                                                                              m_timerFactoryMock,
                                                                              m_sessionServerAppManagerMock,
                                                                              kEnvironmentVariables, kSessionServerPath,
                                                                              kSessionServerStartupTimeout,
                                                                              kSocketPermissions);
    ASSERT_TRUE(m_sut);
    EXPECT_TRUE(m_sut->isPreloaded());
    EXPECT_EQ(kSocketPermissions, m_sut->getSessionManagementSocketPermissions());
    EXPECT_TRUE(m_sut->getClientDisplayName().empty());
    EXPECT_EQ(firebolt::rialto::common::SessionServerState::UNINITIALIZED, m_sut->getInitialState());
    EXPECT_TRUE(m_sut->getAppName().empty());
    EXPECT_EQ(-1, m_sut->getAppManagementSocketName());
    EXPECT_EQ(kMaxPlaybackSessions, m_sut->getMaxPlaybackSessions());
    EXPECT_EQ(kMaxWebAudioPlayers, m_sut->getMaxWebAudioPlayers());
}

void SessionServerAppTests::createAppSut(const firebolt::rialto::common::AppConfig &appConfig)
{
    m_sut = std::make_unique<rialto::servermanager::common::SessionServerApp>(kAppName, kInitialState, appConfig,
                                                                              std::move(m_linuxWrapper),
                                                                              m_timerFactoryMock,
                                                                              m_sessionServerAppManagerMock,
                                                                              kEnvironmentVariablesWithLogPath,
                                                                              kSessionServerPath,
                                                                              kSessionServerStartupTimeout,
                                                                              kSocketPermissions);
    ASSERT_TRUE(m_sut);
    EXPECT_FALSE(m_sut->isPreloaded());
    EXPECT_EQ(kSocketPermissions, m_sut->getSessionManagementSocketPermissions());
    EXPECT_EQ(kInitialState, m_sut->getInitialState());
    EXPECT_EQ(kAppName, m_sut->getAppName());
    EXPECT_EQ(-1, m_sut->getAppManagementSocketName());
    EXPECT_EQ(kMaxPlaybackSessions, m_sut->getMaxPlaybackSessions());
    EXPECT_EQ(kMaxWebAudioPlayers, m_sut->getMaxWebAudioPlayers());
}

void SessionServerAppTests::createAppSutWithDisabledTimer(const firebolt::rialto::common::AppConfig &appConfig)
{
    m_sut = std::make_unique<rialto::servermanager::common::SessionServerApp>(kAppName, kInitialState, appConfig,
                                                                              std::move(m_linuxWrapper),
                                                                              m_timerFactoryMock,
                                                                              m_sessionServerAppManagerMock,
                                                                              kEnvironmentVariablesWithLogPath,
                                                                              kSessionServerPath,
                                                                              std::chrono::milliseconds{0},
                                                                              kSocketPermissions);
    ASSERT_TRUE(m_sut);
    EXPECT_FALSE(m_sut->isPreloaded());
    EXPECT_EQ(kSocketPermissions, m_sut->getSessionManagementSocketPermissions());
    EXPECT_EQ(kInitialState, m_sut->getInitialState());
    EXPECT_EQ(kAppName, m_sut->getAppName());
    EXPECT_EQ(-1, m_sut->getAppManagementSocketName());
    EXPECT_EQ(kMaxPlaybackSessions, m_sut->getMaxPlaybackSessions());
    EXPECT_EQ(kMaxWebAudioPlayers, m_sut->getMaxWebAudioPlayers());
}

void SessionServerAppTests::willFailToInitialiseSockets() const
{
    EXPECT_CALL(m_linuxWrapperMock, socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC | SOCK_NONBLOCK, 0, _))
        .WillOnce(Return(-1));
}

void SessionServerAppTests::launchingAppWillTimeout()
{
    const int kAppId{m_sut->getServerId()};
    EXPECT_CALL(m_linuxWrapperMock, socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC | SOCK_NONBLOCK, 0, _))
        .WillOnce(DoAll(SetArrayArgument<3>(kSocketPair.begin(), kSocketPair.end()), Return(0)));
    EXPECT_CALL(m_sessionServerAppManagerMock,
                onSessionServerStateChanged(kAppId, firebolt::rialto::common::SessionServerState::ERROR));
    EXPECT_CALL(m_sessionServerAppManagerMock,
                onSessionServerStateChanged(kAppId, firebolt::rialto::common::SessionServerState::NOT_RUNNING));
    EXPECT_CALL(m_linuxWrapperMock, vfork()).WillOnce(Return(kPid));
    EXPECT_CALL(*m_timerFactoryMock,
                createTimer(kSessionServerStartupTimeout, _, firebolt::rialto::common::TimerType::ONE_SHOT))
        .WillOnce(DoAll(InvokeArgument<1>(), Return(ByMove(std::move(m_timer)))));
}

void SessionServerAppTests::willFailToLaunchApp() const
{
    EXPECT_CALL(m_linuxWrapperMock, socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC | SOCK_NONBLOCK, 0, _))
        .WillOnce(DoAll(SetArrayArgument<3>(kSocketPair.begin(), kSocketPair.end()), Return(0)));
    EXPECT_CALL(m_linuxWrapperMock, vfork()).WillOnce(Return(-1));
    EXPECT_CALL(m_linuxWrapperMock, close(kSocketPair[0]));
    EXPECT_CALL(m_linuxWrapperMock, close(kSocketPair[1]));
}

void SessionServerAppTests::willLaunchApp() const
{
    EXPECT_CALL(m_linuxWrapperMock, socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC | SOCK_NONBLOCK, 0, _))
        .WillOnce(DoAll(SetArrayArgument<3>(kSocketPair.begin(), kSocketPair.end()), Return(0)));
    EXPECT_CALL(m_linuxWrapperMock, vfork()).WillOnce(Return(0));
    EXPECT_CALL(m_linuxWrapperMock, close(kSocketPair[0])).Times(2).WillRepeatedly(Return(-1));
    EXPECT_CALL(m_linuxWrapperMock, getpid()).WillOnce(Return(kPid));
    EXPECT_CALL(m_linuxWrapperMock, dup(kSocketPair[0])).WillOnce(Return(kDuplicatedSocket));
    EXPECT_CALL(m_linuxWrapperMock, execve(CharStrMatcher(kSessionServerPath), _, _)).WillOnce(Return(-1));
    EXPECT_CALL(m_linuxWrapperMock, exit(EXIT_FAILURE)); // Not possible to stop on execve with mock :-)
}

void SessionServerAppTests::willStartTimer()
{
    EXPECT_CALL(*m_timerFactoryMock,
                createTimer(kSessionServerStartupTimeout, _, firebolt::rialto::common::TimerType::ONE_SHOT))
        .WillOnce(Return(ByMove(std::move(m_timer))));
}

void SessionServerAppTests::willKillAppOnDestruction() const
{
    auto killTimer{std::make_unique<StrictMock<firebolt::rialto::server::TimerMock>>()};
    EXPECT_CALL(m_timerMock, isActive()).WillOnce(Return(false));
    EXPECT_CALL(m_linuxWrapperMock, kill(kPid, SIGKILL)).WillOnce(Return(0));
    EXPECT_CALL(*killTimer, cancel());
    EXPECT_CALL(*m_timerFactoryMock, createTimer(kKillTimeout, _, firebolt::rialto::common::TimerType::ONE_SHOT))
        .WillOnce(DoAll(InvokeArgument<1>(), Return(ByMove(std::move(killTimer)))));
    EXPECT_CALL(m_linuxWrapperMock, waitpid(kPid, nullptr, 0)).WillOnce(Return(-1));
    EXPECT_CALL(m_linuxWrapperMock, close(kSocketPair[0])).WillOnce(Return(0));
}

void SessionServerAppTests::willCancelStartupTimer() const
{
    EXPECT_CALL(m_timerMock, isActive()).WillOnce(Return(true));
    EXPECT_CALL(m_timerMock, cancel());
}

void SessionServerAppTests::timerWillBeInactive() const
{
    EXPECT_CALL(m_timerMock, isActive()).WillRepeatedly(Return(false));
}

void SessionServerAppTests::timerWillBeActive() const
{
    EXPECT_CALL(m_timerMock, isActive()).WillRepeatedly(Return(true));
}

bool SessionServerAppTests::triggerConfigure(const firebolt::rialto::common::AppConfig &appConfig) const
{
    const bool ret = m_sut->configure(kAppName, kInitialState, appConfig);
    EXPECT_EQ(kInitialState, m_sut->getInitialState());
    return ret;
}
