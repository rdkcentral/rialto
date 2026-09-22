/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
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

#include "IStateObserver.h"
#include "ServerManagerServiceFactory.h"
#include "SessionServerCommon.h"
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <gtest/gtest.h>
#include <limits.h>
#include <mutex>
#include <poll.h>
#include <spawn.h>
#include <sstream>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

extern char **environ;

namespace
{
constexpr char kAppId[]{"per-instance-shm-process-test"};
constexpr char kSocketPath[]{"/tmp/rialto-per-instance-shm-process-test"};
constexpr auto kTimeout{std::chrono::seconds{5}};

class StateObserver : public rialto::servermanager::service::IStateObserver
{
public:
    void stateChanged(const std::string &appId, const firebolt::rialto::common::SessionServerState &state) override
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        if (appId == kAppId)
        {
            m_state = state;
            if (state == firebolt::rialto::common::SessionServerState::ACTIVE)
            {
                ++m_activeGeneration;
            }
            m_condition.notify_all();
        }
    }

    bool waitFor(firebolt::rialto::common::SessionServerState state)
    {
        std::unique_lock<std::mutex> lock{m_mutex};
        return m_condition.wait_for(lock, kTimeout, [&]() { return m_state == state; });
    }

    bool waitForActiveGeneration(unsigned generation)
    {
        std::unique_lock<std::mutex> lock{m_mutex};
        return m_condition.wait_for(lock, kTimeout, [&]() { return m_activeGeneration >= generation; });
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_condition;
    firebolt::rialto::common::SessionServerState m_state{firebolt::rialto::common::SessionServerState::NOT_RUNNING};
    unsigned m_activeGeneration{0U};
};

struct ChildProcess
{
    pid_t pid{-1};
    int input{-1};
    int output{-1};

    ChildProcess() = default;
    ChildProcess(pid_t childPid, int childInput, int childOutput)
        : pid{childPid}, input{childInput}, output{childOutput}
    {
    }
    ChildProcess(const ChildProcess &) = delete;
    ChildProcess &operator=(const ChildProcess &) = delete;
    ChildProcess(ChildProcess &&other) noexcept : pid{other.pid}, input{other.input}, output{other.output}
    {
        other.pid = -1;
        other.input = -1;
        other.output = -1;
    }
    ~ChildProcess()
    {
        if (input >= 0)
        {
            close(input);
        }
        if (output >= 0)
        {
            close(output);
        }
        if (pid > 0)
        {
            kill(pid, SIGKILL);
            waitpid(pid, nullptr, 0);
        }
    }
};

ChildProcess launchClient()
{
    int parentToChild[2]{};
    int childToParent[2]{};
    if (pipe2(parentToChild, O_CLOEXEC) != 0 || pipe2(childToParent, O_CLOEXEC) != 0)
    {
        return {};
    }

    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_adddup2(&actions, parentToChild[0], STDIN_FILENO);
    posix_spawn_file_actions_adddup2(&actions, childToParent[1], STDOUT_FILENO);
    posix_spawn_file_actions_addclose(&actions, parentToChild[1]);
    posix_spawn_file_actions_addclose(&actions, childToParent[0]);

    char *arguments[]{const_cast<char *>(RIALTO_PROCESS_CLIENT_PATH), const_cast<char *>(kSocketPath), nullptr};
    pid_t pid{-1};
    int result = posix_spawn(&pid, RIALTO_PROCESS_CLIENT_PATH, &actions, nullptr, arguments, environ);
    posix_spawn_file_actions_destroy(&actions);
    close(parentToChild[0]);
    close(childToParent[1]);
    if (result != 0)
    {
        close(parentToChild[1]);
        close(childToParent[0]);
        return {};
    }
    return ChildProcess{pid, parentToChild[1], childToParent[0]};
}

bool sendCommand(const ChildProcess &child, const std::string &command)
{
    std::string line{command + "\n"};
    return write(child.input, line.data(), line.size()) == static_cast<ssize_t>(line.size());
}

std::string readLine(const ChildProcess &child)
{
    std::string line;
    auto deadline = std::chrono::steady_clock::now() + kTimeout;
    while (std::chrono::steady_clock::now() < deadline)
    {
        pollfd descriptor{child.output, POLLIN, 0};
        int remaining = static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count());
        if (poll(&descriptor, 1, remaining) <= 0)
        {
            break;
        }
        char value{};
        if (read(child.output, &value, 1) != 1)
        {
            break;
        }
        if (value == '\n')
        {
            return line;
        }
        line += value;
    }
    return line;
}

