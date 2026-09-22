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

#include "IMediaPipeline.h"
#include "MediaPipelineClientMock.h"
#include <algorithm>
#include <chrono>
#include <dirent.h>
#include <gmock/gmock.h>
#include <iostream>
#include <limits.h>
#include <memory>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace
{
std::vector<ino_t> getMemfdInodes()
{
    std::vector<ino_t> inodes;
    DIR *directory = opendir("/proc/self/fd");
    if (!directory)
    {
        return inodes;
    }

    while (auto *entry = readdir(directory))
    {
        if (entry->d_name[0] == '.')
        {
            continue;
        }
        std::string path{"/proc/self/fd/" + std::string{entry->d_name}};
        char target[PATH_MAX]{};
        auto length = readlink(path.c_str(), target, sizeof(target) - 1U);
        if (length <= 0)
        {
            continue;
        }
        target[length] = '\0';
        if (std::string{target}.find("memfd:rialto_avbuf") == std::string::npos)
        {
            continue;
        }
        struct stat status
        {
        };
        if (stat(path.c_str(), &status) == 0)
        {
            inodes.emplace_back(status.st_ino);
        }
    }
    closedir(directory);
    std::sort(inodes.begin(), inodes.end());
    return inodes;
}

bool createPipelines(std::vector<std::unique_ptr<firebolt::rialto::IMediaPipeline>> &pipelines, int count = 2)
{
    auto factory = firebolt::rialto::IMediaPipelineFactory::createFactory();
    auto client = std::make_shared<testing::NiceMock<firebolt::rialto::MediaPipelineClientMock>>();
    constexpr firebolt::rialto::VideoRequirements kRequirements{1920, 1080};
    for (int i = 0; i < count; ++i)
    {
        auto pipeline = factory->createMediaPipeline(client, kRequirements);
        if (!pipeline)
        {
            return false;
        }
        pipelines.emplace_back(std::move(pipeline));
    }
    return true;
}

void report(const std::string &state)
{
    auto inodes = getMemfdInodes();
    std::cout << state << ' ' << inodes.size();
    for (auto inode : inodes)
    {
        std::cout << ' ' << inode;
    }
    std::cout << std::endl;
}
} // namespace

int main(int argc, char **argv)
{
    if (argc != 2 || setenv("RIALTO_SOCKET_PATH", argv[1], 1) != 0)
    {
        return 2;
    }

    std::vector<std::unique_ptr<firebolt::rialto::IMediaPipeline>> pipelines;
    if (!createPipelines(pipelines, 1))
    {
        std::cout << "CREATE_FAILED" << std::endl;
        return 3;
    }
    // The first creation starts client registration; wait for RUNNING before measuring stable instance mappings.
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
    pipelines.clear();
    if (!createPipelines(pipelines))
    {
        std::cout << "CREATE_FAILED" << std::endl;
        return 3;
    }
    report("READY");

    std::string command;
    while (std::getline(std::cin, command))
    {
        if (command == "COUNT")
        {
            report("COUNT");
        }
        else if (command == "RECREATE")
        {
            bool staleAccepted = std::any_of(pipelines.begin(), pipelines.end(), [](const auto &pipeline)
                                             { return pipeline->load(firebolt::rialto::MediaType::MSE, "", "", false); });
            pipelines.clear();
            if (staleAccepted || !createPipelines(pipelines))
            {
                std::cout << "RECREATE_FAILED" << std::endl;
                continue;
            }
            report("RECREATED");
        }
        else if (command == "EXIT")
        {
            return 0;
        }
    }
    return 0;
}
