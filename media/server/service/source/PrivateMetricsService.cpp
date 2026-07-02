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

#include "PrivateMetricsService.h"
#include "RialtoServerLogging.h"
#include <cinttypes>
#include <fstream>
#include <string>

namespace firebolt::rialto::server::service
{
PrivateMetricsService::PrivateMetricsService(
    std::shared_ptr<firebolt::rialto::server::IMetricsCollectorFactory> collectorFactory)
    : m_collectorFactory{std::move(collectorFactory)}
{
}

PrivateMetricsService::~PrivateMetricsService()
{
    std::lock_guard<std::mutex> lock{m_mutex};
    m_collectors.clear();
}

void PrivateMetricsService::clientReady(int clientId,
                                        const std::shared_ptr<firebolt::rialto::server::IMetricsCollectorClient> &client)
{
    std::lock_guard<std::mutex> lock{m_mutex};
    auto collector = m_collectorFactory->create(clientId, client);
    if (collector)
    {
        m_collectors.emplace(clientId, std::move(collector));
        RIALTO_SERVER_LOG_INFO("MetricsCollector created for client %d", clientId);
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create MetricsCollector for client %d", clientId);
    }
}

void PrivateMetricsService::clientDisconnected(int clientId)
{
    std::lock_guard<std::mutex> lock{m_mutex};
    auto iter = m_collectors.find(clientId);
    if (iter != m_collectors.end())
    {
        m_collectors.erase(iter);
        RIALTO_SERVER_LOG_INFO("MetricsCollector destroyed for client %d", clientId);
    }
}

void PrivateMetricsService::reportMetrics(int clientId, const firebolt::rialto::server::ClientMetricsData &metrics)
{
    std::lock_guard<std::mutex> lock{m_mutex};
    auto iter = m_collectors.find(clientId);
    if (iter != m_collectors.end())
    {
        iter->second->processMetrics(metrics);
    }
    else
    {
        RIALTO_SERVER_LOG_WARN("reportMetrics for unknown client %d", clientId);
    }
}

void PrivateMetricsService::notifyPlaybackStateChanged(int sessionId, PlaybackState oldState, PlaybackState newState)
{
    std::lock_guard<std::mutex> lock{m_mutex};
    for (auto &[clientId, collector] : m_collectors)
    {
        collector->notifyPlaybackStateChanged(sessionId, oldState, newState);
    }
}

void PrivateMetricsService::notifyApplicationStateChanged(ApplicationState oldState, ApplicationState newState)
{
    std::lock_guard<std::mutex> lock{m_mutex};
    for (auto &[clientId, collector] : m_collectors)
    {
        collector->notifyApplicationStateChanged(oldState, newState);
    }

    // When transitioning to INACTIVE, record a server-side memory snapshot.
    // At this point, pipelines and shared memory have already been freed but
    // no client may be connected to supply a full sample — so we read the
    // server's own memory directly.
    if (newState == ApplicationState::INACTIVE)
    {
        std::uint64_t serverMemoryKb{0};
        {
            std::ifstream status{"/proc/self/status"};
            std::string line;
            while (std::getline(status, line))
            {
                if (line.rfind("VmRSS:", 0) == 0)
                {
                    std::sscanf(line.c_str(), "VmRSS: %" SCNu64, &serverMemoryKb);
                    break;
                }
            }
        }

        std::uint64_t cgroupMemoryUsageKb{0};
        {
            auto readFileValue = [](const std::string &path) -> std::uint64_t
            {
                std::ifstream file{path};
                if (!file.is_open())
                {
                    return 0;
                }
                std::string content;
                if (!std::getline(file, content) || content.empty() || content == "max")
                {
                    return 0;
                }
                std::uint64_t value{0};
                if (std::sscanf(content.c_str(), "%" SCNu64, &value) == 1)
                {
                    return value;
                }
                return 0;
            };

            // Resolve the process's cgroup path from /proc/self/cgroup
            std::ifstream cgroupFile{"/proc/self/cgroup"};
            std::string cgroupBase;
            if (cgroupFile.is_open())
            {
                std::string line;
                while (std::getline(cgroupFile, line))
                {
                    if (line.rfind("0::", 0) == 0)
                    {
                        std::string relativePath{line.substr(3)};
                        if (!relativePath.empty() && relativePath != "/")
                        {
                            cgroupBase = "/sys/fs/cgroup" + relativePath;
                        }
                        else
                        {
                            cgroupBase = "/sys/fs/cgroup";
                        }
                        break;
                    }
                }
            }

            std::uint64_t usageBytes{0};
            if (!cgroupBase.empty())
            {
                usageBytes = readFileValue(cgroupBase + "/memory.current");
            }
            if (usageBytes == 0)
            {
                usageBytes = readFileValue("/sys/fs/cgroup/memory/memory.usage_in_bytes");
            }
            cgroupMemoryUsageKb = usageBytes / 1024;
        }

        // Read smaps_rollup to split private-dirty heap from file-backed libs.
        std::uint64_t anonKb{0}, sharedCleanKb{0}, privateCleanKb{0}, privateDirtyKb{0};
        {
            std::ifstream smaps{"/proc/self/smaps_rollup"};
            std::string sline;
            while (std::getline(smaps, sline))
            {
                if (sline.rfind("Anonymous:", 0) == 0)
                    std::sscanf(sline.c_str(), "Anonymous: %" SCNu64, &anonKb);
                else if (sline.rfind("Shared_Clean:", 0) == 0)
                    std::sscanf(sline.c_str(), "Shared_Clean: %" SCNu64, &sharedCleanKb);
                else if (sline.rfind("Private_Clean:", 0) == 0)
                    std::sscanf(sline.c_str(), "Private_Clean: %" SCNu64, &privateCleanKb);
                else if (sline.rfind("Private_Dirty:", 0) == 0)
                    std::sscanf(sline.c_str(), "Private_Dirty: %" SCNu64, &privateDirtyKb);
            }
        }
        RIALTO_SERVER_LOG_MIL("Metrics: INACTIVE memory snapshot — server_mem_kb=%" PRIu64
                              ", cgroup_mem_kb=%" PRIu64
                              ", anon_kb=%" PRIu64
                              ", private_dirty_kb=%" PRIu64
                              ", private_clean_kb=%" PRIu64
                              ", shared_clean_kb=%" PRIu64,
                              serverMemoryKb, cgroupMemoryUsageKb,
                              anonKb, privateDirtyKb, privateCleanKb, sharedCleanKb);
    }
}
} // namespace firebolt::rialto::server::service
