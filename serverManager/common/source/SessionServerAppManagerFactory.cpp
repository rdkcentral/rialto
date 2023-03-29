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

#include "SessionServerAppManagerFactory.h"
#include "IEventThread.h"
#include "RialtoServerManagerLogging.h"
#include "SessionServerAppFactory.h"
#include "SessionServerAppManager.h"
#include <memory>
#include <string>

namespace
{
int getNumberOfPreloadedServers()
try
{
    const char *numOfPreloadedServersEnvVar = getenv("RIALTO_PRELOADED_SERVERS");
    if (numOfPreloadedServersEnvVar)
    {
        RIALTO_SERVER_MANAGER_LOG_INFO("Number of preloaded servers: %s", numOfPreloadedServersEnvVar);
        return std::stoi(std::string(numOfPreloadedServersEnvVar));
    }
    return 0;
}
catch (std::exception &e)
{
    return 0;
}
} // namespace

namespace rialto::servermanager::common
{
std::unique_ptr<ISessionServerAppManager>
createSessionServerAppManager(std::unique_ptr<ipc::IController> &ipc,
                              const std::shared_ptr<service::IStateObserver> &stateObserver,
                              const std::list<std::string> &environmentVariables)
{
    return std::make_unique<SessionServerAppManager>(ipc, stateObserver,
                                                     std::make_unique<SessionServerAppFactory>(environmentVariables),
                                                     firebolt::rialto::common::IEventThreadFactory::createFactory(),
                                                     getNumberOfPreloadedServers());
}
} // namespace rialto::servermanager::common
