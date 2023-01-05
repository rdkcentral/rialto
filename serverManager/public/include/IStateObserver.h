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
 * @file IStateObserver.h
 *
 * This file comprises the class definition of IStateObserver.
 * An interface, which allows to subscribe for application state change notifications
 */

#ifndef RIALTO_SERVERMANAGER_SERVICE_I_STATE_OBSERVER_H_
#define RIALTO_SERVERMANAGER_SERVICE_I_STATE_OBSERVER_H_

#include "MediaServerCommon.h"
#include <string>

namespace rialto::servermanager::service
{
/**
 * @brief Represents an interface, which allows to subscribe for application state change notifications
 *
 * Implementation of this class should be done by a client. Pointer to the IStateObserver implementation
 * should be passed to ServerManagerService, when it is created.
 */
class IStateObserver
{
public:
    IStateObserver() = default;
    virtual ~IStateObserver() = default;

    IStateObserver(const IStateObserver &) = delete;
    IStateObserver &operator=(const IStateObserver &) = delete;
    IStateObserver(IStateObserver &&) = delete;
    IStateObserver &operator=(IStateObserver &&) = delete;

    /**
     * @brief Change state notification function
     *
     * This method should be implemented by a client. Implementation should be a client reaction state change of
     * RialtoSessionServer
     *
     * Function is called by RialtoServerManager, when it receives StateChangedEvent from RialtoSessionServer.
     *
     * @param[in]     appId     : The ID of an application, which changed its state
     * @param[in]     state     : New session server state
     *
     */
    virtual void stateChanged(const std::string &appId, const SessionServerState &state) = 0;
};
} // namespace rialto::servermanager::service

#endif // RIALTO_SERVERMANAGER_SERVICE_I_STATE_OBSERVER_H_