std::vector<std::uint64_t> parseInodes(const std::string &line, const std::string &expectedState)
{
    std::istringstream stream{line};
    std::string state;
    std::size_t count{};
    stream >> state >> count;
    if (state != expectedState)
    {
        return {};
    }
    std::vector<std::uint64_t> inodes;
    for (std::size_t i = 0; i < count; ++i)
    {
        std::uint64_t inode{};
        stream >> inode;
        inodes.emplace_back(inode);
    }
    return inodes;
}

std::size_t countProcessMemfds(pid_t pid)
{
    std::string directoryPath{"/proc/" + std::to_string(pid) + "/fd"};
    DIR *directory = opendir(directoryPath.c_str());
    if (!directory)
    {
        return 0U;
    }
    std::size_t count{};
    while (auto *entry = readdir(directory))
    {
        if (entry->d_name[0] == '.')
        {
            continue;
        }
        std::string path{directoryPath + "/" + entry->d_name};
        char target[PATH_MAX]{};
        auto length = readlink(path.c_str(), target, sizeof(target) - 1U);
        if (length > 0)
        {
            target[length] = '\0';
            if (std::string{target}.find("memfd:rialto_avbuf") != std::string::npos)
            {
                ++count;
            }
        }
    }
    closedir(directory);
    return count;
}

pid_t findRialtoServerProcess()
{
    char expectedPath[PATH_MAX]{};
    if (!realpath(RIALTO_SERVER_PATH, expectedPath))
    {
        return -1;
    }
    DIR *directory = opendir("/proc");
    if (!directory)
    {
        return -1;
    }
    pid_t result{-1};
    while (auto *entry = readdir(directory))
    {
        char *end{};
        std::int64_t pid = std::strtoll(entry->d_name, &end, 10);
        if (!end || *end != '\0' || pid <= 0)
        {
            continue;
        }
        std::string executable{"/proc/" + std::to_string(pid) + "/exe"};
        char path[PATH_MAX]{};
        auto length = readlink(executable.c_str(), path, sizeof(path) - 1U);
        if (length > 0)
        {
            path[length] = '\0';
            if (std::string{path} == expectedPath)
            {
                result = static_cast<pid_t>(pid);
                break;
            }
        }
    }
    closedir(directory);
    return result;
}

class PerInstanceSharedMemoryProcessTest : public testing::Test
{
protected:
    void SetUp() override
    {
        m_observer = std::make_shared<StateObserver>();
        firebolt::rialto::common::ServerManagerConfig config;
        config.sessionServerEnvVars = {"RIALTO_CONSOLE_LOG=1", "RIALTO_DEBUG=5"};
        config.numOfPreloadedServers = 0;
        config.sessionServerPath = RIALTO_SERVER_PATH;
        config.sessionServerStartupTimeout = std::chrono::milliseconds{3000};
        config.healthcheckInterval = std::chrono::seconds{60};
        config.numOfFailedPingsBeforeRecovery = 3;
        m_manager = rialto::servermanager::service::create(m_observer, config);
        startServer();
    }

    void TearDown() override
    {
        if (m_manager)
        {
            m_manager->changeSessionServerState(kAppId, firebolt::rialto::common::SessionServerState::NOT_RUNNING);
        }
        unlink(kSocketPath);
    }

    void startServer()
    {
        firebolt::rialto::common::AppConfig appConfig{kSocketPath, "process-test"};
        ASSERT_TRUE(
            m_manager->initiateApplication(kAppId, firebolt::rialto::common::SessionServerState::ACTIVE, appConfig));
        ASSERT_TRUE(m_observer->waitFor(firebolt::rialto::common::SessionServerState::ACTIVE));
        ASSERT_EQ(m_manager->getAppConnectionInfo(kAppId), kSocketPath);
    }

