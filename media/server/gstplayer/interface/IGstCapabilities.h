/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Sky UK
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

#ifndef FIREBOLT_RIALTO_SERVER_I_GST_CAPABILITIES_H_
#define FIREBOLT_RIALTO_SERVER_I_GST_CAPABILITIES_H_

#include <MediaCommon.h>
#include <memory>
#include <string>
#include <vector>

namespace firebolt::rialto::server
{
class IGstCapabilities;

/**
 * @brief IGstCapabilities factory class, returns a concrete implementation of IGstCapabilities
 */
class IGstCapabilitiesFactory
{
public:
    IGstCapabilitiesFactory() = default;
    virtual ~IGstCapabilitiesFactory() = default;

    /**
     * @brief Gets the IGstCapabilitiesFactory instance.
     *
     * @retval the factory instance or null on error.
     */
    static std::shared_ptr<IGstCapabilitiesFactory> getFactory();

    /**
     * @brief Creates a IGstCapabilities object.
     *
     *
     * @retval the new gstreamer capabilities instance or null on error.
     */
    virtual std::unique_ptr<IGstCapabilities> createGstCapabilities() = 0;
};

class IGstCapabilities
{
public:
    IGstCapabilities() = default;
    virtual ~IGstCapabilities() = default;

    IGstCapabilities(const IGstCapabilities &) = delete;
    IGstCapabilities &operator=(const IGstCapabilities &) = delete;
    IGstCapabilities(IGstCapabilities &&) = delete;
    IGstCapabilities &operator=(IGstCapabilities &&) = delete;

    /**
     * @brief Gets vector of mime types supported by gstreamer
     *
     * @retval vector of mime types supported by gstreamer
     */
    virtual std::vector<std::string> getSupportedMimeTypes(MediaSourceType sourceType) = 0;

    /**
     * @brief Checks is \a mimeType is supported by gstreamer
     *
     * @retval True if mime type is supported by gstreamer
     */
    virtual bool isMimeTypeSupported(const std::string &mimeType) = 0;

    /**
     * @brief  Has any gstreamer sink or decoder got a named property
     *
     * @retval true if any gstreamer sink or decoder has the property
     */
    virtual std::vector<std::string> getSupportedProperties(MediaSourceType mediaType,
                                                            const std::vector<std::string> &propertyNames) = 0;
};

}; // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_I_GST_CAPABILITIES_H_
