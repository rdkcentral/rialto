/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 Sky UK
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

#include "CgroupsManager.h"
#include "RialtoServerManagerLogging.h"
#include "LinuxWrapper.h"

namespace rialto::servermanager::service
{
CgroupsManager::CgroupsManager(const CgroupsManagerConfig &config)
: m_cgroupPath{"/sys/fs/cgroup/memory/RialtoServer/"}
{
    RIALTO_SERVER_MANAGER_LOG_DEBUG("CgroupsManager is constructed with maxMemoryUsageMb: %u", config.maxMemoryUsageMb);
    const std::string cgroupMemoryControllerPath = "/sys/fs/cgroup/memory/RialtoServer";

    if (mkdir(m_cgroupPath.c_str(), 0755) && errno != EEXIST)
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Failed to create cgroup directory: %s", strerror(errno));
        return;
    }

    std::string memoryLimitFile = m_cgroupPath + "/memory.limit_in_bytes";
    int fd = open(memoryLimitFile.c_str(), O_WRONLY | O_CLOEXEC);
    if (fd != -1)
    {
        std::string limitStr = std::to_string(config.maxMemoryUsageBytes);
        if (write(fd, limitStr.c_str(), limitStr.length()) == static_cast<ssize_t>(limitStr.length()))
        {
            RIALTO_SERVER_MANAGER_LOG_INFO("Set memory limit to %lu bytes", m_maxMemoryUsageBytes);
        }
        else
        {
            RIALTO_SERVER_MANAGER_LOG_ERROR("Failed to write memory limit: %s", strerror(errno));
        }
        close(fd);
    }
    else
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Failed to open memory limit file: %s", strerror(errno));
    }
}

CgroupsManager::~CgroupsManager()
{
    RIALTO_SERVER_MANAGER_LOG_DEBUG("CgroupsManager is destructed");
    // Clean up the cgroup directory
    if (rmdir(m_cgroupPath.c_str()) != 0)
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Failed to remove cgroup directory: %s", strerror(errno));
    }
    else
    {
        RIALTO_SERVER_MANAGER_LOG_DEBUG("Removed cgroup directory successfully");
    }
    
}

bool CgroupsManager::addServer(const uint32_t pid)
{
    RIALTO_SERVER_MANAGER_LOG_DEBUG("Adding server with PID: %u", pid);
    std::string cgroupFile = m_cgroupPath + "/cgroup.procs";
    int fd = open(cgroupFile.c_str(), O_WRONLY | O_CLOEXEC);
    if (fd == -1)
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Failed to open cgroup.procs file: %s", strerror(errno));
        return false;
    }
    std::string pidStr = std::to_string(pid) + "\n";
    if (write(fd, pidStr.c_str(), pidStr.length()) != static_cast<ssize_t>(pidStr.length()))
    {
        RIALTO_SERVER_MANAGER_LOG_ERROR("Failed to write PID to cgroup.procs: %s", strerror(errno));
        close(fd);
        return false;
    }
    close(fd);
    RIALTO_SERVER_MANAGER_LOG_INFO("Added server with PID %u to cgroup", pid);
    return true;
}

} // namespace rialto::servermanager::service