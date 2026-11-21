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

#ifndef FIREBOLT_RIALTO_SERVER_IPC_I_VIDEO_FRAME_CAPTURE_MODULE_SERVICE_H_
#define FIREBOLT_RIALTO_SERVER_IPC_I_VIDEO_FRAME_CAPTURE_MODULE_SERVICE_H_

#include "IVideoFrameCaptureService.h"
#include "videoframecapturemodule.pb.h"
#include <IIpcServer.h>
#include <memory>

namespace firebolt::rialto::server::ipc
{
class IVideoFrameCaptureModuleService;

/**
 * @brief IVideoFrameCaptureModuleService factory class.
 */
class IVideoFrameCaptureModuleServiceFactory
{
public:
    IVideoFrameCaptureModuleServiceFactory() = default;
    virtual ~IVideoFrameCaptureModuleServiceFactory() = default;

    static std::shared_ptr<IVideoFrameCaptureModuleServiceFactory> createFactory();

    virtual std::shared_ptr<IVideoFrameCaptureModuleService>
    create(service::IVideoFrameCaptureService &videoFrameCaptureService) const = 0;
};

/**
 * @brief The VideoFrameCaptureModule IPC service: registers captures per client connection.
 */
class IVideoFrameCaptureModuleService : public ::firebolt::rialto::VideoFrameCaptureModule,
                                        public std::enable_shared_from_this<IVideoFrameCaptureModuleService>
{
public:
    IVideoFrameCaptureModuleService() = default;
    virtual ~IVideoFrameCaptureModuleService() = default;

    IVideoFrameCaptureModuleService(const IVideoFrameCaptureModuleService &) = delete;
    IVideoFrameCaptureModuleService(IVideoFrameCaptureModuleService &&) = delete;
    IVideoFrameCaptureModuleService &operator=(const IVideoFrameCaptureModuleService &) = delete;
    IVideoFrameCaptureModuleService &operator=(IVideoFrameCaptureModuleService &&) = delete;

    virtual void clientConnected(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient) = 0;
    virtual void clientDisconnected(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient) = 0;
};
} // namespace firebolt::rialto::server::ipc

#endif // FIREBOLT_RIALTO_SERVER_IPC_I_VIDEO_FRAME_CAPTURE_MODULE_SERVICE_H_
