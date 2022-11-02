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
 *
 * @file ServerManagerServiceFactory.h
 *
 * This file comprises the create() function definition
 * Function is used to create a new ServerManagerService instance
 */

#ifndef RIALTO_SERVERMANAGER_SERVICE_SERVER_MANAGER_SERVICE_FACTORY_H_
#define RIALTO_SERVERMANAGER_SERVICE_SERVER_MANAGER_SERVICE_FACTORY_H_

#include "IServerManagerService.h"
#include "IStateObserver.h"
#include <list>
#include <memory>
#include <string>

namespace rialto::servermanager::service
{
/**
 * @brief Create new ServerManagerService instance
 *
 * This function is used to create a new ServerManagerService instance.
 *
 * @param[in]    stateObserver        : A pointer to IStateObserver interface implementation
 * @param[in]    sessionServerEnvVars : List of environment variables, that need to be passed to RialtoSessionServer
 *
 * @retval a pointer to a new ServerManagerService instance.
 *
 */
std::unique_ptr<IServerManagerService> create(const std::shared_ptr<IStateObserver> &stateObserver,
                                              const std::list<std::string> &sessionServerEnvVars);
} // namespace rialto::servermanager::service

#endif // RIALTO_SERVERMANAGER_SERVICE_SERVER_MANAGER_SERVICE_FACTORY_H_
