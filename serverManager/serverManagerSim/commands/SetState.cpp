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

#include "SetState.h"
#include "../HttpRequest.h"
#include "../TestService.h"
#include <string>

namespace
{
firebolt::rialto::common::SessionServerState convert(const std::string &state)
{
    if ("Inactive" == state)
    {
        return firebolt::rialto::common::SessionServerState::INACTIVE;
    }
    else if ("Active" == state)
    {
        return firebolt::rialto::common::SessionServerState::ACTIVE;
    }
    else if ("NotRunning" == state)
    {
        return firebolt::rialto::common::SessionServerState::NOT_RUNNING;
    }
    return firebolt::rialto::common::SessionServerState::ERROR;
}
} // namespace

namespace rialto::servermanager
{
SetState::SetState(TestService &service, const HttpRequest &request) : m_service{service}, m_kRequest{request} {}

void SetState::run() const
{
    if (m_kRequest.getParams().size() != 2)
    {
        std::string reply{"HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\n"
                          "SetState command not valid!\r\n"
                          "Should be: /SetState/AppName/NewState\r\n"
                          "Available states: Inactive, Active, NotRunning, Error\r\n"
                          "Example command: /SetState/YouTube/Inactive\r\n"};
        m_kRequest.reply(reply);
        return;
    }
    std::string appName{m_kRequest.getParams()[0]};
    firebolt::rialto::common::SessionServerState state{convert(m_kRequest.getParams()[1])};
    firebolt::rialto::common::AppConfig appConfig{m_kRequest.getPostData()};
    std::string reply{"HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n"};
    if (m_service.setState(appName, state, appConfig))
    {
        reply += "SetState command succeeded!\r\n";
    }
    else
    {
        reply += "SetState command failed!\r\n";
    }
    m_kRequest.reply(reply);
}
} // namespace rialto::servermanager
