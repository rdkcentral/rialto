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

#ifndef FIREBOLT_RIALTO_SERVER_IPC_VIDEO_FRAME_CAPTURE_MODULE_SERVICE_H_
#define FIREBOLT_RIALTO_SERVER_IPC_VIDEO_FRAME_CAPTURE_MODULE_SERVICE_H_

#include "IVideoFrameCaptureModuleService.h"

#include <map>
#include <memory>
#include <mutex>

namespace firebolt::rialto::server::ipc
{
class VideoFrameCaptureModuleServiceFactory : public IVideoFrameCaptureModuleServiceFactory
{
public:
    std::shared_ptr<IVideoFrameCaptureModuleService>
    create(service::IVideoFrameCaptureService &videoFrameCaptureService) const override;
};

class VideoFrameCaptureModuleService : public IVideoFrameCaptureModuleService
{
public:
    explicit VideoFrameCaptureModuleService(service::IVideoFrameCaptureService &videoFrameCaptureService);
    ~VideoFrameCaptureModuleService() override = default;

    void clientConnected(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient) override;
    void clientDisconnected(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient) override;
    std::optional<service::VideoFrameCaptureClientId>
    captureClientIdOf(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient) override;

    void createVideoFrameCapture(::google::protobuf::RpcController *controller,
                                 const ::firebolt::rialto::CreateVideoFrameCaptureRequest *request,
                                 ::firebolt::rialto::CreateVideoFrameCaptureResponse *response,
                                 ::google::protobuf::Closure *done) override;
    void destroyVideoFrameCapture(::google::protobuf::RpcController *controller,
                                  const ::firebolt::rialto::DestroyVideoFrameCaptureRequest *request,
                                  ::firebolt::rialto::DestroyVideoFrameCaptureResponse *response,
                                  ::google::protobuf::Closure *done) override;

    void getCapabilities(::google::protobuf::RpcController *controller,
                         const ::firebolt::rialto::GetVideoFrameCaptureCapabilitiesRequest *request,
                         ::firebolt::rialto::GetVideoFrameCaptureCapabilitiesResponse *response,
                         ::google::protobuf::Closure *done) override;
    void getPool(::google::protobuf::RpcController *controller,
                 const ::firebolt::rialto::GetVideoFrameCapturePoolRequest *request,
                 ::firebolt::rialto::GetVideoFrameCapturePoolResponse *response,
                 ::google::protobuf::Closure *done) override;

    void acquireCurrentFrame(::google::protobuf::RpcController *controller,
                             const ::firebolt::rialto::AcquireCurrentVideoFrameRequest *request,
                             ::firebolt::rialto::AcquireCurrentVideoFrameResponse *response,
                             ::google::protobuf::Closure *done) override;
    void releaseFrame(::google::protobuf::RpcController *controller,
                      const ::firebolt::rialto::ReleaseVideoFrameRequest *request,
                      ::firebolt::rialto::ReleaseVideoFrameResponse *response,
                      ::google::protobuf::Closure *done) override;

private:
    /** @retval false if the call did not come from a known client connection. */
    bool clientIdOf(::google::protobuf::RpcController *controller, service::VideoFrameCaptureClientId &clientId);

    service::IVideoFrameCaptureService &m_videoFrameCaptureService;
    std::mutex m_mutex;
    service::VideoFrameCaptureClientId m_nextClientId{1};
    std::map<std::shared_ptr<::firebolt::rialto::ipc::IClient>, service::VideoFrameCaptureClientId> m_clientIds;
};
} // namespace firebolt::rialto::server::ipc

#endif // FIREBOLT_RIALTO_SERVER_IPC_VIDEO_FRAME_CAPTURE_MODULE_SERVICE_H_
