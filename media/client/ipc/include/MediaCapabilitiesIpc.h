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

#ifndef FIREBOLT_RIALTO_CLIENT_MEDIA_CAPABILITIES_IPC_H_
#define FIREBOLT_RIALTO_CLIENT_MEDIA_CAPABILITIES_IPC_H_

#include "IMediaCapabilities.h"
#include "IMediaCapabilitiesIpcFactory.h"
#include "IpcModule.h"
#include "mediapipelinecapabilitiesmodule.pb.h"
#include <memory>

namespace firebolt::rialto::client
{
/**
 * @brief IMediaCapabilitiesIpcFactory factory class definition.
 */
class MediaCapabilitiesIpcFactory : public IMediaCapabilitiesIpcFactory
{
public:
    MediaCapabilitiesIpcFactory() = default;
    ~MediaCapabilitiesIpcFactory() override = default;

    std::unique_ptr<IMediaCapabilities> createMediaCapabilitiesIpc() const override;
};

class MediaCapabilitiesIpc : public IMediaCapabilities, public IpcModule
{
public:
    explicit MediaCapabilitiesIpc(IIpcClient &ipcClient);
    ~MediaCapabilitiesIpc() override;

    common::AudioDecoderCapabilities getSupportedAudioCapabilities() override;
    common::VideoDecoderCapabilities getSupportedVideoCapabilities() override;

protected:
    bool createRpcStubs(const std::shared_ptr<ipc::IChannel> &ipcChannel) override;
    bool subscribeToEvents(const std::shared_ptr<ipc::IChannel> &ipcChannel) override { return true; }

private:
    std::unique_ptr<::firebolt::rialto::MediaPipelineCapabilitiesModule_Stub> m_stub;
};

} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_MEDIA_CAPABILITIES_IPC_H_
