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

#include "ServiceContext.h"
#include "ControllerFactory.h"
#include "SessionServerAppManagerFactory.h"

namespace rialto::servermanager::service
{
ServiceContext::ServiceContext(const std::shared_ptr<IStateObserver> &stateObserver,
                               const std::list<std::string> &environmentVariables, const std::string &sessionServerPath,
                               std::chrono::milliseconds sessionServerStartupTimeout,
                               std::chrono::seconds healthcheckInterval, unsigned numOfFailedPingsBeforeRecovery,
                               unsigned int socketPermissions)
    : m_sessionServerAppManager{common::createSessionServerAppManager(m_ipcController, stateObserver,
                                                                      environmentVariables, sessionServerPath,
                                                                      sessionServerStartupTimeout, healthcheckInterval,
                                                                      numOfFailedPingsBeforeRecovery, socketPermissions)},
      m_ipcController{ipc::create(m_sessionServerAppManager)}
{
}

common::ISessionServerAppManager &ServiceContext::getSessionServerAppManager()
{
    return *m_sessionServerAppManager;
}
} // namespace rialto::servermanager::service
