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

#include "ServerManagerModuleServiceTestsFixture.h"
#include <cstdlib>
#include <optional>
#include <string>

namespace
{
constexpr char kPlaybackInfoTimerEnvVar[] = "RIALTO_PLAYBACK_INFO_TIMER_MS";

class ScopedPlaybackInfoTimerEnv
{
public:
    ScopedPlaybackInfoTimerEnv()
    {
        if (const char *value = std::getenv(kPlaybackInfoTimerEnvVar))
        {
            m_hadPreviousValue = true;
            m_previousValue = value;
        }
        unsetenv(kPlaybackInfoTimerEnvVar);
    }

    ~ScopedPlaybackInfoTimerEnv()
    {
        if (m_hadPreviousValue)
        {
            setenv(kPlaybackInfoTimerEnvVar, m_previousValue.c_str(), 1);
        }
        else
        {
            unsetenv(kPlaybackInfoTimerEnvVar);
        }
    }

    void set(const char *value) { setenv(kPlaybackInfoTimerEnvVar, value, 1); }

private:
    bool m_hadPreviousValue{false};
    std::string m_previousValue;
};
} // namespace

TEST_F(ServerManagerModuleServiceTests, shouldSetConfiguration)
{
    sessionServerManagerWillHandleRequestSuccess();
    sessionServerManagerWillSetConfiguration(firebolt::rialto::common::SessionServerState::ACTIVE);
    sessionServerManagerWillSetLogLevels();
    sendSetConfiguration(firebolt::rialto::common::SessionServerState::ACTIVE);
}

TEST_F(ServerManagerModuleServiceTests, shouldSetConfigurationWithFd)
{
    sessionServerManagerWillHandleRequestSuccess();
    sessionServerManagerWillSetConfigurationWithFd(firebolt::rialto::common::SessionServerState::ACTIVE);
    sessionServerManagerWillSetLogLevels();
    sendSetConfigurationWithFd(firebolt::rialto::common::SessionServerState::ACTIVE);
}

TEST_F(ServerManagerModuleServiceTests, shouldSetPlaybackInfoTimerEnvironmentVariable)
{
    ScopedPlaybackInfoTimerEnv env;
    sessionServerManagerWillHandleRequestSuccess();
    sessionServerManagerWillSetConfiguration(firebolt::rialto::common::SessionServerState::ACTIVE);
    sessionServerManagerWillSetLogLevels();

    sendSetConfiguration(firebolt::rialto::common::SessionServerState::ACTIVE, std::optional<uint32_t>{250});

    ASSERT_NE(nullptr, std::getenv(kPlaybackInfoTimerEnvVar));
    EXPECT_STREQ("250", std::getenv(kPlaybackInfoTimerEnvVar));
}

TEST_F(ServerManagerModuleServiceTests, shouldClearPlaybackInfoTimerEnvironmentVariableWhenConfigurationIsMissing)
{
    ScopedPlaybackInfoTimerEnv env;
    env.set("250");
    sessionServerManagerWillHandleRequestSuccess();
    sessionServerManagerWillSetConfiguration(firebolt::rialto::common::SessionServerState::ACTIVE);
    sessionServerManagerWillSetLogLevels();

    sendSetConfiguration(firebolt::rialto::common::SessionServerState::ACTIVE);

    EXPECT_EQ(nullptr, std::getenv(kPlaybackInfoTimerEnvVar));
}

TEST_F(ServerManagerModuleServiceTests, shouldClearPlaybackInfoTimerEnvironmentVariableWhenConfigurationIsZero)
{
    ScopedPlaybackInfoTimerEnv env;
    env.set("250");
    sessionServerManagerWillHandleRequestSuccess();
    sessionServerManagerWillSetConfiguration(firebolt::rialto::common::SessionServerState::ACTIVE);
    sessionServerManagerWillSetLogLevels();

    sendSetConfiguration(firebolt::rialto::common::SessionServerState::ACTIVE, std::optional<uint32_t>{0});

    EXPECT_EQ(nullptr, std::getenv(kPlaybackInfoTimerEnvVar));
}

