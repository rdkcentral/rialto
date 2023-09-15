/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Sky UK
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

#ifndef FIREBOLT_RIALTO_SERVER_GST_WEB_AUDIO_PLAYER_MOCK_H_
#define FIREBOLT_RIALTO_SERVER_GST_WEB_AUDIO_PLAYER_MOCK_H_

#include "IGstWebAudioPlayer.h"
#include <gmock/gmock.h>
#include <memory>
#include <string>

namespace firebolt::rialto::server
{
class GstWebAudioPlayerMock : public IGstWebAudioPlayer
{
public:
    GstWebAudioPlayerMock() = default;
    virtual ~GstWebAudioPlayerMock() = default;

    MOCK_METHOD(void, setCaps, (const std::string &audioMimeType, const WebAudioConfig *config), (override));
    MOCK_METHOD(void, play, (), (override));
    MOCK_METHOD(void, pause, (), (override));
    MOCK_METHOD(void, setVolume, (double volume), (override));
    MOCK_METHOD(bool, getVolume, (double &volume), (override));
    MOCK_METHOD(uint32_t, writeBuffer, (uint8_t * mainPtr, uint32_t mainLength, uint8_t *wrapPtr, uint32_t wrapLength),
                (override));
    MOCK_METHOD(void, setEos, (), (override));
    MOCK_METHOD(uint64_t, getQueuedBytes, (), (override));
    MOCK_METHOD(void, ping, (std::unique_ptr<IHeartbeatHandler> && heartbeatHandler), (override));
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_GST_WEB_AUDIO_PLAYER_MOCK_H_
