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

#include "SetLog.h"
#include "../HttpRequest.h"
#include "../TestService.h"
#include <string>

namespace
{
rialto::servermanager::service::LoggingLevel convert(const std::string &level)
{
    if ("fatal" == level)
    {
        return rialto::servermanager::service::LoggingLevel::FATAL;
    }
    else if ("error" == level)
    {
        return rialto::servermanager::service::LoggingLevel::ERROR;
    }
    else if ("warning" == level)
    {
        return rialto::servermanager::service::LoggingLevel::WARNING;
    }
    else if ("milestone" == level)
    {
        return rialto::servermanager::service::LoggingLevel::MILESTONE;
    }
    else if ("info" == level)
    {
        return rialto::servermanager::service::LoggingLevel::INFO;
    }
    else if ("debug" == level)
    {
        return rialto::servermanager::service::LoggingLevel::DEBUG;
    }
    return rialto::servermanager::service::LoggingLevel::UNCHANGED;
}

rialto::servermanager::service::LoggingLevels convert(const std::string &component, const std::string &level)
{
    rialto::servermanager::service::LoggingLevels levels;
    auto loggingLevel{convert(level)};
    if ("all" == component)
    {
        levels.defaultLoggingLevel = loggingLevel;
        levels.clientLoggingLevel = loggingLevel;
        levels.sessionServerLoggingLevel = loggingLevel;
        levels.ipcLoggingLevel = loggingLevel;
        levels.serverManagerLoggingLevel = loggingLevel;
        levels.commonLoggingLevel = loggingLevel;
    }
    else if ("default" == component)
    {
        levels.defaultLoggingLevel = loggingLevel;
    }
    else if ("client" == component)
    {
        levels.clientLoggingLevel = loggingLevel;
    }
    else if ("sessionServer" == component)
    {
        levels.sessionServerLoggingLevel = loggingLevel;
    }
    else if ("ipc" == component)
    {
        levels.ipcLoggingLevel = loggingLevel;
    }
    else if ("serverManager" == component)
    {
        levels.serverManagerLoggingLevel = loggingLevel;
    }
    else if ("common" == component)
    {
        levels.commonLoggingLevel = loggingLevel;
    }
    return levels;
}
} // namespace

namespace rialto::servermanager
{
SetLog::SetLog(TestService &service, const HttpRequest &request) : m_service{service}, m_kRequest{request} {}

void SetLog::run() const
{
    if (m_kRequest.getParams().size() != 2)
    {
        std::string reply{"HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\n"
                          "SetLog command not valid!\r\n"
                          "Should be: /SetLog/<component>/<level>\r\n"
                          "Available components: all, client, sessionServer, ipc, serverManager\r\n"
                          "Available levels: fatal, error, warning, milestone, info, debug\r\n"
                          "Example command: /SetLog/ipc/debug\r\n"};
        m_kRequest.reply(reply);
        return;
    }
    std::string component{m_kRequest.getParams()[0]};
    std::string level{m_kRequest.getParams()[1]};
    std::string reply{"HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n"};
    if (m_service.setLogLevels(convert(component, level)))
    {
        reply += "SetLog command succeeded!\r\n";
    }
    else
    {
        reply += "SetLog command failed!\r\n";
    }
    m_kRequest.reply(reply);
}
} // namespace rialto::servermanager
