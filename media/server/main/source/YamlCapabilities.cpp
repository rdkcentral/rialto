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

#include "YamlCapabilities.h"
#include "RialtoServerLogging.h"

namespace firebolt::rialto::server
{
YamlCapabilities::YamlCapabilities(const std::shared_ptr<firebolt::rialto::wrappers::IYamlCppWrapper> &yamlCppWrapper)
    : m_yamlCppWrapper{yamlCppWrapper}
{
    RIALTO_SERVER_LOG_DEBUG("YamlCapabilities: constructor - reads decoder capabilities from YAML files only");
}

firebolt::rialto::common::DecoderCapabilitiesStatus
YamlCapabilities::getAudioDecoderCapabilities(firebolt::rialto::common::AudioDecoderCapabilities &audioDecoderCapabilities)
{
    if (!m_yamlCppWrapper)
    {
        RIALTO_SERVER_LOG_WARN("YamlCapabilities: YAML wrapper not available");
        return firebolt::rialto::common::DecoderCapabilitiesStatus::INTERNAL_ERROR;
    }

    RIALTO_SERVER_LOG_DEBUG("YamlCapabilities: Reading audio decoder capabilities from YAML");
    return m_yamlCppWrapper->getAudioDecoderCapabilities(audioDecoderCapabilities);
}

firebolt::rialto::common::DecoderCapabilitiesStatus
YamlCapabilities::getVideoDecoderCapabilities(firebolt::rialto::common::VideoDecoderCapabilities &videoDecoderCapabilities)
{
    if (!m_yamlCppWrapper)
    {
        RIALTO_SERVER_LOG_WARN("YamlCapabilities: YAML wrapper not available");
        return firebolt::rialto::common::DecoderCapabilitiesStatus::INTERNAL_ERROR;
    }

    RIALTO_SERVER_LOG_DEBUG("YamlCapabilities: Reading video decoder capabilities from YAML");
    return m_yamlCppWrapper->getVideoDecoderCapabilities(videoDecoderCapabilities);
}

} // namespace firebolt::rialto::server
