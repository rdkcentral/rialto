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

#ifndef FIREBOLT_RIALTO_SERVER_WEB_AUDIO_PLAYER_SERVER_INTERNAL_FACTORY_MOCK_H_
#define FIREBOLT_RIALTO_SERVER_WEB_AUDIO_PLAYER_SERVER_INTERNAL_FACTORY_MOCK_H_

#include "IWebAudioPlayer.h"
#include "IWebAudioPlayerServerInternalFactory.h"
#include <gmock/gmock.h>
#include <memory>

namespace firebolt::rialto::server
{
class WebAudioPlayerServerInternalFactoryMock : public IWebAudioPlayerServerInternalFactory
{
public:
    MOCK_METHOD(std::unique_ptr<IWebAudioPlayer>, createWebAudioPlayer,
                (std::weak_ptr<IWebAudioPlayerClient> client,
                                                                  const std::string &audioMimeType,
                                                                  const uint32_t priority,
                                                                  const WebAudioConfig *config),
                (const, override));
    MOCK_METHOD(std::unique_ptr<IWebAudioPlayer>, createWebAudioPlayerServerInternal,
                (std::weak_ptr<IWebAudioPlayerClient> client,
                                                                  const std::string &audioMimeType,
                                                                  const uint32_t priority,
                                                                  const WebAudioConfig *config,
                                                                 const std::shared_ptr<ISharedMemoryBuffer> &shmBuffer),
                (const, override));
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_WEB_AUDIO_PLAYER_SERVER_INTERNAL_FACTORY_MOCK_H_
