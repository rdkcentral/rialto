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

#ifndef FIREBOLT_RIALTO_SERVER_IPC_I_MEDIA_CAPABILITIES_MODULE_SERVICE_H_
#define FIREBOLT_RIALTO_SERVER_IPC_I_MEDIA_CAPABILITIES_MODULE_SERVICE_H_

#include "IMediaPipelineService.h"
#include "mediacapabilitiesmodule.pb.h"
#include <IIpcServer.h>
#include <memory>

namespace firebolt::rialto::server::ipc
{
class IMediaCapabilitiesModuleService;

/**
 * @brief IMediaCapabilitiesModuleService factory class, returns a concrete implementation of
 * IMediaCapabilitiesModuleService
 */
class IMediaCapabilitiesModuleServiceFactory
{
public:
    IMediaCapabilitiesModuleServiceFactory() = default;
    virtual ~IMediaCapabilitiesModuleServiceFactory() = default;

    /**
     * @brief Create a IMediaCapabilitiesModuleServiceFactory instance.
     *
     * @retval the factory instance or null on error.
     */
    static std::shared_ptr<IMediaCapabilitiesModuleServiceFactory> createFactory();

    /**
     * @brief Creates a MediaCapabilitiesModuleService object.
     *
     * @param[in] mediaPipelineService : The service for media pipeline objects
     */
    virtual std::shared_ptr<IMediaCapabilitiesModuleService>
    create(service::IMediaPipelineService &mediaPipelineService) const = 0;
};

/**
 * @brief The definition of the IMediaCapabilitiesModuleService interface.
 */
class IMediaCapabilitiesModuleService : public ::firebolt::rialto::MediaCapabilitiesModule,
                                        public std::enable_shared_from_this<IMediaCapabilitiesModuleService>
{
public:
    IMediaCapabilitiesModuleService() = default;
    virtual ~IMediaCapabilitiesModuleService() = default;

    IMediaCapabilitiesModuleService(const IMediaCapabilitiesModuleService &) = delete;
    IMediaCapabilitiesModuleService(IMediaCapabilitiesModuleService &&) = delete;
    IMediaCapabilitiesModuleService &operator=(const IMediaCapabilitiesModuleService &) = delete;
    IMediaCapabilitiesModuleService &operator=(IMediaCapabilitiesModuleService &&) = delete;

    /**
     * @brief Connect to the ipc client.
     *
     * @param[in] ipcClient : The ipc client to connect to.
     */
    virtual void clientConnected(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient) = 0;

    /**
     * @brief Disconnect from the ipc client.
     *
     * @param[in] ipcClient : The ipc client to disconnect to.
     */
    virtual void clientDisconnected(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient) = 0;
};

} // namespace firebolt::rialto::server::ipc

#endif // FIREBOLT_RIALTO_SERVER_IPC_I_MEDIA_CAPABILITIES_MODULE_SERVICE_H_
