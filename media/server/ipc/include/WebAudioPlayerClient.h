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

#ifndef FIREBOLT_RIALTO_SERVER_IPC_WEB_AUDIO_PLAYER_CLIENT_H_
#define FIREBOLT_RIALTO_SERVER_IPC_WEB_AUDIO_PLAYER_CLIENT_H_

#include "IIpcServer.h"
#include "IWebAudioPlayerClient.h"
#include <memory>

namespace firebolt::rialto::server::ipc
{
class WebAudioPlayerClient : public IWebAudioPlayerClient
{
public:
    WebAudioPlayerClient(int webAudioPlayerHandle, const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient);
    ~WebAudioPlayerClient() override;

    void notifyState(WebAudioPlayerState state) override;

private:
    int m_webAudioPlayerHandle;
    std::shared_ptr<::firebolt::rialto::ipc::IClient> m_ipcClient;
};
} // namespace firebolt::rialto::server::ipc

#endif // FIREBOLT_RIALTO_SERVER_IPC_WEB_AUDIO_PLAYER_CLIENT_H_
