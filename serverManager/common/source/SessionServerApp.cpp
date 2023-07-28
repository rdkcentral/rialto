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
const std::string logPathEnvVariable{"RIALTO_LOG_PATH"};

int generateServerId()
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
} // namespace

namespace rialto::servermanager::common
{
SessionServerApp::SessionServerApp(SessionServerAppManager &sessionServerAppManager,
                                   const std::list<std::string> &environmentVariables,
                                   const std::string &sessionServerPath,
                                   std::chrono::milliseconds sessionServerStartupTimeout)
    : m_kServerId{generateServerId()}, m_socks{-1, -1}, m_sessionServerAppManager{sessionServerAppManager}, m_pid{-1},
      m_isPreloaded{true}, m_kSessionServerPath{sessionServerPath},
      m_kSessionServerStartupTimeout{sessionServerStartupTimeout}, m_childInitialized{false}
{
    RIALTO_SERVER_MANAGER_LOG_INFO("Creating preloaded SessionServerApp with serverId: %d", m_kServerId);
    std::transform(environmentVariables.begin(), environmentVariables.end(), std::back_inserter(m_environmentVariables),
                   [this](const std::string &str) { return strdup(addAppSuffixToLogFile(str).c_str()); });
    m_environmentVariables.push_back(nullptr);
}

SessionServerApp::SessionServerApp(const std::string &appName,
                                   const firebolt::rialto::common::SessionServerState &initialState,
                                   const firebolt::rialto::common::AppConfig &appConfig,
                                   SessionServerAppManager &sessionServerAppManager,
                                   const std::list<std::string> &environmentVariables,
                                   const std::string &sessionServerPath,
                                   std::chrono::milliseconds sessionServerStartupTimeout)
    : m_kServerId{generateServerId()}, m_appName{appName}, m_initialState{initialState},
      m_sessionManagementSocketName{getSessionManagementSocketPath(appConfig)},
      m_clientDisplayName{appConfig.clientDisplayName}, m_socks{-1, -1},
      m_sessionServerAppManager{sessionServerAppManager}, m_pid{-1}, m_isPreloaded{false},
      m_kSessionServerPath{sessionServerPath}, m_kSessionServerStartupTimeout{sessionServerStartupTimeout},
      m_childInitialized{false}
{
    RIALTO_SERVER_MANAGER_LOG_INFO("Creating SessionServerApp for app: %s with appId: %d", appName.c_str(), m_kServerId);
    std::transform(environmentVariables.begin(), environmentVariables.end(), std::back_inserter(m_environmentVariables),
                   [this](const std::string &str) { return strdup(addAppSuffixToLogFile(str).c_str()); });
    m_environmentVariables.push_back(nullptr);
}

SessionServerApp::~SessionServerApp()
{
    RIALTO_SERVER_MANAGER_LOG_INFO("Application %d is destructed", m_kServerId);
    cancelStartupTimerInternal();
    waitForChildProcess();
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
    RIALTO_SERVER_MANAGER_LOG_INFO("Launching: %d", m_kServerId);
    if (!initializeSockets())
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Failed to launch: %d - unable to initialize sockets", m_kServerId);
        return false;
    }
    setupStartupTimer();
    const int childSocket{m_socks[0]};
    const bool result = spawnSessionServer();
    std::unique_lock<std::mutex> lock{m_processStartupMutex};
    if (!m_processStartupCv.wait_for(lock, std::chrono::seconds{1}, [this]() { return m_childInitialized; }))
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Child initialization failed. Timeout on waiting for process startup");
        return false;
    }
    RIALTO_SERVER_MANAGER_LOG_DEBUG("Child initialized. Parent process will close the socket: %d now.", childSocket);
    if (0 != close(childSocket))
    {
        RIALTO_SERVER_MANAGER_LOG_SYS_ERROR(errno, "Close of socket %d failed in parent process", childSocket);
    }
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
    m_clientDisplayName = appConfig.clientDisplayName;
    m_isPreloaded = false;
    return true;
}

bool SessionServerApp::isConnected() const
{
    std::unique_lock<std::mutex> lock{m_timerMutex};
    return !m_startupTimer || !m_startupTimer->isActive();
}

std::string SessionServerApp::getSessionManagementSocketName() const
{
    return m_sessionManagementSocketName;
}

std::string SessionServerApp::getClientDisplayName() const
{
    return m_clientDisplayName;
}

firebolt::rialto::common::SessionServerState SessionServerApp::getInitialState() const
{
    return m_initialState;
}

