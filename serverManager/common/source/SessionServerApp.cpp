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

#include "SessionServerApp.h"
#include "RialtoServerManagerLogging.h"
#include "SessionServerAppManager.h"
#include "Utils.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

namespace
{
constexpr char sessionManagementPrefix[]{"/tmp/rialto-"};
constexpr int maxPlaybackSessions{2};

std::string generateSessionManagementSocket()
{
    static int sessionNum{0};
    return sessionManagementPrefix + std::to_string(sessionNum++);
}

std::string getSessionServerPath()
{
    const char *customPath = getenv("RIALTO_SESSION_SERVER_PATH");
    if (customPath)
    {
        RIALTO_SERVER_MANAGER_LOG_INFO("Using custom SessionServer path: %s", customPath);
        return std::string(customPath);
    }
    return "/usr/bin/RialtoServer";
}

std::chrono::milliseconds getStartupTimeout()
{
    const char *customTimeout = getenv("RIALTO_SESSION_SERVER_STARTUP_TIMEOUT_MS");
    std::chrono::milliseconds timeout{0};
    if (customTimeout)
    {
        try
        {
            timeout = std::chrono::milliseconds(std::stoull(customTimeout));
            RIALTO_SERVER_MANAGER_LOG_INFO("Using custom SessionServer startup timeout: %sms", customTimeout);
        }
        catch (const std::exception &e)
        {
            RIALTO_SERVER_MANAGER_LOG_ERROR("Custom SessionServer startup timeout invalid, ignoring: %s", customTimeout);
        }
    }
    return timeout;
}
} // namespace

namespace rialto::servermanager::common
{
SessionServerApp::SessionServerApp(const std::string &appId, const service::SessionServerState &initialState,
                                   SessionServerAppManager &sessionServerAppManager,
                                   const std::list<std::string> &environmentVariables)
    : m_kAppId{appId}, m_kInitialState{initialState}, m_kSessionManagementSocketName{generateSessionManagementSocket()},
      m_socks{-1, -1}, m_sessionServerAppManager{sessionServerAppManager}, m_pid{-1}
{
    RIALTO_SERVER_MANAGER_LOG_INFO("Application %s is created", m_kAppId.c_str());
    std::transform(environmentVariables.begin(), environmentVariables.end(), std::back_inserter(m_environmentVariables),
                   [](const std::string &str) { return strdup(str.c_str()); });
    m_environmentVariables.push_back(nullptr);
}

SessionServerApp::~SessionServerApp()
{
    RIALTO_SERVER_MANAGER_LOG_INFO("Application %s is destructed", m_kAppId.c_str());
    cancelStartupTimerInternal();
    if (m_pid != -1)
    {
        if (waitpid(m_pid, nullptr, 0) < 0)
        {
            RIALTO_SERVER_MANAGER_LOG_SYS_WARN(errno, "waitpid failed for %s", m_kAppId.c_str());
        }
    }
    if (m_socks[0] >= 0)
    {
        close(m_socks[0]);
    }
    for (char *var : m_environmentVariables)
    {
        if (var)
        {
            free(var);
        }
    }
    m_environmentVariables.clear();
}

bool SessionServerApp::launch()
{
    RIALTO_SERVER_MANAGER_LOG_INFO("Launching: %s", m_kAppId.c_str());
    if (!initializeSockets())
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Failed to launch: %s - unable to initialize sockets", m_kAppId.c_str());
        return false;
    }
    setupStartupTimer();
    bool result = spawnSessionServer();
    close(m_socks[0]);
    m_socks[0] = -1;
    return result;
}

std::string SessionServerApp::getSessionManagementSocketName() const
{
    return m_kSessionManagementSocketName;
}

service::SessionServerState SessionServerApp::getInitialState() const
{
    return m_kInitialState;
}

int SessionServerApp::getAppManagementSocketName() const
{
    return m_socks[1];
}

int SessionServerApp::getMaxPlaybackSessions() const
{
    return maxPlaybackSessions; // temporarily hardcoded
}

void SessionServerApp::cancelStartupTimer()
{
    cancelStartupTimerInternal();
}

void SessionServerApp::cancelStartupTimerInternal()
{
    std::unique_lock<std::mutex> lock{m_timerMutex};
    if (m_startupTimer && m_startupTimer->isActive())
    {
        RIALTO_SERVER_MANAGER_LOG_INFO("Application: %s connected successfully", m_kAppId.c_str());
        m_startupTimer->cancel();
    }
}

void SessionServerApp::kill() const
{
    if (m_pid > 0)
    {
        ::kill(m_pid, SIGKILL);
    }
}

bool SessionServerApp::initializeSockets()
{
    if (socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC | SOCK_NONBLOCK, 0, m_socks.data()) < 0)
    {
        RIALTO_SERVER_MANAGER_LOG_SYS_ERROR(errno, "socketpair failed");
        return false;
    }
    return true;
}

void SessionServerApp::setupStartupTimer()
{
    std::chrono::milliseconds timeout = getStartupTimeout();
    if (std::chrono::milliseconds(0) < timeout)
    {
        std::unique_lock<std::mutex> lock{m_timerMutex};
        auto factory = firebolt::rialto::common::ITimerFactory::getFactory();
        m_startupTimer =
            factory->createTimer(timeout,
                                 [this]()
                                 {
                                     RIALTO_SERVER_MANAGER_LOG_WARN("Killing: %s", m_kAppId.c_str());
                                     m_sessionServerAppManager
                                         .onSessionServerStateChanged(m_kAppId, service::SessionServerState::ERROR);
                                     kill();
                                 });
    }
    else
    {
        RIALTO_SERVER_MANAGER_LOG_INFO("Startup timer disabled");
    }
}

bool SessionServerApp::spawnSessionServer()
{
    pid_t childPid = vfork();
    if (childPid == -1)
    {
        RIALTO_SERVER_MANAGER_LOG_SYS_ERROR(errno, "Unable to spawn RialtoSessionServer - fork problem");
        close(m_socks[1]);
        m_socks[1] = -1;
        return false;
    }
    else if (childPid > 0)
    {
        RIALTO_SERVER_MANAGER_LOG_DEBUG("%s launched. PID: %d", m_kAppId.c_str(), childPid);
        m_pid = childPid;
        return true;
    }
    else
    {
        int newSocket = dup(m_socks[0]);
        close(m_socks[0]);
        m_socks[0] = newSocket;
        int devNull = open("/dev/null", O_RDWR, 0);
        if (devNull < 0)
        {
            _exit(EXIT_FAILURE);
        }
        dup2(devNull, STDIN_FILENO);
        dup2(devNull, STDOUT_FILENO);
        dup2(devNull, STDERR_FILENO);
        if (devNull > STDERR_FILENO)
        {
            close(devNull);
            devNull = -1;
        }
        const std::string appName{getSessionServerPath()};
        const std::string appMgmtSocketStr{std::to_string(m_socks[0])};
        char *const appArguments[] = {strdup(appName.c_str()), strdup(appMgmtSocketStr.c_str()), nullptr};
        RIALTO_SERVER_MANAGER_LOG_DEBUG("PID: %d, executing: \"%s\" \"%s\"", getpid(), appArguments[0], appArguments[1]);
        execve(appName.c_str(), appArguments, m_environmentVariables.data());
        RIALTO_SERVER_MANAGER_LOG_SYS_ERROR(errno, "Unable to spawn RialtoSessionServer - execve problem");
        for (char *arg : appArguments)
        {
            free(arg);
        }
        _exit(EXIT_FAILURE);
    }
}
} // namespace rialto::servermanager::common
