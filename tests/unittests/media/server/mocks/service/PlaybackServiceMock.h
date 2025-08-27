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

#ifndef FIREBOLT_RIALTO_SERVER_SERVICE_PLAYBACK_SERVICE_MOCK_H_
#define FIREBOLT_RIALTO_SERVER_SERVICE_PLAYBACK_SERVICE_MOCK_H_

#include "IPlaybackService.h"
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <vector>

namespace firebolt::rialto::server::service
{
class PlaybackServiceMock : public IPlaybackService
{
public:
    MOCK_METHOD(bool, switchToActive, (), (override));
    MOCK_METHOD(void, switchToInactive, (), (override));
    MOCK_METHOD(void, setMaxPlaybacks, (int maxPlaybacks), (override));
    MOCK_METHOD(void, setMaxWebAudioPlayers, (int maxWebAudio), (override));
    MOCK_METHOD(void, setClientDisplayName, (const std::string &clientDisplayName), (const, override));
    MOCK_METHOD(void, setResourceManagerAppName, (const std::string &appName), (const, override));
    MOCK_METHOD(bool, isActive, (), (const, override));
    MOCK_METHOD(bool, getSharedMemory, (int32_t & fd, uint32_t &size), (const, override));
    MOCK_METHOD(int, getMaxPlaybacks, (), (const, override));
    MOCK_METHOD(int, getMaxWebAudioPlayers, (), (const, override));
    MOCK_METHOD(std::shared_ptr<ISharedMemoryBuffer>, getShmBuffer, (), (const, override));
    MOCK_METHOD(IMediaPipelineService &, getMediaPipelineService, (), (const, override));
    MOCK_METHOD(IWebAudioPlayerService &, getWebAudioPlayerService, (), (const, override));
    MOCK_METHOD(void, ping, (const std::shared_ptr<IHeartbeatProcedure> &heartbeatProcedure), (const, override));
    MOCK_METHOD(void, setSubtitleResyncInterval, (const std::chrono::seconds subtitleResyncInterva), (override));
};
} // namespace firebolt::rialto::server::service

#endif // FIREBOLT_RIALTO_SERVER_SERVICE_PLAYBACK_SERVICE_MOCK_H_
