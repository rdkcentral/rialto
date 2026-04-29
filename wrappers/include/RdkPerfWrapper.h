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

#ifndef FIREBOLT_RIALTO_WRAPPERS_RDK_PERF_WRAPPER_H_
#define FIREBOLT_RIALTO_WRAPPERS_RDK_PERF_WRAPPER_H_

#include "IRdkPerfWrapper.h"
#include "rdk_perf.h"
#include <memory>

namespace firebolt::rialto::wrappers
{
class RdkPerfWrapperFactory : public IRdkPerfWrapperFactory
{
public:
    RdkPerfWrapperFactory() = default;
    ~RdkPerfWrapperFactory() override = default;

    std::unique_ptr<IRdkPerfWrapper> createRdkPerfWrapper(const char *szName) const override;
};

class RdkPerfWrapper : public IRdkPerfWrapper
{
public:
    explicit RdkPerfWrapper(const char *szName);
    ~RdkPerfWrapper() override = default;

private:
    RDKPerf m_perf;
};
} // namespace firebolt::rialto::wrappers

#endif // FIREBOLT_RIALTO_WRAPPERS_RDK_PERF_WRAPPER_H_
