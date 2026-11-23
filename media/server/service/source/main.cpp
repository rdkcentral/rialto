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
#include <systemd/sd-journal.h>
#include <syslog.h>
#include <unistd.h>

#include "IApplicationSessionServer.h"
#include "IGstInitialiser.h"
#include "RialtoServerLogging.h"

// NOLINT(build/filename_format)

int main(int argc, char *argv[])
{
    const char kSrcRev[] = SRCREV;
    const char kTags[] = TAGS;

    const int outFd = sd_journal_stream_fd("RialtoServer", LOG_INFO, 0);
    if(outFd > 0)
    {
        if(dup2(outFd, STDOUT_FILENO) < 0)
        {
            RIALTO_SERVER_LOG_ERROR("Failed to redirect stdout: %s", std::strerror(errno));
        }
        close(outFd);
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create journal stdout stream: %s", std::strerror(-outFd));
    }

    const int errFd = sd_journal_stream_fd("RialtoServer", LOG_ERR, 0);
    if(errFd > 0)
    {
        if(dup2(errFd, STDERR_FILENO) < 0)
        {
            RIALTO_SERVER_LOG_ERROR("Failed to redirect stderr: %s", std::strerror(errno));
        }
        close(errFd);
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create journal stderr stream: %s", std::strerror(-errFd));
    }

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
    

    firebolt::rialto::server::IGstInitialiser::instance().initialise(&argc, &argv);

    auto appSessionServer =
        firebolt::rialto::server::IApplicationSessionServerFactory::getFactory()->createApplicationSessionServer();

    if (!appSessionServer->init(argc, argv))
    {
        return EXIT_FAILURE;
    }
    appSessionServer->startService();

#ifdef FREE_MEM_BEFORE_EXIT
    RIALTO_SERVER_LOG_INFO("Calling ShutdownProtobufLibrary");
    google::protobuf::ShutdownProtobufLibrary();
#endif

    return EXIT_SUCCESS;
}
