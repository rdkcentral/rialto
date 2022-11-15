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

#ifndef FIREBOLT_RIALTO_SERVER_IPC_I_MEDIA_KEYS_CAPABILITIES_MODULE_SERVICE_H_
#define FIREBOLT_RIALTO_SERVER_IPC_I_MEDIA_KEYS_CAPABILITIES_MODULE_SERVICE_H_

#include "ICdmService.h"
#include "mediakeyscapabilitiesmodule.pb.h"
#include <IIpcServer.h>
#include <memory>

namespace firebolt::rialto::server::ipc
{
class IMediaKeysCapabilitiesModuleService;

/**
 * @brief IMediaKeysCapabilitiesModuleService factory class, returns a concrete implementation of
 * IMediaKeysCapabilitiesModuleService
 */
class IMediaKeysCapabilitiesModuleServiceFactory
{
public:
    IMediaKeysCapabilitiesModuleServiceFactory() = default;
    virtual ~IMediaKeysCapabilitiesModuleServiceFactory() = default;

    /**
     * @brief Create a IMediaKeysCapabilitiesModuleServiceFactory instance.
     *
     * @retval the factory instance or null on error.
     */
    static std::shared_ptr<IMediaKeysCapabilitiesModuleServiceFactory> createFactory();

    /**
     * @brief Creates a MediaKeysCapabilitiesModuleService object.
     *
     * @param[in] cdmService : The service for cdm objects.
     *
     * @retval the rialto controller ipc instance or null on error.
     */
    virtual std::shared_ptr<IMediaKeysCapabilitiesModuleService> create(service::ICdmService &cdmService) const = 0;
};

/**
 * @brief The definition of the IMediaKeysCapabilitiesModuleService interface.
 */
class IMediaKeysCapabilitiesModuleService : public ::firebolt::rialto::MediaKeysCapabilitiesModule,
                                            public std::enable_shared_from_this<IMediaKeysCapabilitiesModuleService>
{
public:
    IMediaKeysCapabilitiesModuleService() = default;
    virtual ~IMediaKeysCapabilitiesModuleService() = default;

    IMediaKeysCapabilitiesModuleService(const IMediaKeysCapabilitiesModuleService &) = delete;
    IMediaKeysCapabilitiesModuleService(IMediaKeysCapabilitiesModuleService &&) = delete;
    IMediaKeysCapabilitiesModuleService &operator=(const IMediaKeysCapabilitiesModuleService &) = delete;
    IMediaKeysCapabilitiesModuleService &operator=(IMediaKeysCapabilitiesModuleService &&) = delete;

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

#endif // FIREBOLT_RIALTO_SERVER_IPC_I_MEDIA_KEYS_CAPABILITIES_MODULE_SERVICE_H_
