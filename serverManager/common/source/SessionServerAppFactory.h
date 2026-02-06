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

#ifndef RIALTO_SERVERMANAGER_COMMON_SESSION_SERVER_APP_FACTORY_H_
#define RIALTO_SERVERMANAGER_COMMON_SESSION_SERVER_APP_FACTORY_H_

#include "ILinuxWrapper.h"
#include "ISessionServerAppFactory.h"
#include <chrono>
#include <list>
#include <memory>
#include <string>

namespace rialto::servermanager::common
{
class SessionServerAppFactory : public ISessionServerAppFactory
{
public:
    explicit SessionServerAppFactory(const std::list<std::string> &environmentVariables,
                                     const std::string &sessionServerPath,
                                     std::chrono::milliseconds sessionServerStartupTimeout,
                                     unsigned int socketPermissions, const std::string &socketOwner,
                                     const std::string &socketGroup);
    ~SessionServerAppFactory() override = default;

    std::shared_ptr<ISessionServerApp>
    create(const std::string &appName, const firebolt::rialto::common::SessionServerState &initialState,
           const firebolt::rialto::common::AppConfig &appConfig, SessionServerAppManager &sessionServerAppManager,
           std::unique_ptr<firebolt::rialto::ipc::INamedSocket> &&namedSocket) const override;
    std::shared_ptr<ISessionServerApp>
    create(SessionServerAppManager &sessionServerAppManager,
           std::unique_ptr<firebolt::rialto::ipc::INamedSocket> &&namedSocket) const override;

private:
    const std::list<std::string> m_kEnvironmentVariables;
    const std::string m_kSessionServerPath;
    const std::chrono::milliseconds m_kSessionServerStartupTimeout;
    const unsigned int m_kSocketPermissions;
    const std::string m_kSocketOwner;
    const std::string m_kSocketGroup;
    std::shared_ptr<firebolt::rialto::wrappers::ILinuxWrapperFactory> m_linuxWrapperFactory;
};
} // namespace rialto::servermanager::common

#endif // RIALTO_SERVERMANAGER_COMMON_SESSION_SERVER_APP_FACTORY_H_
