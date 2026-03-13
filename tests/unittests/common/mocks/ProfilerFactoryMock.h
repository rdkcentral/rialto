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

#ifndef FIREBOLT_RIALTO_COMMON_PROFILER_FACTORY_MOCK_H_
#define FIREBOLT_RIALTO_COMMON_PROFILER_FACTORY_MOCK_H_

#include "IProfiler.h"
#include <gmock/gmock.h>
#include <memory>
#include <string>

namespace firebolt::rialto::common
{
class ProfilerFactoryMock : public IProfilerFactory
{
public:
    ProfilerFactoryMock() = default;
    virtual ~ProfilerFactoryMock() = default;

    MOCK_METHOD(std::unique_ptr<IProfiler>, createProfiler, (std::string moduleName), (const, override));
};
} // namespace firebolt::rialto::common

#endif // FIREBOLT_RIALTO_COMMON_PROFILER_FACTORY_MOCK_H_
