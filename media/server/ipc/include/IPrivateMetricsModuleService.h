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

#ifndef FIREBOLT_RIALTO_SERVER_IPC_I_PRIVATE_METRICS_MODULE_SERVICE_H_
#define FIREBOLT_RIALTO_SERVER_IPC_I_PRIVATE_METRICS_MODULE_SERVICE_H_

#include "ControlCommon.h"
#include "MediaCommon.h"
#include "privatemetricsmodule.pb.h"
#include <IIpcServer.h>
#include <memory>

namespace firebolt::rialto::server::ipc
{
class IPrivateMetricsModuleService;

class IPrivateMetricsModuleServiceFactory
{
public:
    IPrivateMetricsModuleServiceFactory() = default;
    virtual ~IPrivateMetricsModuleServiceFactory() = default;

    static std::shared_ptr<IPrivateMetricsModuleServiceFactory> createFactory();

    virtual std::shared_ptr<IPrivateMetricsModuleService> create() const = 0;
};

class IPrivateMetricsModuleService : public ::firebolt::rialto::PrivateMetricsModule,
                                     public std::enable_shared_from_this<IPrivateMetricsModuleService>
{
public:
    IPrivateMetricsModuleService() = default;
    virtual ~IPrivateMetricsModuleService() = default;

    IPrivateMetricsModuleService(const IPrivateMetricsModuleService &) = delete;
    IPrivateMetricsModuleService(IPrivateMetricsModuleService &&) = delete;
    IPrivateMetricsModuleService &operator=(const IPrivateMetricsModuleService &) = delete;
    IPrivateMetricsModuleService &operator=(IPrivateMetricsModuleService &&) = delete;

    virtual void clientConnected(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient) = 0;
    virtual void clientDisconnected(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient) = 0;

    virtual void notifyPlaybackStateChanged(int sessionId, PlaybackState oldState, PlaybackState newState) = 0;
    virtual void notifyApplicationStateChanged(ApplicationState oldState, ApplicationState newState) = 0;
};
} // namespace firebolt::rialto::server::ipc

#endif // FIREBOLT_RIALTO_SERVER_IPC_I_PRIVATE_METRICS_MODULE_SERVICE_H_
