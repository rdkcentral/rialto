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

#ifndef FIREBOLT_RIALTO_COMMON_PROFILER_MOCK_H_
#define FIREBOLT_RIALTO_COMMON_PROFILER_MOCK_H_

#include "IProfiler.h"
#include <gmock/gmock.h>
#include <optional>
#include <string>

namespace firebolt::rialto::common
{
class ProfilerMock : public IProfiler
{
public:
    ProfilerMock() = default;
    virtual ~ProfilerMock() = default;

    MOCK_METHOD(bool, enabled, (), (const, noexcept, override));

    MOCK_METHOD(std::optional<RecordId>, record, (std::string stage), (override));
    MOCK_METHOD(std::optional<RecordId>, record, (std::string stage, std::string info), (override));

    MOCK_METHOD(std::optional<RecordId>, find, (std::string stage), (override));
    MOCK_METHOD(std::optional<RecordId>, find, (std::string stage, std::string info), (override));

    MOCK_METHOD(void, log, (RecordId id), (override));

    MOCK_METHOD(bool, dump, (const std::string &path), (const, override));
};
} // namespace firebolt::rialto::common

#endif // FIREBOLT_RIALTO_COMMON_PROFILER_MOCK_H_
