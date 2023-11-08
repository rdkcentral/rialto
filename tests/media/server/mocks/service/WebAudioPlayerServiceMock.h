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

#ifndef FIREBOLT_RIALTO_SERVER_SERVICE_WEB_AUDIO_PLAYER_SERVICE_MOCK_H_
#define FIREBOLT_RIALTO_SERVER_SERVICE_WEB_AUDIO_PLAYER_SERVICE_MOCK_H_

#include "IWebAudioPlayerService.h"
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <vector>

namespace firebolt::rialto::server::service
{
class WebAudioPlayerServiceMock : public IWebAudioPlayerService
{
public:
    MOCK_METHOD(bool, createWebAudioPlayer,
                (int handle, const std::shared_ptr<IWebAudioPlayerClient> &webAudioPlayerClient,
                 const std::string &audioMimeType, const uint32_t priority, std::weak_ptr<const WebAudioConfig> config),
                (override));
    MOCK_METHOD(bool, destroyWebAudioPlayer, (int handle), (override));
    MOCK_METHOD(bool, play, (int handle), (override));
    MOCK_METHOD(bool, pause, (int handle), (override));
    MOCK_METHOD(bool, setEos, (int handle), (override));
    MOCK_METHOD(bool, getBufferAvailable,
                (int handle, uint32_t &availableFrames, std::shared_ptr<WebAudioShmInfo> &webAudioShmInfo), (override));
    MOCK_METHOD(bool, getBufferDelay, (int handle, uint32_t &delayFrames), (override));
    MOCK_METHOD(bool, writeBuffer, (int handle, const uint32_t numberOfFrames, void *data), (override));
    MOCK_METHOD(bool, getDeviceInfo,
                (int handle, uint32_t &preferredFrames, uint32_t &maximumFrames, bool &supportDeferredPlay), (override));
    MOCK_METHOD(bool, setVolume, (int handle, double volume), (override));
    MOCK_METHOD(bool, getVolume, (int handle, double &volume), (override));
    MOCK_METHOD(void, ping, (const std::shared_ptr<IHeartbeatProcedure> &heartbeatProcedure), (override));
};
} // namespace firebolt::rialto::server::service

#endif // FIREBOLT_RIALTO_SERVER_SERVICE_WEB_AUDIO_PLAYER_SERVICE_MOCK_H_
