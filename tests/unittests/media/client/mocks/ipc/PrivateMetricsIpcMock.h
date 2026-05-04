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

#ifndef FIREBOLT_RIALTO_CLIENT_PRIVATE_METRICS_IPC_MOCK_H_
#define FIREBOLT_RIALTO_CLIENT_PRIVATE_METRICS_IPC_MOCK_H_

#include "IPrivateMetricsIpc.h"
#include <gmock/gmock.h>

namespace firebolt::rialto::client
{
class PrivateMetricsIpcMock : public IPrivateMetricsIpc
{
public:
    PrivateMetricsIpcMock() = default;
    virtual ~PrivateMetricsIpcMock() = default;

    MOCK_METHOD(bool, reportClientMetrics,
                (std::uint64_t sampleId, std::uint32_t reason, const std::string &appName, std::uint32_t processId,
                 std::uint64_t monotonicTimeMs, std::uint64_t epochTimeMs, std::uint64_t processCpuTimeMs),
                (override));
};
} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_PRIVATE_METRICS_IPC_MOCK_H_
