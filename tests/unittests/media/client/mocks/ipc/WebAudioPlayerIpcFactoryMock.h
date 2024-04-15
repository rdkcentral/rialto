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

#ifndef FIREBOLT_RIALTO_CLIENT_WEB_AUDIO_PLAYER_IPC_FACTORY_MOCK_H_
#define FIREBOLT_RIALTO_CLIENT_WEB_AUDIO_PLAYER_IPC_FACTORY_MOCK_H_

#include "IWebAudioPlayerIpc.h"
#include <gmock/gmock.h>
#include <memory>
#include <string>

namespace firebolt::rialto::client
{
class WebAudioPlayerIpcFactoryMock : public IWebAudioPlayerIpcFactory
{
public:
    WebAudioPlayerIpcFactoryMock() = default;
    virtual ~WebAudioPlayerIpcFactoryMock() = default;

    MOCK_METHOD(std::unique_ptr<IWebAudioPlayerIpc>, createWebAudioPlayerIpc,
                (IWebAudioPlayerIpcClient * client, const std::string &audioMimeType, const uint32_t priority,
                 std::weak_ptr<const WebAudioConfig> config, std::weak_ptr<IIpcClient> ipcClient),
                (override));
};
} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_WEB_AUDIO_PLAYER_IPC_FACTORY_MOCK_H_
