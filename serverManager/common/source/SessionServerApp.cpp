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
#include "LinuxUtils.h"
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
#include <utility>

namespace
{
constexpr int kMaxPlaybackSessions{2};
constexpr int kMaxWebAudioPlayers{1};
const std::string kSessionManagementSocketDefaultDir{"/tmp/"};
const std::string kSessionManagementSocketDefaultName{"rialto-"};
const std::string kLogPathEnvVariable{"RIALTO_LOG_PATH"};

int generateServerId()
{
    static int id{0};
    return id++;
}

std::string generateSessionManagementSocketPath()
{
    static int sessionNum{0};
    return kSessionManagementSocketDefaultDir + kSessionManagementSocketDefaultName + std::to_string(sessionNum++);
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
    return kSessionManagementSocketDefaultDir + appConfig.clientIpcSocketName;
}
} // namespace

namespace rialto::servermanager::common
{
SessionServerApp::SessionServerApp(const std::shared_ptr<firebolt::rialto::wrappers::ILinuxWrapper> &linuxWrapper,
                                   const std::shared_ptr<firebolt::rialto::common::ITimerFactory> &timerFactory,
                                   ISessionServerAppManager &sessionServerAppManager,
                                   const std::list<std::string> &environmentVariables,
                                   const std::string &sessionServerPath,
                                   std::chrono::milliseconds sessionServerStartupTimeout, unsigned int socketPermissions,
                                   const std::string &socketOwner, const std::string &socketGroup,
                                   std::unique_ptr<firebolt::rialto::ipc::INamedSocket> &&namedSocket)
    : m_kServerId{generateServerId()}, m_initialState{firebolt::rialto::common::SessionServerState::UNINITIALIZED},
      m_socks{-1, -1}, m_linuxWrapper{linuxWrapper}, m_timerFactory{timerFactory},
      m_sessionServerAppManager{sessionServerAppManager}, m_pid{-1}, m_isPreloaded{true},
      m_kSessionServerPath{sessionServerPath}, m_kSessionServerStartupTimeout{sessionServerStartupTimeout},
      m_kSessionManagementSocketPermissions{socketPermissions}, m_kSessionManagementSocketOwner{socketOwner},
      m_kSessionManagementSocketGroup{socketGroup}, m_childInitialized{false},
      m_expectedState{firebolt::rialto::common::SessionServerState::UNINITIALIZED}, m_namedSocket{std::move(namedSocket)}
{
    RIALTO_SERVER_MANAGER_LOG_INFO("Creating preloaded SessionServerApp with serverId: %d", m_kServerId);
    std::transform(environmentVariables.begin(), environmentVariables.end(), std::back_inserter(m_environmentVariables),
                   [this](const std::string &str) { return strdup(addAppSuffixToLogFile(str).c_str()); });
    m_environmentVariables.push_back(nullptr);
}

SessionServerApp::SessionServerApp(const std::string &appName,
                                   const firebolt::rialto::common::SessionServerState &initialState,
                                   const firebolt::rialto::common::AppConfig &appConfig,
                                   const std::shared_ptr<firebolt::rialto::wrappers::ILinuxWrapper> &linuxWrapper,
                                   const std::shared_ptr<firebolt::rialto::common::ITimerFactory> &timerFactory,
                                   ISessionServerAppManager &sessionServerAppManager,
                                   const std::list<std::string> &environmentVariables,
                                   const std::string &sessionServerPath,
                                   std::chrono::milliseconds sessionServerStartupTimeout, unsigned int socketPermissions,
                                   const std::string &socketOwner, const std::string &socketGroup,
                                   std::unique_ptr<firebolt::rialto::ipc::INamedSocket> &&namedSocket)
    : m_kServerId{generateServerId()}, m_appName{appName}, m_initialState{initialState},
      m_sessionManagementSocketName{getSessionManagementSocketPath(appConfig)},
      m_clientDisplayName{appConfig.clientDisplayName}, m_subtitlesDisplayName{appConfig.subtitlesDisplayName},
      m_socks{-1, -1}, m_linuxWrapper{linuxWrapper}, m_timerFactory{timerFactory},
      m_sessionServerAppManager{sessionServerAppManager}, m_pid{-1}, m_isPreloaded{false},
      m_kSessionServerPath{sessionServerPath}, m_kSessionServerStartupTimeout{sessionServerStartupTimeout},
      m_kSessionManagementSocketPermissions{socketPermissions}, m_kSessionManagementSocketOwner{socketOwner},
      m_kSessionManagementSocketGroup{socketGroup}, m_childInitialized{false}, m_expectedState{initialState},
      m_namedSocket{std::move(namedSocket)}
{
    RIALTO_SERVER_MANAGER_LOG_INFO("Creating SessionServerApp for app: %s with appId: %d", appName.c_str(), m_kServerId);
    std::transform(environmentVariables.begin(), environmentVariables.end(), std::back_inserter(m_environmentVariables),
                   [this](const std::string &str) { return strdup(addAppSuffixToLogFile(str).c_str()); });
    m_environmentVariables.push_back(nullptr);
    if (m_namedSocket)
    {
        m_namedSocket->bind(m_sessionManagementSocketName);
        firebolt::rialto::common::setFileOwnership(m_sessionManagementSocketName, m_kSessionManagementSocketOwner,
                                                   m_kSessionManagementSocketGroup);
        firebolt::rialto::common::setFilePermissions(m_sessionManagementSocketName,
                                                     m_kSessionManagementSocketPermissions);
    }
}

SessionServerApp::~SessionServerApp()
{
    RIALTO_SERVER_MANAGER_LOG_INFO("Application %d is destructed", m_kServerId);
    cancelStartupTimerInternal();
    waitForChildProcess();
    if (m_socks[0] >= 0)
    {
        m_linuxWrapper->close(m_socks[0]);
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
    const int kChildSocket{m_socks[0]};
    const bool kResult = spawnSessionServer();
    std::unique_lock<std::mutex> lock{m_processStartupMutex};
    if (!m_processStartupCv.wait_for(lock, std::chrono::seconds{1}, [this]() { return m_childInitialized; }))
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Child initialization failed. Timeout on waiting for process startup");
        return false;
    }
    RIALTO_SERVER_MANAGER_LOG_DEBUG("Child initialized. Parent process will close the socket: %d now.", kChildSocket);
    if (0 != m_linuxWrapper->close(kChildSocket))
    {
        RIALTO_SERVER_MANAGER_LOG_SYS_ERROR(errno, "Close of socket %d failed in parent process", kChildSocket);
    }
    m_socks[0] = -1;
    return kResult;
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
    m_subtitlesDisplayName = appConfig.subtitlesDisplayName;
    m_isPreloaded = false;
    m_expectedState = initialState;
    if (m_namedSocket)
    {
        m_namedSocket->bind(m_sessionManagementSocketName);
        firebolt::rialto::common::setFileOwnership(m_sessionManagementSocketName, m_kSessionManagementSocketOwner,
                                                   m_kSessionManagementSocketGroup);
        firebolt::rialto::common::setFilePermissions(m_sessionManagementSocketName,
                                                     m_kSessionManagementSocketPermissions);
    }
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

unsigned int SessionServerApp::getSessionManagementSocketPermissions() const
{
    return m_kSessionManagementSocketPermissions;
}

std::string SessionServerApp::getSessionManagementSocketOwner() const
{
    return m_kSessionManagementSocketOwner;
}

std::string SessionServerApp::getSessionManagementSocketGroup() const
{
    return m_kSessionManagementSocketGroup;
}

std::string SessionServerApp::getClientDisplayName() const
{
    return m_clientDisplayName;
}

std::string SessionServerApp::getSubtitlesDisplayName() const
{
    return m_subtitlesDisplayName;
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
    return kMaxPlaybackSessions; // temporarily hardcoded
}

int SessionServerApp::getMaxWebAudioPlayers() const
{
    return kMaxWebAudioPlayers; // temporarily hardcoded
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
    if (envVar.find(kLogPathEnvVariable) != std::string::npos)
    {
        return envVar + "." + std::to_string(m_kServerId);
    }
    return envVar;
}

void SessionServerApp::kill()
{
    if (m_pid > 0)
    {
        m_linuxWrapper->kill(m_pid, SIGKILL);
    }
}

void SessionServerApp::setExpectedState(const firebolt::rialto::common::SessionServerState &state)
{
    m_expectedState = state;
}

firebolt::rialto::common::SessionServerState SessionServerApp::getExpectedState() const
{
    return m_expectedState;
}

bool SessionServerApp::initializeSockets()
{
    if (m_linuxWrapper->socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC | SOCK_NONBLOCK, 0, m_socks.data()) < 0)
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
        m_startupTimer = m_timerFactory->createTimer(m_kSessionServerStartupTimeout, [this]()
                                                     { m_sessionServerAppManager.onServerStartupTimeout(m_kServerId); });
    }
    else
    {
        RIALTO_SERVER_MANAGER_LOG_INFO("Startup timer disabled");
    }
}

