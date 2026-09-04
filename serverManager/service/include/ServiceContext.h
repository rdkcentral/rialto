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

#ifndef RIALTO_SERVERMANAGER_SERVICE_SERVICE_CONTEXT_H_
#define RIALTO_SERVERMANAGER_SERVICE_SERVICE_CONTEXT_H_

#include "IController.h"
#include "IServiceContext.h"
#include "ISessionServerAppManager.h"
#include "IStateObserver.h"
#include "IYamlCapabilities.h"
#include <list>
#include <memory>
#include <string>

namespace rialto::servermanager::service
{
class ServiceContext : public IServiceContext
{
public:
    ServiceContext(const std::shared_ptr<IStateObserver> &stateObserver,
                   const std::list<std::string> &environmentVariables, const std::string &sessionServerPath,
                   std::chrono::milliseconds sessionServerStartupTimeout, std::chrono::seconds healthcheckInterval,
                   unsigned numOfFailedPingsBeforeRecovery, unsigned int socketPermissions,
                   const std::string &socketOwner, const std::string &socketGroup,
                   const std::shared_ptr<IYamlCapabilities> &mediaCapabilities = nullptr);
    virtual ~ServiceContext() = default;

    common::ISessionServerAppManager &getSessionServerAppManager() override;

private:
    // INITIALIZATION ORDER:
    // m_ipcController is declared BEFORE m_sessionServerAppManager to ensure it exists before
    // m_sessionServerAppManager is constructed. However, we initialize it in the constructor body
    // (not in initializer list) AFTER m_sessionServerAppManager is fully constructed.
    //
    // Why this order?
    // 1. Members are initialized in declaration order (C++ standard)
    // 2. m_ipcController must be declared first so it exists when m_sessionServerAppManager is constructed
    // 3. m_sessionServerAppManager constructor receives m_ipcController by reference
    // 4. After m_sessionServerAppManager is constructed, we populate m_ipcController in body
    // 5. m_sessionServerAppManager stores a reference that gets populated later
    std::unique_ptr<ipc::IController> m_ipcController;
    std::unique_ptr<common::ISessionServerAppManager> m_sessionServerAppManager;
};
} // namespace rialto::servermanager::service

#endif // RIALTO_SERVERMANAGER_SERVICE_SERVICE_CONTEXT_H_
