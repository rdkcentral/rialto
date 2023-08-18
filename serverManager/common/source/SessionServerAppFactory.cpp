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

#include "SessionServerAppFactory.h"
#include "SessionServerApp.h"
#include <memory>
#include <string>

namespace rialto::servermanager::common
{
SessionServerAppFactory::SessionServerAppFactory(const std::list<std::string> &environmentVariables,
                                                 const std::string &sessionServerPath,
                                                 std::chrono::milliseconds sessionServerStartupTimeout,
                                                 unsigned int socketPermissions)
    : m_kEnvironmentVariables{environmentVariables}, m_kSessionServerPath{sessionServerPath},
      m_kSessionServerStartupTimeout{sessionServerStartupTimeout}, m_kSocketPermissions{socketPermissions},
      m_linuxWrapperFactory{firebolt::rialto::common::ILinuxWrapperFactory::createFactory()}
{
}

std::unique_ptr<ISessionServerApp> SessionServerAppFactory::create(
    const std::string &appName, const firebolt::rialto::common::SessionServerState &initialState,
    const firebolt::rialto::common::AppConfig &appConfig, SessionServerAppManager &sessionServerAppManager) const
{
    return std::make_unique<SessionServerApp>(appName, initialState, appConfig,
                                              m_linuxWrapperFactory->createLinuxWrapper(), sessionServerAppManager,
                                              m_kEnvironmentVariables, m_kSessionServerPath,
                                              m_kSessionServerStartupTimeout, m_kSocketPermissions);
}

std::unique_ptr<ISessionServerApp> SessionServerAppFactory::create(SessionServerAppManager &sessionServerAppManager) const
{
    return std::make_unique<SessionServerApp>(m_linuxWrapperFactory->createLinuxWrapper(), sessionServerAppManager,
                                              m_kEnvironmentVariables, m_kSessionServerPath,
                                              m_kSessionServerStartupTimeout, m_kSocketPermissions);
}
} // namespace rialto::servermanager::common