bool SessionServerApp::spawnSessionServer()
{
    return m_linuxWrapper->vfork(
        [this](pid_t childPid)
        {
            if (childPid == -1)
            {
                RIALTO_SERVER_MANAGER_LOG_SYS_ERROR(errno, "Unable to spawn RialtoSessionServer - fork problem");
                m_linuxWrapper->close(m_socks[1]);
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
                    newSocket = m_linuxWrapper->dup(m_socks[0]);
                    if (0 != m_linuxWrapper->close(m_socks[0]))
                    {
                        RIALTO_SERVER_MANAGER_LOG_SYS_WARN(errno, "Socket %d could not be closed in child process.",
                                                           m_socks[0]);
                    }
                    RIALTO_SERVER_MANAGER_LOG_DEBUG("Child socket initialized: %d", newSocket);
                    m_childInitialized = true;
                    m_processStartupCv.notify_one();
                }
                if (!firebolt::rialto::logging::isConsoleLoggingEnabled())
                {
                    int devNull = m_linuxWrapper->open("/dev/null", O_RDWR, 0);
                    if (devNull < 0)
                    {
                        m_linuxWrapper->exit(EXIT_FAILURE);
                        return false; // wrapper function is not [[noreturn]]
                    }
                    m_linuxWrapper->dup2(devNull, STDIN_FILENO);
                    m_linuxWrapper->dup2(devNull, STDOUT_FILENO);
                    m_linuxWrapper->dup2(devNull, STDERR_FILENO);
                    if (devNull > STDERR_FILENO)
                    {
                        m_linuxWrapper->close(devNull);
                        devNull = -1;
                    }
                }
                const std::string kAppMgmtSocketStr{std::to_string(newSocket)};
                char *const appArguments[] = {strdup(m_kSessionServerPath.c_str()), strdup(kAppMgmtSocketStr.c_str()),
                                              nullptr};
                RIALTO_SERVER_MANAGER_LOG_DEBUG("PID: %d, executing: \"%s\" \"%s\"", m_linuxWrapper->getpid(),
                                                appArguments[0], appArguments[1]);
                m_linuxWrapper->execve(m_kSessionServerPath.c_str(), appArguments, m_environmentVariables.data());
                RIALTO_SERVER_MANAGER_LOG_SYS_ERROR(errno, "Unable to spawn RialtoSessionServer - execve problem");
                for (char *arg : appArguments)
                {
                    free(arg);
                }
                m_linuxWrapper->exit(EXIT_FAILURE);
                return true; // wrapper function is not [[noreturn]]
            }
        });
}

