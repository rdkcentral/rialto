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
#include "HealthcheckServiceFactory.h"
#include "IEventThread.h"
#include "INamedSocket.h"
#include "SessionServerAppFactory.h"
#include "SessionServerAppManager.h"
#include <memory>

namespace rialto::servermanager::common
{
std::unique_ptr<ISessionServerAppManager> createSessionServerAppManager(
    std::unique_ptr<ipc::IController> &ipc, const std::shared_ptr<service::IStateObserver> &stateObserver,
    const std::list<std::string> &environmentVariables, const std::string &sessionServerPath,
    std::chrono::milliseconds sessionServerStartupTimeout, std::chrono::seconds healthcheckInterval,
    std::chrono::seconds subtitleResyncInterval, unsigned numOfFailedPingsBeforeRecovery,
    unsigned int socketPermissions, const std::string &socketOwner, const std::string &socketGroup)
{
    return std::make_unique<
        SessionServerAppManager>(ipc, stateObserver,
                                 std::make_unique<SessionServerAppFactory>(environmentVariables, sessionServerPath,
                                                                           sessionServerStartupTimeout,
                                                                           socketPermissions, socketOwner, socketGroup, subtitleResyncInterval),
                                 std::make_unique<HealthcheckServiceFactory>(healthcheckInterval,
                                                                             numOfFailedPingsBeforeRecovery),
                                 firebolt::rialto::common::IEventThreadFactory::createFactory(),
                                 firebolt::rialto::ipc::INamedSocketFactory::getFactory());
}
} // namespace rialto::servermanager::common
