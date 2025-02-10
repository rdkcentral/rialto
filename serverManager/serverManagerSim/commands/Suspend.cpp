/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 Sky UK
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

#include "Suspend.h"
#include "../HttpRequest.h"
#include "../TestService.h"
#include <string>

namespace rialto::servermanager
{
Suspend::Suspend(TestService &service, const HttpRequest &request) : m_service{service}, m_kRequest{request} {}

void Suspend::run() const
{
    if (m_kRequest.getParams().size() != 1)
    {
        std::string reply{"HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\n"
                          "Suspend command not valid!\r\n"
                          "Should be: /Suspend/AppName\r\n"
                          "Example command: /Suspend/YouTube\r\n"};
        m_kRequest.reply(reply);
        return;
    }
    std::string appName{m_kRequest.getParams()[0]};
    std::string reply{"HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n"};
    if (m_service.suspendSessionServer(appName))
    {
        reply += "Suspend command succeeded!\r\n";
    }
    else
    {
        reply += "Suspend command failed!\r\n";
    }
    m_kRequest.reply(reply);
}
} // namespace rialto::servermanager
