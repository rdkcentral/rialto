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

#ifndef FIREBOLT_RIALTO_CLIENT_I_PRIVATE_METRICS_IPC_H_
#define FIREBOLT_RIALTO_CLIENT_I_PRIVATE_METRICS_IPC_H_

#include <cstdint>
#include <memory>
#include <string>

namespace firebolt::rialto::client
{
class IPrivateMetricsIpc;

class IPrivateMetricsIpcClient
{
public:
    IPrivateMetricsIpcClient() = default;
    virtual ~IPrivateMetricsIpcClient() = default;

    IPrivateMetricsIpcClient(const IPrivateMetricsIpcClient &) = delete;
    IPrivateMetricsIpcClient &operator=(const IPrivateMetricsIpcClient &) = delete;
    IPrivateMetricsIpcClient(IPrivateMetricsIpcClient &&) = delete;
    IPrivateMetricsIpcClient &operator=(IPrivateMetricsIpcClient &&) = delete;

    virtual void reportClientMetrics(std::uint64_t sampleId, std::uint32_t reason) = 0;
};

class IPrivateMetricsIpcFactory
{
public:
    IPrivateMetricsIpcFactory() = default;
    virtual ~IPrivateMetricsIpcFactory() = default;

    static std::shared_ptr<IPrivateMetricsIpcFactory> createFactory();

    virtual std::shared_ptr<IPrivateMetricsIpc> createPrivateMetricsIpc(IPrivateMetricsIpcClient *client) = 0;
};

class IPrivateMetricsIpc
{
public:
    IPrivateMetricsIpc() = default;
    virtual ~IPrivateMetricsIpc() = default;

    IPrivateMetricsIpc(const IPrivateMetricsIpc &) = delete;
    IPrivateMetricsIpc &operator=(const IPrivateMetricsIpc &) = delete;
    IPrivateMetricsIpc(IPrivateMetricsIpc &&) = delete;
    IPrivateMetricsIpc &operator=(IPrivateMetricsIpc &&) = delete;

    virtual bool reportClientMetrics(std::uint64_t sampleId, std::uint32_t reason, const std::string &appName,
                                     std::uint32_t processId, std::uint64_t monotonicTimeMs,
                                     std::uint64_t epochTimeMs, std::uint64_t processCpuTimeMs,
                                     std::uint64_t processMemoryKb) = 0;
};
} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_I_PRIVATE_METRICS_IPC_H_
