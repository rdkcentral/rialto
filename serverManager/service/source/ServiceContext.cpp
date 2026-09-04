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
// ServiceContext initialization order:
// ====================================
// DECLARATION order (see ServiceContext.h):
// 1. m_ipcController - declared first (must exist before SessionServerAppManager constructor)
// 2. m_sessionServerAppManager - declared second
//
// INITIALIZATION:
// 1. In initializer list: m_sessionServerAppManager = createSessionServerAppManager(m_ipcController, ...)
//    → m_ipcController already exists as empty unique_ptr (not yet populated)
//    → SessionServerAppManager constructor receives and stores reference to it
// 2. In constructor body: m_ipcController = ipc::create(m_sessionServerAppManager)
//    → m_sessionServerAppManager is now fully constructed
//    → ipc::create populates the m_ipcController reference with actual controller
//    → SessionServerAppManager can now use the populated m_ipcController
//
// This properly handles the circular dependency between IPC controller and session server app manager
// without undefined behavior.

ServiceContext::ServiceContext(const std::shared_ptr<IStateObserver> &stateObserver,
                               const std::list<std::string> &environmentVariables, const std::string &sessionServerPath,
                               std::chrono::milliseconds sessionServerStartupTimeout,
                               std::chrono::seconds healthcheckInterval, unsigned numOfFailedPingsBeforeRecovery,
                               unsigned int socketPermissions, const std::string &socketOwner,
                               const std::string &socketGroup,
                               const std::shared_ptr<IYamlCapabilities> &mediaCapabilities)
    : m_ipcController{nullptr}, // Initialize as empty (will be populated in body)
      m_sessionServerAppManager{common::createSessionServerAppManager(m_ipcController, stateObserver,
                                                                      environmentVariables, sessionServerPath,
                                                                      sessionServerStartupTimeout, healthcheckInterval,
                                                                      numOfFailedPingsBeforeRecovery, socketPermissions,
                                                                      socketOwner, socketGroup, mediaCapabilities)}
{
    // Now that m_sessionServerAppManager is fully constructed, populate m_ipcController
    m_ipcController = ipc::create(m_sessionServerAppManager);
}

common::ISessionServerAppManager &ServiceContext::getSessionServerAppManager()
{
    return *m_sessionServerAppManager;
}
} // namespace rialto::servermanager::service
