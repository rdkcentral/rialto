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

#ifndef FIREBOLT_RIALTO_SERVER_I_WEB_AUDIO_PLAYER_SERVER_INTERNAL_FACTORY_H_
#define FIREBOLT_RIALTO_SERVER_I_WEB_AUDIO_PLAYER_SERVER_INTERNAL_FACTORY_H_

/**
 * @file IWebAudioPlayerServerInternal.h
 *
 * The definition of the IWebAudioPlayerServerInternal interface.
 *
 * This interface defines the server internal APIs for playback of AV content.
 */

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "IDecryptionService.h"
#include "ISharedMemoryBuffer.h"
#include "IWebAudioPlayer.h"
#include <MediaCommon.h>

namespace firebolt::rialto::server
{
/**
 * @brief IWebAudioPlayer factory class, returns a concrete implementation of IWebAudioPlayer for internal server use
 */
class IWebAudioPlayerServerInternalFactory : public IWebAudioPlayerFactory
{
public:
    virtual ~IWebAudioPlayerServerInternalFactory() = default;

    /**
     * @brief Create a IWebAudioPlayerServerInternalFactory instance.
     *
     * @retval the factory instance or null on error.
     */
    static std::shared_ptr<IWebAudioPlayerServerInternalFactory> createFactory();

    /**
     * @brief IWebAudioPlayerServerInternalFactory method, returns a concrete implementation of IWebAudioPlayer for
     * internal server use
     *
     * @param[in] client            : The Web Audio Player client
     * @param[in] audioMimeType     : The audio encoding format, currently only "audio/x-raw" (PCM)
     * @param[in] priority          : Priority value for this pipeline.
     * @param[in] config            : Additional type dependent configuration data or nullptr
     * @param[in] shmBuffer         : The shared buffer object.
     * @param[in] handle            : The handle for this WebAudioPlayer.
     *
     * @retval the new backend instance or null on error.
     */
    virtual std::unique_ptr<IWebAudioPlayer>
    createWebAudioPlayerServerInternal(std::weak_ptr<IWebAudioPlayerClient> client, const std::string &audioMimeType,
                                       const uint32_t priority, const WebAudioConfig *config,
                                       const std::shared_ptr<ISharedMemoryBuffer> &shmBuffer, int handle) const = 0;
};
}; // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_I_WEB_AUDIO_PLAYER_SERVER_INTERNAL_FACTORY_H_
