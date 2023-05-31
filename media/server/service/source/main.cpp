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

#include "CdmService.h"
#include "ControlService.h"
#include "GstInit.h"
#include "IControlServerInternal.h"
#include "IHeartbeatProcedure.h"
#include "IMediaKeysCapabilities.h"
#include "IMediaKeysServerInternal.h"
#include "IMediaPipelineCapabilities.h"
#include "IMediaPipelineServerInternal.h"
#include "ISharedMemoryBuffer.h"
#include "IWebAudioPlayerServerInternalFactory.h"
#include "IpcFactory.h"
#include "PlaybackService.h"
#include "SessionServerManager.h"
#include <cstdlib>
#include <thread>

// NOLINT(build/filename_format)

int main(int argc, char *argv[])
{
    firebolt::rialto::server::gstInitalise(argc, argv);

    firebolt::rialto::server::ipc::IpcFactory ipcFactory;
    firebolt::rialto::server::service::ControlService
        controlService{firebolt::rialto::server::IControlServerInternalFactory::createFactory(),
                       firebolt::rialto::server::IHeartbeatProcedureFactory::createFactory()};
    firebolt::rialto::server::service::CdmService
        cdmService{firebolt::rialto::server::IMediaKeysServerInternalFactory::createFactory(),
                   firebolt::rialto::IMediaKeysCapabilitiesFactory::createFactory()};
    firebolt::rialto::server::service::PlaybackService
        playbackService{firebolt::rialto::server::IMediaPipelineServerInternalFactory::createFactory(),
                        firebolt::rialto::IMediaPipelineCapabilitiesFactory::createFactory(),
                        firebolt::rialto::server::IWebAudioPlayerServerInternalFactory::createFactory(),
                        firebolt::rialto::server::ISharedMemoryBufferFactory::createFactory(), cdmService};
    firebolt::rialto::server::service::SessionServerManager serviceManager{ipcFactory, playbackService, cdmService,
                                                                           controlService};
    if (!serviceManager.initialize(argc, argv))
    {
        return EXIT_FAILURE;
    }
    serviceManager.startService();
    return EXIT_SUCCESS;
}
