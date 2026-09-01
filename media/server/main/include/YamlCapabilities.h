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

#ifndef FIREBOLT_RIALTO_SERVER_YAML_CAPABILITIES_H_
#define FIREBOLT_RIALTO_SERVER_YAML_CAPABILITIES_H_

#include <memory>

#include "AudioDecoderCapabilities.h"
#include "DecoderCapabilitiesCommon.h"
#include "IYamlCppWrapper.h"
#include "VideoDecoderCapabilities.h"

namespace firebolt::rialto::server
{
/**
 * @brief YAML-based capabilities reader
 *
 * This class is responsible for reading decoder capabilities from YAML configuration files.
 * It handles the YAML parsing and provides the capabilities in the standard format.
 *
 * This class does NOT perform GStreamer operations - see GstCapabilities for that.
 * Use this when you need capabilities from configuration files.
 */
class YamlCapabilities
{
public:
    /**
     * @brief Constructor
     *
     * @param yamlCppWrapper Wrapper for YAML file operations
     */
    explicit YamlCapabilities(const std::shared_ptr<firebolt::rialto::wrappers::IYamlCppWrapper> &yamlCppWrapper);

    /**
     * @brief Destructor
     */
    ~YamlCapabilities() = default;

    /**
     * @brief Get audio decoder capabilities from YAML configuration
     *
     * @param[out] audioDecoderCapabilities The audio capabilities to populate
     * @return DecoderCapabilitiesStatus indicating success/failure
     */
    firebolt::rialto::DecoderCapabilitiesStatus
    getAudioDecoderCapabilities(firebolt::rialto::common::AudioDecoderCapabilities &audioDecoderCapabilities);

    /**
     * @brief Get video decoder capabilities from YAML configuration
     *
     * @param[out] videoDecoderCapabilities The video capabilities to populate
     * @return DecoderCapabilitiesStatus indicating success/failure
     */
    firebolt::rialto::DecoderCapabilitiesStatus
    getVideoDecoderCapabilities(firebolt::rialto::common::VideoDecoderCapabilities &videoDecoderCapabilities);

private:
    std::shared_ptr<firebolt::rialto::wrappers::IYamlCppWrapper> m_yamlCppWrapper;
};

} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_YAML_CAPABILITIES_H_
