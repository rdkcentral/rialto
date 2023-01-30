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

#ifndef RIALTO_SERVERMANAGER_SERVICE_SERVER_MANAGER_SERVICE_H_
#define RIALTO_SERVERMANAGER_SERVICE_SERVER_MANAGER_SERVICE_H_

#include "IServerManagerService.h"
#include "IServiceContext.h"
#include <memory>
#include <string>

namespace rialto::servermanager::service
{
class ServerManagerService : public IServerManagerService
{
public:
    explicit ServerManagerService(std::unique_ptr<IServiceContext> &&context);
    virtual ~ServerManagerService();
    ServerManagerService(const ServerManagerService &) = delete;
    ServerManagerService(ServerManagerService &&) = delete;
    ServerManagerService &operator=(const ServerManagerService &) = delete;
    ServerManagerService &operator=(ServerManagerService &&) = delete;

    bool changeSessionServerState(const std::string &appId, const firebolt::rialto::common::SessionServerState &state) override;
    std::string getAppConnectionInfo(const std::string &appId) const override;
    bool setLogLevels(const LoggingLevels &logLevels) const override;

private:
    const std::unique_ptr<IServiceContext> m_kContext;
};
} // namespace rialto::servermanager::service

#endif // RIALTO_SERVERMANAGER_SERVICE_SERVER_MANAGER_SERVICE_H_
