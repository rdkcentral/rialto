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

#ifndef FIREBOLT_RIALTO_SERVER_I_GST_WEB_AUDIO_PLAYER_H_
#define FIREBOLT_RIALTO_SERVER_I_GST_WEB_AUDIO_PLAYER_H_

#include <MediaCommon.h>
#include <memory>
#include <stdint.h>
#include <string>

#include "IGstWebAudioPlayerClient.h"

namespace firebolt::rialto::server
{
class IGstWebAudioPlayer;

/**
 * @brief IGstWebAudioPlayer factory class, returns a concrete implementation of IGstWebAudioPlayer
 */
class IGstWebAudioPlayerFactory
{
public:
    IGstWebAudioPlayerFactory() = default;
    virtual ~IGstWebAudioPlayerFactory() = default;

    /**
     * @brief Gets the IGstWebAudioPlayerFactory instance.
     *
     * @retval the factory instance or null on error.
     */
    static std::shared_ptr<IGstWebAudioPlayerFactory> getFactory();

    /**
     * @brief Creates a IGstWebAudioPlayer object.
     *
     * @param[in] client    : The gstreamer web audio player client.
     *
     * @retval the new player instance or null on error.
     */
    virtual std::unique_ptr<IGstWebAudioPlayer> createGstWebAudioPlayer(IGstWebAudioPlayerClient *client) = 0;
};

class IGstWebAudioPlayer
{
public:
    IGstWebAudioPlayer() = default;
    virtual ~IGstWebAudioPlayer() = default;

    IGstWebAudioPlayer(const IGstWebAudioPlayer &) = delete;
    IGstWebAudioPlayer &operator=(const IGstWebAudioPlayer &) = delete;
    IGstWebAudioPlayer(IGstWebAudioPlayer &&) = delete;
    IGstWebAudioPlayer &operator=(IGstWebAudioPlayer &&) = delete;

    /**
     * @brief Starts playback of the media.
     *
     * TODO
     */
    virtual void play() = 0;

    /**
     * @brief Pauses playback of the media.
     *
     * TODO
     */
    virtual void pause() = 0;

    /**
     * @brief TODO
     */
    virtual void setVolume(double volume) = 0;

    /**
     * @brief TODO
     * @retval True on success
     */
    virtual bool getVolume(double &volume) = 0;
};

}; // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_I_GST_WEB_AUDIO_PLAYER_H_
