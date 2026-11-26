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

#ifndef FIREBOLT_RIALTO_SERVER_COMMON_SERVER_PROFILING_H_
#define FIREBOLT_RIALTO_SERVER_COMMON_SERVER_PROFILING_H_

#include <optional>
#include <string>

#include "rdk_perf.h"

namespace firebolt::rialto::server::service::profiling
{
enum class ProfileInterface
{
    MediaPipeline = 0,
    MediaKeys,
    WebAudioPlayer,
    MediaPipelineCapabilities,
    MediaKeysCapabilities,
};

namespace Internal
{
bool isEnabledForInterface(ProfileInterface profileInterface);
std::string createScopeName(ProfileInterface profileInterface, const char *operationName);
}

class Scope final
{
public:
    inline Scope(ProfileInterface profileInterface, const char *operationName)
    {
        if (!Internal::isEnabledForInterface(profileInterface))
        {
            return;
        }

        m_scopeName = Internal::createScopeName(profileInterface, operationName);
        m_perfScope.emplace(m_scopeName.c_str());
    }

    Scope(const Scope &) = delete;
    Scope &operator=(const Scope &) = delete;
    Scope(Scope &&) = delete;
    Scope &operator=(Scope &&) = delete;
    ~Scope() = default;

private:
    std::string m_scopeName;
    std::optional<RDKPerf> m_perfScope;
};

inline Scope makeScope(ProfileInterface profileInterface, const char *operationName)
{
    return Scope{profileInterface, operationName};
}

} // namespace firebolt::rialto::server::service::profiling

#endif // FIREBOLT_RIALTO_SERVER_COMMON_SERVER_PROFILING_H_