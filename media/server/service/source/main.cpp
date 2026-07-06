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

#include <cstring>
#include <google/protobuf/service.h>
#include <cstdio>

#include "IApplicationSessionServer.h"
#include "IGstInitialiser.h"
#include "RialtoServerLogging.h"
#include "RialtoTelemetry.h"

// NOLINT(build/filename_format)

int main(int argc, char *argv[])
{
    const char kSrcRev[] = SRCREV;
    const char kTags[] = TAGS;

    if (std::strlen(kSrcRev) > 0)
    {
        if (std::strlen(kTags) > 0)
        {
            RIALTO_SERVER_LOG_MIL("Release Tag(s): %s (Commit ID: %s)", kTags, kSrcRev);
        }
        else
        {
            RIALTO_SERVER_LOG_MIL("Release Tag(s): No Release Tags! (Commit ID: %s)", kSrcRev);
        }
    }
    else
    {
        RIALTO_SERVER_LOG_WARN("Failed to get git commit ID!");
    }

    printf("main.cpp: Telemetry init");

#ifdef RIALTO_TELEMETRY_SUPPORT
    RIALTO_SERVER_LOG_MIL("Rialto telemetry ENABLED");
#endif

    TELEMETRY_INIT("rialto-server");
    RIALTO_SERVER_LOG_MIL("Rialto telemetry intialized");

    char telemetryBuff[128] = {0};
    snprintf(telemetryBuff, sizeof(telemetryBuff), "Rialto telemetry initialized");
    TELEMETRY_EVENT_STRING("RialtoMain", telemetryBuff);

    firebolt::rialto::server::IGstInitialiser::instance().initialise(&argc, &argv);

    auto appSessionServer =
        firebolt::rialto::server::IApplicationSessionServerFactory::getFactory()->createApplicationSessionServer();

    if (!appSessionServer->init(argc, argv))
    {
        TELEMETRY_UNINIT();
        return EXIT_FAILURE;
    }
    appSessionServer->startService();

#ifdef FREE_MEM_BEFORE_EXIT
    RIALTO_SERVER_LOG_INFO("Calling ShutdownProtobufLibrary");
    google::protobuf::ShutdownProtobufLibrary();
#endif

    return EXIT_SUCCESS;
}
