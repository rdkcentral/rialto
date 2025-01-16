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

namespace
{
const std::string kEmptyClientIpcSocketName{""};
const std::string kClientIpcSocketName{"socketname"};
const std::string kFullPathClientIpcSocketName{"/some/path/socketname-1"};
const std::string kClientDisplayName{"displayname"};
} // namespace

TEST_F(SessionServerAppTests, ShouldConfigurePreloadedSut)
{
    createPreloadedAppSut();
    willConfigurePreloadedServer();
    EXPECT_TRUE(triggerConfigure(firebolt::rialto::common::AppConfig{kEmptyClientIpcSocketName, kClientDisplayName}));
    EXPECT_EQ(kClientDisplayName, m_sut->getClientDisplayName());
}

TEST_F(SessionServerAppTests, ShouldFailConfigurePreloadedSutTwice)
{
    createPreloadedAppSut();
    willConfigurePreloadedServer();
    EXPECT_TRUE(triggerConfigure(firebolt::rialto::common::AppConfig{kEmptyClientIpcSocketName, kClientDisplayName}));
    EXPECT_FALSE(triggerConfigure(firebolt::rialto::common::AppConfig{kEmptyClientIpcSocketName, kClientDisplayName}));
}

TEST_F(SessionServerAppTests, ShouldFailToConfigureSutForApp)
{
    createAppSut(firebolt::rialto::common::AppConfig{kEmptyClientIpcSocketName, kClientDisplayName});
    EXPECT_FALSE(triggerConfigure(firebolt::rialto::common::AppConfig{kEmptyClientIpcSocketName, kClientDisplayName}));
}

TEST_F(SessionServerAppTests, ShouldCreateSutForAppWithEmptyClientSocketName)
{
    const std::string kExpectedPrefix{"/tmp/rialto-"};
    createAppSut(firebolt::rialto::common::AppConfig{kEmptyClientIpcSocketName, kClientDisplayName});
    EXPECT_EQ(kExpectedPrefix, m_sut->getSessionManagementSocketName().substr(0, kExpectedPrefix.size()));
    EXPECT_EQ(kClientDisplayName, m_sut->getClientDisplayName());
}

TEST_F(SessionServerAppTests, ShouldCreateSutForAppWithClientSocketName)
{
    const std::string kExpectedSocketName{"/tmp/" + kClientIpcSocketName};
    createAppSut(firebolt::rialto::common::AppConfig{kClientIpcSocketName, kClientDisplayName});
    EXPECT_EQ(kExpectedSocketName, m_sut->getSessionManagementSocketName());
    EXPECT_EQ(kClientDisplayName, m_sut->getClientDisplayName());
}

TEST_F(SessionServerAppTests, ShouldCreateSutForAppWithFullPathClientSocketName)
{
    createAppSut(firebolt::rialto::common::AppConfig{kFullPathClientIpcSocketName, kClientDisplayName});
    EXPECT_EQ(kFullPathClientIpcSocketName, m_sut->getSessionManagementSocketName());
    EXPECT_EQ(kClientDisplayName, m_sut->getClientDisplayName());
}

TEST_F(SessionServerAppTests, ShouldFailToLaunchAppWhenSocketInitialisationFails)
{
    createAppSut(firebolt::rialto::common::AppConfig{kEmptyClientIpcSocketName, kClientDisplayName});
    willFailToInitialiseSockets();
    EXPECT_FALSE(m_sut->launch());
}

TEST_F(SessionServerAppTests, ShouldFailToLaunchAppWhenLaunchTimeouts)
{
    createAppSut(firebolt::rialto::common::AppConfig{kEmptyClientIpcSocketName, kClientDisplayName});
    launchingAppWillTimeout();
    EXPECT_FALSE(m_sut->launch());
    willKillAppOnDestruction();
    m_sut.reset();
}

TEST_F(SessionServerAppTests, ShouldFailToLaunchAppWhenForkFails)
{
    createAppSut(firebolt::rialto::common::AppConfig{kEmptyClientIpcSocketName, kClientDisplayName});
    willStartTimer();
    willFailToLaunchApp();
    EXPECT_FALSE(m_sut->launch());
    willCancelStartupTimer();
    m_sut.reset();
}

TEST_F(SessionServerAppTests, ShouldLaunchApp)
{
    createAppSut(firebolt::rialto::common::AppConfig{kEmptyClientIpcSocketName, kClientDisplayName});
    willLaunchApp();
    willStartTimer();
    EXPECT_TRUE(m_sut->launch());
    willCancelStartupTimer();
    m_sut.reset();
}

TEST_F(SessionServerAppTests, ShouldCancelStartupTimer)
{
    createAppSut(firebolt::rialto::common::AppConfig{kEmptyClientIpcSocketName, kClientDisplayName});
    willLaunchApp();
    willStartTimer();
    EXPECT_TRUE(m_sut->launch());
    timerWillBeActive();
    EXPECT_FALSE(m_sut->isConnected());
    willCancelStartupTimer();
    m_sut->cancelStartupTimer();
    timerWillBeInactive();
    EXPECT_TRUE(m_sut->isConnected());
}

TEST_F(SessionServerAppTests, ShouldLaunchAppWithoutStartupTimer)
{
    createAppSutWithDisabledTimer(firebolt::rialto::common::AppConfig{kEmptyClientIpcSocketName, kClientDisplayName});
    willLaunchApp();
    EXPECT_TRUE(m_sut->launch());
    m_sut.reset();
}

TEST_F(SessionServerAppTests, ShouldStoreExpectedSessionServerState)
{
    createAppSutWithDisabledTimer(firebolt::rialto::common::AppConfig{kEmptyClientIpcSocketName, kClientDisplayName});
    m_sut->setExpectedState(firebolt::rialto::common::SessionServerState::ACTIVE);
    EXPECT_EQ(firebolt::rialto::common::SessionServerState::ACTIVE, m_sut->getExpectedState());
    m_sut.reset();
}
