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

#include "RdkPerfWrapper.h"
#include <sys/stat.h>

namespace firebolt::rialto::wrappers
{
std::unique_ptr<IRdkPerfWrapper> RdkPerfWrapperFactory::createRdkPerfWrapper(const char *szName) const
{
    return std::make_unique<RdkPerfWrapper>(szName);
}

RdkPerfWrapper::RdkPerfWrapper(const char *szName) : m_perf(szName) {}
} // namespace firebolt::rialto::wrappers
