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

#ifndef FIREBOLT_RIALTO_SERVER_APPLICATION_SESSION_SERVER_H_
#define FIREBOLT_RIALTO_SERVER_APPLICATION_SESSION_SERVER_H_

#include "CdmService.h"
#include "ControlService.h"
#include "IApplicationSessionServer.h"
#include "IControlServerInternal.h"
#include "IHeartbeatProcedure.h"
#include "IMediaKeysCapabilities.h"
#include "IMediaKeysServerInternal.h"
#include "IMediaPipelineCapabilities.h"
#include "IMediaPipelineServerInternal.h"
#include "ISharedMemoryBuffer.h"
#include "IWebAudioPlayerServerInternal.h"
#include "IpcFactory.h"
#include "PlaybackService.h"
#include "SessionServerManager.h"
// Orchestrator interfaces and implementations
#include "IGstCapabilities.h"
#include "IMediaCapabilities.h"
#include "MediaCapabilities.h"
#include <memory>

namespace firebolt::rialto::server
{
class ApplicationSessionServerFactory : public IApplicationSessionServerFactory
{
public:
    ApplicationSessionServerFactory() = default;
    ~ApplicationSessionServerFactory() override = default;

    std::unique_ptr<IApplicationSessionServer> createApplicationSessionServer() const override;
};

class ApplicationSessionServer : public IApplicationSessionServer
{
public:
    ApplicationSessionServer();
    ~ApplicationSessionServer() override = default;

    bool init(int argc, char *argv[]) override;
    void startService() override;

private:
    // CRITICAL: Initialize orchestrator BEFORE services that depend on it
    // C++ member initialization follows declaration order, not initializer list order

    firebolt::rialto::server::ipc::IpcFactory m_ipcFactory;

    // Create GstCapabilities for fallback (safely handles null factory)
    std::shared_ptr<IGstCapabilities> m_gstCapabilities;

    // Create orchestrator (Path 0: preloaded from ServerManager, Path B: GStreamer fallback)
    // CRITICAL: Must be initialized in constructor initializer list BEFORE m_playbackService
    // uses it (in-class initializer runs in declaration order, not initializer list order)
    // Explicit nullptr initializer ensures clear initialization semantics
    std::shared_ptr<firebolt::rialto::IMediaCapabilities> m_mediaCapabilities = nullptr;

    firebolt::rialto::server::service::ControlService m_controlService{
        firebolt::rialto::server::IControlServerInternalFactory::createFactory()};
    firebolt::rialto::server::service::CdmService
        m_cdmService{firebolt::rialto::server::IMediaKeysServerInternalFactory::createFactory(),
                     firebolt::rialto::IMediaKeysCapabilitiesFactory::createFactory()};
    // PlaybackService must receive non-nullptr m_mediaCapabilities
    // Order: m_gstCapabilities → m_mediaCapabilities → m_playbackService (via in-class initializer)
    firebolt::rialto::server::service::PlaybackService
        m_playbackService{firebolt::rialto::server::IMediaPipelineServerInternalFactory::createFactory(),
                          firebolt::rialto::IMediaPipelineCapabilitiesFactory::createFactory(),
                          firebolt::rialto::server::IWebAudioPlayerServerInternalFactory::createFactory(),
                          firebolt::rialto::server::ISharedMemoryBufferFactory::createFactory(),
                          m_cdmService,
                          m_mediaCapabilities};
    firebolt::rialto::server::service::SessionServerManager
        m_serviceManager{m_ipcFactory, m_playbackService, m_cdmService, m_controlService,
                         firebolt::rialto::server::IHeartbeatProcedureFactory::createFactory()};
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_APPLICATION_SESSION_SERVER_H_
