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

#ifndef FIREBOLT_RIALTO_CLIENT_WEB_AUDIO_PLAYER_H_
#define FIREBOLT_RIALTO_CLIENT_WEB_AUDIO_PLAYER_H_

#include "IWebAudioPlayer.h"

#include <memory>
#include <stdint.h>
#include <string>

namespace firebolt::rialto
{
/**
 * @brief IWebAudioPlayer factory class definition.
 */
class WebAudioPlayerFactory : public IWebAudioPlayerFactory
{
public:
    WebAudioPlayerFactory() = default;
    ~WebAudioPlayerFactory() override = default;

    std::unique_ptr<IWebAudioPlayer> createWebAudioPlayer(std::weak_ptr<IWebAudioPlayerClient> client,
                                                          const std::string &audioMimeType, const uint32_t priority,
                                                          const WebAudioConfig *config) const override;
};

}; // namespace firebolt::rialto

namespace firebolt::rialto::client
{
/**
 * @brief The definition of the WebAudioPlayer.
 */
class WebAudioPlayer : public IWebAudioPlayer
{
public:
    /**
     * @brief The constructor.
     *
     * @param[in] client:        The Web Audio Player client
     * @param[in] audioMimeType: The audio encoding format, currently only "audio/x-raw" (PCM)
     * @param[in] priority:      Priority value for this pipeline.
     * @param[in] config:        Additional type dependent configuration data or nullptr
     */
    WebAudioPlayer(std::weak_ptr<IWebAudioPlayerClient> client, const std::string &audioMimeType,
                   const uint32_t priority, const WebAudioConfig *config);

    /**
     * @brief Virtual destructor.
     */
    virtual ~WebAudioPlayer();

    bool play() override;

    bool pause() override;

    bool setEos() override;

    bool getBufferAvailable(uint32_t &availableFrames, const std::shared_ptr<WebAudioShmInfo> &webAudioShmInfo) override;

    bool getBufferDelay(uint32_t &delayFrames) override;

    bool writeBuffer(const uint32_t numberOfFrames, void *data) override;

    bool getDeviceInfo(uint32_t &preferredFrames, uint32_t &maximumFrames, bool &supportDeferredPlay) override;

    bool setVolume(double volume) override;

    bool getVolume(double &volume) override;

    std::weak_ptr<IWebAudioPlayerClient> getClient() override;

protected:
    /**
     * @brief The web audio player client.
     */
    std::weak_ptr<IWebAudioPlayerClient> m_webAudioPlayerClient;
};

}; // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_WEB_AUDIO_PLAYER_H_
