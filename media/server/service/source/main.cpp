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
#include "IGstPlayer.h"
#include "IMediaKeysCapabilities.h"
#include "IMediaKeysServerInternal.h"
#include "IMediaPipelineServerInternal.h"
#include "ISharedMemoryBuffer.h"
#include "IpcFactory.h"
#include "MainThread.h"
#include "PlaybackService.h"
#include "SessionServerManager.h"
#include <cstdlib>
#include <thread>

// NOLINT(build/filename_format)

int main(int argc, char *argv[])
{
    firebolt::rialto::server::IGstPlayer::initalise(argc, argv);

    firebolt::rialto::server::ipc::IpcFactory ipcFactory;
    firebolt::rialto::server::service::MainThread mainThread;
    firebolt::rialto::server::service::CdmService
        cdmService{mainThread, firebolt::rialto::server::IMediaKeysServerInternalFactory::createFactory(),
                   firebolt::rialto::IMediaKeysCapabilitiesFactory::createFactory()};
    firebolt::rialto::server::service::PlaybackService
        playbackService{mainThread, firebolt::rialto::server::IMediaPipelineServerInternalFactory::createFactory(),
                        firebolt::rialto::server::ISharedMemoryBufferFactory::createFactory(), cdmService};
    firebolt::rialto::server::service::SessionServerManager serviceManager{ipcFactory, playbackService, cdmService};
    if (!serviceManager.initialize(argc, argv))
    {
        return EXIT_FAILURE;
    }
    serviceManager.startService();
    return EXIT_SUCCESS;
}
