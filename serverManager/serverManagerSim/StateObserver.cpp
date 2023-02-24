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

#include "StateObserver.h"
#include <algorithm>

namespace rialto::servermanager
{
void StateObserver::stateChanged(const std::string &appId, const SessionServerState &state)
{
    std::unique_lock<std::mutex> lock{m_sessionServerStateMutex};
    m_sessionServerStates[appId] = state;
}

SessionServerState StateObserver::getCurrentState(const std::string &appId) const
{
    std::unique_lock<std::mutex> lock{m_sessionServerStateMutex};
    auto state = m_sessionServerStates.find(appId);
    if (state != m_sessionServerStates.end())
    {
        return state->second;
    }
    return SessionServerState::NOT_RUNNING;
}

std::string StateObserver::getActiveApp() const
{
    std::unique_lock<std::mutex> lock{m_sessionServerStateMutex};
    auto app = std::find_if(m_sessionServerStates.begin(), m_sessionServerStates.end(),
                            [](const auto &app) { return app.second == SessionServerState::ACTIVE; });
    if (app != m_sessionServerStates.end())
    {
        return app->first;
    }
    return "";
}

} // namespace rialto::servermanager
