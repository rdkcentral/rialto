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

#ifndef RIALTO_SERVERMANAGER_STATE_OBSERVER_H_
#define RIALTO_SERVERMANAGER_STATE_OBSERVER_H_

#include "IStateObserver.h"
#include <map>
#include <mutex>
#include <string>

namespace rialto::servermanager
{
class StateObserver : public service::IStateObserver
{
public:
    StateObserver() = default;
    virtual ~StateObserver() = default;
    StateObserver(const StateObserver &) = delete;
    StateObserver(StateObserver &&) = delete;
    StateObserver &operator=(const StateObserver &) = delete;
    StateObserver &operator=(StateObserver &&) = delete;

    void stateChanged(const std::string &appId, const SessionServerState &state) override;
    SessionServerState getCurrentState(const std::string &appId) const;
    std::string getActiveApp() const;

private:
    mutable std::mutex m_sessionServerStateMutex;
    std::map<std::string, SessionServerState> m_sessionServerStates;
};
} // namespace rialto::servermanager

#endif // RIALTO_SERVERMANAGER_STATE_OBSERVER_H_