int SessionServerApp::getServerId() const
{
    return m_kServerId;
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
        RIALTO_SERVER_MANAGER_LOG_INFO("Application: %d connected successfully", m_kServerId);
        m_startupTimer->cancel();
    }
}

std::string SessionServerApp::addAppSuffixToLogFile(const std::string &envVar) const
{
    if (envVar.find(logPathEnvVariable) != std::string::npos)
    {
        return envVar + "." + std::to_string(m_kServerId);
    }
    return envVar;
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
    if (std::chrono::milliseconds(0) < m_kSessionServerStartupTimeout)
    {
        std::unique_lock<std::mutex> lock{m_timerMutex};
        auto factory = firebolt::rialto::common::ITimerFactory::getFactory();
        m_startupTimer =
            factory->createTimer(m_kSessionServerStartupTimeout,
                                 [this]()
                                 {
                                     RIALTO_SERVER_MANAGER_LOG_WARN("Killing: %d", m_kServerId);
                                     m_sessionServerAppManager
                                         .onSessionServerStateChanged(m_kServerId,
                                                                      firebolt::rialto::common::SessionServerState::ERROR);
                                     kill();
                                     m_sessionServerAppManager
                                         .onSessionServerStateChanged(m_kServerId,
                                                                      firebolt::rialto::common::SessionServerState::NOT_RUNNING);
                                 });
    }
    else
    {
        RIALTO_SERVER_MANAGER_LOG_INFO("Startup timer disabled");
    }
}

std::vector<char *> SessionServerApp::createArguments(int newSocket)
{
    const std::string appMgmtSocketStr{std::to_string(newSocket)};

    std::vector<char *> appArguments;

    const char *valgrindParams = getenv("VALGRIND_PARAMS");
    if (valgrindParams)
    {
        std::string envVarsStr{valgrindParams};
        size_t pos = 0;
        while ((pos = envVarsStr.find(";")) != std::string::npos)
        {
            appArguments.push_back(strdup(envVarsStr.substr(0, pos).c_str()));
            envVarsStr.erase(0, pos + 1);
        }
        appArguments.push_back(strdup(envVarsStr.c_str()));
    }

    appArguments.insert(appArguments.end(),
                        {strdup(m_kSessionServerPath.c_str()), strdup(appMgmtSocketStr.c_str()), nullptr});



    return appArguments;
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
        RIALTO_SERVER_MANAGER_LOG_DEBUG("%d launched. PID: %d", m_kServerId, childPid);
        m_pid = childPid;
        return true;
    }
    else
    {
        int newSocket{-1};
        {
            std::unique_lock<std::mutex> lock{m_processStartupMutex};
            newSocket = dup(m_socks[0]);
            if (0 != close(m_socks[0]))
            {
                RIALTO_SERVER_MANAGER_LOG_SYS_WARN(errno, "Socket %d could not be closed in child process.", m_socks[0]);
            }
            RIALTO_SERVER_MANAGER_LOG_DEBUG("Child socket initialized: %d", newSocket);
            m_childInitialized = true;
            m_processStartupCv.notify_one();
        }
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

        std::vector<char *> appArguments = createArguments(newSocket);

        std::string launchParameters;
        for (char *param : appArguments)
        {
            if (param)
                launchParameters += std::string(param) + " ";
        }
        launchParameters.pop_back();

        RIALTO_SERVER_MANAGER_LOG_MIL("PID: %d, executing: '%s'", getpid(), launchParameters.c_str());

        execve(appArguments[0], appArguments.data(), m_environmentVariables.data());
        RIALTO_SERVER_MANAGER_LOG_SYS_ERROR(errno, "Unable to spawn RialtoSessionServer - execve problem");
        for (char *arg : appArguments)
        {
            free(arg);
        }
        _exit(EXIT_FAILURE);
    }
}

void SessionServerApp::waitForChildProcess()
{
    if (m_pid == -1)
    {
        return;
    }
    auto killTimer = firebolt::rialto::common::ITimerFactory::getFactory()
                         ->createTimer(std::chrono::milliseconds{1000},
                                       [this]()
                                       {
                                           RIALTO_SERVER_MANAGER_LOG_ERROR("Waitpid timeout. Killing: %d", m_kServerId);
                                           kill();
                                       });
    if (waitpid(m_pid, nullptr, 0) < 0)
    {
        RIALTO_SERVER_MANAGER_LOG_SYS_WARN(errno, "waitpid failed for %d", m_kServerId);
    }
    killTimer->cancel();
    RIALTO_SERVER_MANAGER_LOG_DEBUG("Server with id: %d exited.", m_kServerId);
}
} // namespace rialto::servermanager::common
