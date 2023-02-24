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

#include "GetState.h"
#include "../HttpRequest.h"
#include "../TestService.h"
#include <string>

namespace
{
std::string toString(const rialto::servermanager::service::SessionServerState &state)
{
    switch (state)
    {
    case rialto::servermanager::service::SessionServerState::UNINITIALIZED:
    {
        return "Uninitialized";
    }
    case rialto::servermanager::service::SessionServerState::INACTIVE:
    {
        return "Inactive";
    }
    case rialto::servermanager::service::SessionServerState::ACTIVE:
    {
        return "Active";
    }
    case rialto::servermanager::service::SessionServerState::NOT_RUNNING:
    {
        return "NotRunning";
    }
    case rialto::servermanager::service::SessionServerState::ERROR:
    {
        return "Error";
    }
    }
    return "Unknown";
}
} // namespace

namespace rialto::servermanager
{
GetState::GetState(TestService &service, const HttpRequest &request) : m_service{service}, m_kRequest{request} {}

void GetState::run() const
{
    if (m_kRequest.getParams().size() != 1)
    {
        std::string reply{"HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\n"
                          "GetState command not valid!\r\n"
                          "Should be: /GetState/AppName\r\n"
                          "Example command: /GetState/YouTube\r\n"};
        m_kRequest.reply(reply);
        return;
    }
    std::string appName{m_kRequest.getParams()[0]};
    std::string reply{"HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n"
                      "GetState for: " +
                      appName + " returned: " + toString(m_service.getState(appName)) + "\r\n"};
    m_kRequest.reply(reply);
}
} // namespace rialto::servermanager
