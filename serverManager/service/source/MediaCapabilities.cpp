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

#include "MediaCapabilities.h"
#include "RialtoServerManagerLogging.h"
#include <chrono>

namespace rialto::servermanager::service
{
MediaCapabilities::MediaCapabilities(std::shared_ptr<firebolt::rialto::wrappers::IYamlCppWrapper> yamlCppWrapper)
    : m_yamlCppWrapper{std::move(yamlCppWrapper)}
{
}

firebolt::rialto::DecoderCapabilitiesStatus
MediaCapabilities::getAudioDecoderCapabilities(firebolt::rialto::common::AudioDecoderCapabilities &capabilities)
{
    RIALTO_SERVER_MANAGER_LOG_DEBUG("MediaCapabilities: loading audio capabilities from YAML");

    capabilities = {};
    const auto status = m_yamlCppWrapper->getAudioDecoderCapabilities(capabilities);
    if (status == firebolt::rialto::DecoderCapabilitiesStatus::OK)
        RIALTO_SERVER_MANAGER_LOG_INFO("MediaCapabilities: audio capabilities loaded successfully");
    else if (status == firebolt::rialto::DecoderCapabilitiesStatus::CONFIG_NOT_FOUND)
        RIALTO_SERVER_MANAGER_LOG_INFO("MediaCapabilities: HFP YAML config not found - returning empty capabilities");
    else
        RIALTO_SERVER_MANAGER_LOG_WARN("MediaCapabilities: YAML schema validation or internal error for audio");
    return status;
}

firebolt::rialto::DecoderCapabilitiesStatus
MediaCapabilities::getVideoDecoderCapabilities(firebolt::rialto::common::VideoDecoderCapabilities &capabilities)
{
    RIALTO_SERVER_MANAGER_LOG_DEBUG("MediaCapabilities: loading video capabilities from YAML");

    capabilities = {};
    const auto status = m_yamlCppWrapper->getVideoDecoderCapabilities(capabilities);
    if (status == firebolt::rialto::DecoderCapabilitiesStatus::OK)
        RIALTO_SERVER_MANAGER_LOG_INFO("MediaCapabilities: video capabilities loaded successfully");
    else if (status == firebolt::rialto::DecoderCapabilitiesStatus::CONFIG_NOT_FOUND)
        RIALTO_SERVER_MANAGER_LOG_INFO("MediaCapabilities: HFP YAML config not found - returning empty capabilities");
    else
        RIALTO_SERVER_MANAGER_LOG_WARN("MediaCapabilities: YAML schema validation or internal error for video");
    return status;
}

} // namespace rialto::servermanager::service