void SessionServerApp::waitForChildProcess()
{
    if (m_pid == -1)
    {
        return;
    }
    auto killTimer =
        m_timerFactory->createTimer(std::chrono::milliseconds{1500},
                                    [this]()
                                    {
                                        RIALTO_SERVER_MANAGER_LOG_ERROR("Waitpid timeout. Killing: %d", m_kServerId);
                                        kill();
                                    });
    if (m_linuxWrapper->waitpid(m_pid, nullptr, 0) < 0)
    {
        RIALTO_SERVER_MANAGER_LOG_SYS_WARN(errno, "waitpid failed for %d", m_kServerId);
    }
    killTimer->cancel();
    RIALTO_SERVER_MANAGER_LOG_DEBUG("Server with id: %d exited.", m_kServerId);
}

bool SessionServerApp::isNamedSocketInitialized() const
{
    return m_namedSocket != nullptr;
}

int SessionServerApp::getSessionManagementSocketFd() const
{
    if (m_namedSocket)
    {
        return m_namedSocket->getFd();
    }
    return -1;
}

std::unique_ptr<firebolt::rialto::ipc::INamedSocket> &&SessionServerApp::releaseNamedSocket()
{
    if (m_namedSocket)
    {
        m_namedSocket->blockNewConnections();
    }
    return std::move(m_namedSocket);
}
} // namespace rialto::servermanager::common
