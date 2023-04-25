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

#include "TestService.h"
#include <list>
#include <stdio.h>
#include <string>

namespace
{
std::string getSessionServerPath()
{
    const char *customPath = getenv("RIALTO_SESSION_SERVER_PATH");
    if (customPath)
    {
        fprintf(stderr, "Using custom SessionServer path: %s", customPath);
        return std::string(customPath);
    }
    return "/usr/bin/RialtoServer";
}

std::chrono::milliseconds getStartupTimeout()
{
    const char *customTimeout = getenv("RIALTO_SESSION_SERVER_STARTUP_TIMEOUT_MS");
    std::chrono::milliseconds timeout{0};
    if (customTimeout)
    {
        try
        {
            timeout = std::chrono::milliseconds{std::stoull(customTimeout)};
            fprintf(stderr, "Using custom SessionServer startup timeout: %sms", customTimeout);
        }
        catch (const std::exception &e)
        {
            fprintf(stderr, "Custom SessionServer startup timeout invalid, ignoring: %s", customTimeout);
        }
    }
    return timeout;
}

std::list<std::string> getEnvironmentVariables()
{
    std::list<std::string> environmentVariables;
    const char *sessionServerEnvVars = getenv("SESSION_SERVER_ENV_VARS");
    if (sessionServerEnvVars)
    {
        std::string envVarsStr{sessionServerEnvVars};
        size_t pos = 0;
        while ((pos = envVarsStr.find(";")) != std::string::npos)
        {
            environmentVariables.emplace_back(envVarsStr.substr(0, pos));
            envVarsStr.erase(0, pos + 1);
        }
        environmentVariables.emplace_back(envVarsStr);
    }
    return environmentVariables;
}

int getNumberOfPreloadedServers()
try
{
    const char *numOfPreloadedServersEnvVar = getenv("RIALTO_PRELOADED_SERVERS");
    if (numOfPreloadedServersEnvVar)
    {
        fprintf(stderr, "Number of preloaded servers: %s", numOfPreloadedServersEnvVar);
        return std::stoi(std::string(numOfPreloadedServersEnvVar));
    }
    return 0;
}
catch (std::exception &e)
{
    return 0;
}
} // namespace

int main(int argc, char *argv[])
{
    fprintf(stderr, "===========================================================================\n");
    fprintf(stderr, "==                     RIALTO SERVER MANAGER SIM                         ==\n");
    fprintf(stderr, "==                                                                       ==\n");
    fprintf(stderr, "== Test application is a Http Server running on localhost:9008           ==\n");
    fprintf(stderr, "==                                                                       ==\n");
    fprintf(stderr, "== To set state, send POST HttpRequest /SetState/AppName/NewState        ==\n");
    fprintf(stderr, "== Available states: Inactive, Active, NotRunning, Error                 ==\n");
    fprintf(stderr, "== For example:                                                          ==\n");
    fprintf(stderr, "== curl -X POST -d \"\" <BOX_IP>:9008/SetState/YouTube/NotRunning          ==\n");
    fprintf(stderr, "==                                                                       ==\n");
    fprintf(stderr, "== Custom socket name can be set in POST data. Available values are:     ==\n");
    fprintf(stderr, "==  - Empty string (socket name will be automatically generated)         ==\n");
    fprintf(stderr, "==  - Full socket path, e.g. POST -d \"/var/customsocket\"                 ==\n");
    fprintf(stderr, "==  - Socket name, e.g. POST -d \"sock\" will create /tmp/sock socket      ==\n");
    fprintf(stderr, "==                                                                       ==\n");
    fprintf(stderr, "== To get current state, send GET HttpRequest: /GetState/AppName         ==\n");
    fprintf(stderr, "== For example:                                                          ==\n");
    fprintf(stderr, "== curl -X GET <BOX_IP>:9008/GetState/YouTube                            ==\n");
    fprintf(stderr, "==                                                                       ==\n");
    fprintf(stderr, "== To get application info, send GET HttpRequest: /GetAppInfo/AppName    ==\n");
    fprintf(stderr, "== For example:                                                          ==\n");
    fprintf(stderr, "== curl -X GET <BOX_IP>:9008/GetAppInfo/YouTube                          ==\n");
    fprintf(stderr, "==                                                                       ==\n");
    fprintf(stderr, "== To set log levels, send POST HttpRequest: /SetLog/<component>/<level> ==\n");
    fprintf(stderr, "== For example:                                                          ==\n");
    fprintf(stderr, "== curl -X POST -d \"\" <BOX_IP>:9008/SetLog/client/error                  ==\n");
    fprintf(stderr, "==                                                                       ==\n");
    fprintf(stderr, "== To shutdown Test Service, send POST HttpRequest: /Quit                ==\n");
    fprintf(stderr, "== For example:                                                          ==\n");
    fprintf(stderr, "== curl -X POST -d \"\" <BOX_IP>:9008/Quit                                 ==\n");
    fprintf(stderr, "==                                                                       ==\n");
    fprintf(stderr, "===========================================================================\n");

    firebolt::rialto::common::ServerManagerConfig config{getEnvironmentVariables(), getNumberOfPreloadedServers(),
                                                         getSessionServerPath(), getStartupTimeout()};
    rialto::servermanager::TestService service{config};
    service.run();

    return 0;
}
