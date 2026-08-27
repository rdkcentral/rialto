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

#ifndef FIREBOLT_RIALTO_I_MEDIA_CAPABILITIES_H_
#define FIREBOLT_RIALTO_I_MEDIA_CAPABILITIES_H_

#include "AudioDecoderCapabilities.h"
#include "VideoDecoderCapabilities.h"
#include <memory>

namespace firebolt::rialto
{
class IMediaCapabilities;

/**
 * @brief IMediaCapabilities factory class, for getting the IMediaCapabilities object.
 */
class IMediaCapabilitiesFactory
{
public:
    IMediaCapabilitiesFactory() = default;
    virtual ~IMediaCapabilitiesFactory() = default;

    /**
     * @brief Gets the IMediaCapabilitiesFactory instance.
     *
     * @retval the factory instance or null on error.
     */
    static std::shared_ptr<IMediaCapabilitiesFactory> createFactory();

    /**
     * @brief Creates the IMediaCapabilities object.
     *
     * @retval the IMediaCapabilities instance or null on error.
     */
    virtual std::unique_ptr<IMediaCapabilities> createMediaCapabilities() const = 0;
};

/**
 * @brief Single capabilities query interface replacing IMediaPipelineCapabilities capability methods.
 *        If the ServerManager supplied HFP YAML data at startup, it is returned directly;
 *        otherwise the session server falls back to its GStreamer element-query path.
 */
class IMediaCapabilities
{
public:
    IMediaCapabilities() = default;
    virtual ~IMediaCapabilities() = default;

    IMediaCapabilities(const IMediaCapabilities &) = delete;
    IMediaCapabilities &operator=(const IMediaCapabilities &) = delete;
    IMediaCapabilities(IMediaCapabilities &&) = delete;
    IMediaCapabilities &operator=(IMediaCapabilities &&) = delete;

    /**
     * @brief Returns the audio decoder capabilities supported by Rialto.
     *
     * @retval the supported audio decoder capabilities.
     */
    virtual firebolt::rialto::common::AudioDecoderCapabilities getSupportedAudioCapabilities() = 0;

    /**
     * @brief Returns the video decoder capabilities supported by Rialto.
     *
     * @retval the supported video decoder capabilities.
     */
    virtual firebolt::rialto::common::VideoDecoderCapabilities getSupportedVideoCapabilities() = 0;
};

} // namespace firebolt::rialto

#endif // FIREBOLT_RIALTO_I_MEDIA_CAPABILITIES_H_
