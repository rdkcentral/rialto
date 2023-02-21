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

#ifndef RIALTO_SERVERMANAGER_TEST_SERVICE_H_
#define RIALTO_SERVERMANAGER_TEST_SERVICE_H_

#include "IServerManagerService.h"
#include "LoggingLevels.h"
#include "StateObserver.h"
#include <condition_variable>
#include <list>
#include <memory>
#include <mongoose.h>
#include <mutex>
#include <string>

namespace rialto::servermanager
{
class TestService
{
public:
    explicit TestService(const std::list<std::string> &environmentVariables);
    ~TestService();

    void run();
    void shutdown();

    bool setState(const std::string &appName, const firebolt::rialto::common::SessionServerState &state,
                  const firebolt::rialto::common::AppConfig &appConfig);
    firebolt::rialto::common::SessionServerState getState(const std::string &appName);
    std::string getAppInfo(const std::string &appName);
    bool setLogLevels(const service::LoggingLevels &logLevels);

private:
    std::shared_ptr<StateObserver> m_stateObserver;
    std::unique_ptr<service::IServerManagerService> m_serverManagerService;
    mg_context *m_mgServerCtx;
    bool m_isStopped;
    std::mutex m_serverMutex;
    std::condition_variable m_serverCv;
};
} // namespace rialto::servermanager

#endif // RIALTO_SERVERMANAGER_TEST_SERVICE_H_
