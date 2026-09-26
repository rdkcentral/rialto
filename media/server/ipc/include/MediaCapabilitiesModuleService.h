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

#ifndef FIREBOLT_RIALTO_SERVER_IPC_MEDIA_CAPABILITIES_MODULE_SERVICE_H_
#define FIREBOLT_RIALTO_SERVER_IPC_MEDIA_CAPABILITIES_MODULE_SERVICE_H_

#include "IMediaCapabilitiesModuleService.h"
#include "IMediaPipelineService.h"
#include <memory>

namespace firebolt::rialto::server::ipc
{
class MediaCapabilitiesModuleServiceFactory : public IMediaCapabilitiesModuleServiceFactory
{
public:
    MediaCapabilitiesModuleServiceFactory() = default;
    virtual ~MediaCapabilitiesModuleServiceFactory() = default;

    std::shared_ptr<IMediaCapabilitiesModuleService>
    create(service::IMediaPipelineService &mediaPipelineService) const override;
};

class MediaCapabilitiesModuleService : public IMediaCapabilitiesModuleService
{
public:
    explicit MediaCapabilitiesModuleService(service::IMediaPipelineService &mediaPipelineService);
    ~MediaCapabilitiesModuleService() override;

    void clientConnected(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient) override;
    void clientDisconnected(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient) override;

    void getSupportedAudioCapabilities(::google::protobuf::RpcController *controller,
                                       const ::firebolt::rialto::GetSupportedAudioCapabilitiesRequest *request,
                                       ::firebolt::rialto::AudioCapabilities *response,
                                       ::google::protobuf::Closure *done) override;
    void getSupportedVideoCapabilities(::google::protobuf::RpcController *controller,
                                       const ::firebolt::rialto::GetSupportedVideoCapabilitiesRequest *request,
                                       ::firebolt::rialto::VideoCapabilities *response,
                                       ::google::protobuf::Closure *done) override;

private:
    service::IMediaPipelineService &m_mediaPipelineService;
};
} // namespace firebolt::rialto::server::ipc

#endif // FIREBOLT_RIALTO_SERVER_IPC_MEDIA_CAPABILITIES_MODULE_SERVICE_H_
