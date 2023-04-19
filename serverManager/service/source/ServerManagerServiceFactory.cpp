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

#include "ServerManagerServiceFactory.h"
#include "RialtoServerManagerLogging.h"
#include "ServerManagerService.h"
#include "ServiceContext.h"
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

namespace rialto::servermanager::service
{
std::unique_ptr<IServerManagerService> create(const std::shared_ptr<IStateObserver> &stateObserver,
                                              const std::list<std::string> &sessionServerEnvVars)
{
    return std::make_unique<ServerManagerService>(std::make_unique<ServiceContext>(stateObserver, sessionServerEnvVars),
                                                  getNumberOfPreloadedServers());
}
} // namespace rialto::servermanager::service