    std::shared_ptr<StateObserver> m_observer;
    std::unique_ptr<rialto::servermanager::service::IServerManagerService> m_manager;
};

TEST_F(PerInstanceSharedMemoryProcessTest, ApplicationDeathReleasesServerCapacity)
{
    auto firstClient = launchClient();
    ASSERT_GT(firstClient.pid, 0);
    auto firstInodes = parseInodes(readLine(firstClient), "READY");
    ASSERT_EQ(firstInodes.size(), 2U);
    EXPECT_NE(firstInodes[0], firstInodes[1]);

    pid_t serverPid = findRialtoServerProcess();
    ASSERT_GT(serverPid, 0);
    EXPECT_EQ(countProcessMemfds(serverPid), 2U);

    ASSERT_EQ(kill(firstClient.pid, SIGKILL), 0);
    ASSERT_EQ(waitpid(firstClient.pid, nullptr, 0), firstClient.pid);
    firstClient.pid = -1;

    auto deadline = std::chrono::steady_clock::now() + kTimeout;
    while (countProcessMemfds(serverPid) != 0U && std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds{20});
    }
    ASSERT_EQ(countProcessMemfds(serverPid), 0U);

    auto replacementClient = launchClient();
    ASSERT_GT(replacementClient.pid, 0);
    auto replacementInodes = parseInodes(readLine(replacementClient), "READY");
    ASSERT_EQ(replacementInodes.size(), 2U);
    EXPECT_NE(replacementInodes[0], replacementInodes[1]);
}

TEST_F(PerInstanceSharedMemoryProcessTest, ServerRestartRequiresFreshMappings)
{
    auto client = launchClient();
    ASSERT_GT(client.pid, 0);
    auto firstInodes = parseInodes(readLine(client), "READY");
    ASSERT_EQ(firstInodes.size(), 2U);

    pid_t serverPid = findRialtoServerProcess();
    ASSERT_GT(serverPid, 0);
    ASSERT_EQ(kill(serverPid, SIGKILL), 0);

    bool mappingsReleased{false};
    auto deadline = std::chrono::steady_clock::now() + kTimeout;
    while (std::chrono::steady_clock::now() < deadline)
    {
        ASSERT_TRUE(sendCommand(client, "COUNT"));
        if (parseInodes(readLine(client), "COUNT").empty())
        {
            mappingsReleased = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{20});
    }
    ASSERT_TRUE(mappingsReleased);
    ASSERT_TRUE(m_observer->waitForActiveGeneration(2U));

    ASSERT_TRUE(sendCommand(client, "RECREATE"));
    auto replacementInodes = parseInodes(readLine(client), "RECREATED");
    ASSERT_EQ(replacementInodes.size(), 2U);
    EXPECT_NE(replacementInodes[0], replacementInodes[1]);
    for (auto inode : replacementInodes)
    {
        EXPECT_EQ(std::find(firstInodes.begin(), firstInodes.end(), inode), firstInodes.end());
    }
}

TEST_F(PerInstanceSharedMemoryProcessTest, BothProcessesCanTerminateAndRestartCleanly)
{
    auto client = launchClient();
    ASSERT_GT(client.pid, 0);
    ASSERT_EQ(parseInodes(readLine(client), "READY").size(), 2U);

    pid_t serverPid = findRialtoServerProcess();
    ASSERT_GT(serverPid, 0);
    ASSERT_EQ(kill(client.pid, SIGKILL), 0);
    ASSERT_EQ(kill(serverPid, SIGKILL), 0);
    ASSERT_EQ(waitpid(client.pid, nullptr, 0), client.pid);
    client.pid = -1;

    ASSERT_TRUE(m_observer->waitForActiveGeneration(2U));
    auto replacementClient = launchClient();
    ASSERT_GT(replacementClient.pid, 0);
    auto replacementInodes = parseInodes(readLine(replacementClient), "READY");
    ASSERT_EQ(replacementInodes.size(), 2U);
    EXPECT_NE(replacementInodes[0], replacementInodes[1]);
}
} // namespace
