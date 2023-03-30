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
#include "RialtoLogging.h"
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
constexpr int maxPlaybackSessions{2};
constexpr int maxWebAudioPlayers{1};
const std::string sessionManagementSocketDefaultDir{"/tmp/"};
const std::string sessionManagementSocketDefaultName{"rialto-"};

int generateAppId()
{
    static int id{0};
    return id++;
}

std::string generateSessionManagementSocketPath()
{
    static int sessionNum{0};
    return sessionManagementSocketDefaultDir + sessionManagementSocketDefaultName + std::to_string(sessionNum++);
}

std::string getSessionManagementSocketPath(const firebolt::rialto::common::AppConfig &appConfig)
{
    // Socket name can take the following forms:
    //  - Empty string, in which case Rialto server will automatically allocate the socket name, e.g. "/tmp/rialto-12"
    //  - Full path, such as "/foo/bar", in which case Rialto will use this name for the socket
    //  - Socket name, such as "bar", in which case Rialto will create the named socket in the default dir, e.g.
    if (appConfig.clientIpcSocketName.empty())
    {
        return generateSessionManagementSocketPath();
    }
    else if (appConfig.clientIpcSocketName.at(0) == '/') // full path
    {
        return appConfig.clientIpcSocketName;
    }
    // Socket name
    return sessionManagementSocketDefaultDir + appConfig.clientIpcSocketName;
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
SessionServerApp::SessionServerApp(SessionServerAppManager &sessionServerAppManager,
                                   const std::list<std::string> &environmentVariables)
    : m_kAppId{generateAppId()}, m_socks{-1, -1}, m_sessionServerAppManager{sessionServerAppManager}, m_pid{-1},
      m_isPreloaded{true}
{
    RIALTO_SERVER_MANAGER_LOG_INFO("Creating preloaded SessionServerApp with appId: %d", m_kAppId);
    std::transform(environmentVariables.begin(), environmentVariables.end(), std::back_inserter(m_environmentVariables),
                   [](const std::string &str) { return strdup(str.c_str()); });
    m_environmentVariables.push_back(nullptr);
}

SessionServerApp::SessionServerApp(const std::string &appName,
                                   const firebolt::rialto::common::SessionServerState &initialState,
                                   const firebolt::rialto::common::AppConfig &appConfig,
                                   SessionServerAppManager &sessionServerAppManager,
                                   const std::list<std::string> &environmentVariables)
    : m_kAppId{generateAppId()}, m_appName{appName}, m_initialState{initialState},
      m_sessionManagementSocketName{getSessionManagementSocketPath(appConfig)}, m_socks{-1, -1},
      m_sessionServerAppManager{sessionServerAppManager}, m_pid{-1}, m_isPreloaded{false}
{
    RIALTO_SERVER_MANAGER_LOG_INFO("Creating SessionServerApp for app: %s with appId: %d", appName.c_str(), m_kAppId);
    std::transform(environmentVariables.begin(), environmentVariables.end(), std::back_inserter(m_environmentVariables),
                   [](const std::string &str) { return strdup(str.c_str()); });
    m_environmentVariables.push_back(nullptr);
}

SessionServerApp::~SessionServerApp()
{
    RIALTO_SERVER_MANAGER_LOG_INFO("Application %d is destructed", m_kAppId);
    cancelStartupTimerInternal();
    if (m_pid != -1)
    {
        if (waitpid(m_pid, nullptr, 0) < 0)
        {
            RIALTO_SERVER_MANAGER_LOG_SYS_WARN(errno, "waitpid failed for %d", m_kAppId);
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
    RIALTO_SERVER_MANAGER_LOG_INFO("Launching: %d", m_kAppId);
    if (!initializeSockets())
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Failed to launch: %d - unable to initialize sockets", m_kAppId);
        return false;
    }
    setupStartupTimer();
    bool result = spawnSessionServer();
    close(m_socks[0]);
    m_socks[0] = -1;
    return result;
}

bool SessionServerApp::isPreloaded() const
{
    return m_isPreloaded;
}

bool SessionServerApp::configure(const std::string &appName,
                                 const firebolt::rialto::common::SessionServerState &initialState,
                                 const firebolt::rialto::common::AppConfig &appConfig)
{
    if (!m_isPreloaded)
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("SessionServerApp is already configured!");
        return false;
    }
    m_appName = appName;
    m_initialState = initialState;
    m_sessionManagementSocketName = getSessionManagementSocketPath(appConfig);
    m_isPreloaded = false;
    return true;
}

bool SessionServerApp::isConnected() const
{
    std::unique_lock<std::mutex> lock{m_timerMutex};
    return m_startupTimer && !m_startupTimer->isActive();
}

std::string SessionServerApp::getSessionManagementSocketName() const
{
    return m_sessionManagementSocketName;
}

firebolt::rialto::common::SessionServerState SessionServerApp::getInitialState() const
{
    return m_initialState;
}

int SessionServerApp::getAppId() const
{
    return m_kAppId;
}

const std::string &SessionServerApp::getAppName() const
{
    return m_appName;
}

int SessionServerApp::getAppManagementSocketName() const
{
    return m_socks[1];
}

int SessionServerApp::getMaxPlaybackSessions() const
{
    return maxPlaybackSessions; // temporarily hardcoded
}

int SessionServerApp::getMaxWebAudioPlayers() const
{
    return maxWebAudioPlayers; // temporarily hardcoded
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
        RIALTO_SERVER_MANAGER_LOG_INFO("Application: %d connected successfully", m_kAppId);
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
                                     RIALTO_SERVER_MANAGER_LOG_WARN("Killing: %d", m_kAppId);
                                     m_sessionServerAppManager
                                         .onSessionServerStateChanged(m_kAppId,
                                                                      firebolt::rialto::common::SessionServerState::ERROR);
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
        RIALTO_SERVER_MANAGER_LOG_DEBUG("%d launched. PID: %d", m_kAppId, childPid);
        m_pid = childPid;
        return true;
    }
    else
    {
        int newSocket = dup(m_socks[0]);
        close(m_socks[0]);
        m_socks[0] = newSocket;
        if (!firebolt::rialto::logging::isConsoleLoggingEnabled())
        {
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