TEST_F(ServerManagerModuleServiceTests, shouldSetStateToUninitialized)
{
    sessionServerManagerWillHandleRequestSuccess();
    sessionServerManagerWillSetState(firebolt::rialto::common::SessionServerState::UNINITIALIZED);
    sendSetState(firebolt::rialto::common::SessionServerState::UNINITIALIZED);
}

TEST_F(ServerManagerModuleServiceTests, shouldSetStateToInactive)
{
    sessionServerManagerWillHandleRequestSuccess();
    sessionServerManagerWillSetState(firebolt::rialto::common::SessionServerState::INACTIVE);
    sendSetState(firebolt::rialto::common::SessionServerState::INACTIVE);
}

TEST_F(ServerManagerModuleServiceTests, shouldSetStateToActive)
{
    sessionServerManagerWillHandleRequestSuccess();
    sessionServerManagerWillSetState(firebolt::rialto::common::SessionServerState::ACTIVE);
    sendSetState(firebolt::rialto::common::SessionServerState::ACTIVE);
}

TEST_F(ServerManagerModuleServiceTests, shouldSetStateToNotRunning)
{
    sessionServerManagerWillHandleRequestSuccess();
    sessionServerManagerWillSetState(firebolt::rialto::common::SessionServerState::NOT_RUNNING);
    sendSetState(firebolt::rialto::common::SessionServerState::NOT_RUNNING);
}

TEST_F(ServerManagerModuleServiceTests, shouldSetStateToError)
{
    sessionServerManagerWillHandleRequestSuccess();
    sessionServerManagerWillSetState(firebolt::rialto::common::SessionServerState::ERROR);
    sendSetState(firebolt::rialto::common::SessionServerState::ERROR);
}

TEST_F(ServerManagerModuleServiceTests, shouldSetLogLevels)
{
    sessionServerManagerWillHandleRequestSuccess();
    sessionServerManagerWillSetLogLevels();
    sendSetLogLevels();
}

TEST_F(ServerManagerModuleServiceTests, shouldFailToSetConfiguration)
{
    sessionServerManagerWillHandleRequestFailure();
    sessionServerManagerWillFailToSetConfiguration(firebolt::rialto::common::SessionServerState::ACTIVE);
    sessionServerManagerWillSetLogLevels();
    sendSetConfiguration(firebolt::rialto::common::SessionServerState::ACTIVE);
}

TEST_F(ServerManagerModuleServiceTests, shouldFailToSetConfigurationWithFd)
{
    sessionServerManagerWillHandleRequestFailure();
    sessionServerManagerWillFailToSetConfigurationWithFd(firebolt::rialto::common::SessionServerState::ACTIVE);
    sessionServerManagerWillSetLogLevels();
    sendSetConfigurationWithFd(firebolt::rialto::common::SessionServerState::ACTIVE);
}

TEST_F(ServerManagerModuleServiceTests, shouldFailToSetState)
{
    sessionServerManagerWillHandleRequestFailure();
    sessionServerManagerWillFailToSetState(firebolt::rialto::common::SessionServerState::ACTIVE);
    sendSetState(firebolt::rialto::common::SessionServerState::ACTIVE);
}

TEST_F(ServerManagerModuleServiceTests, shouldPing)
{
    sessionServerManagerWillHandleRequestSuccess();
    sessionServerManagerWillPing();
    sendPing();
}

TEST_F(ServerManagerModuleServiceTests, shouldFailToPing)
{
    sessionServerManagerWillHandleRequestFailure();
    sessionServerManagerWillFailToPing();
    sendPing();
}

TEST_F(ServerManagerModuleServiceTests, shouldFailToPingDueToInvalidController)
{
    sessionServerManagerWillHandleRequestFailureWithInvalidController();
    sendPingWithInvalidController();
}
