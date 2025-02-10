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

#include "CommandFactory.h"
#include "../HttpRequest.h"
#include "GetAppInfo.h"
#include "GetState.h"
#include "Quit.h"
#include "SetLog.h"
#include "SetState.h"
#include "Suspend.h"
#include "Unknown.h"

namespace rialto::servermanager
{
std::unique_ptr<Command> createCommand(TestService &service, const HttpRequest &request)
{
    if ("POST" == request.getMethod() && "SetState" == request.getCommand())
    {
        return std::make_unique<SetState>(service, request);
    }
    else if ("POST" == request.getMethod() && "Suspend" == request.getCommand())
    {
        return std::make_unique<Suspend>(service, request);
    }
    else if ("GET" == request.getMethod() && "GetState" == request.getCommand())
    {
        return std::make_unique<GetState>(service, request);
    }
    else if ("GET" == request.getMethod() && "GetAppInfo" == request.getCommand())
    {
        return std::make_unique<GetAppInfo>(service, request);
    }
    else if ("POST" == request.getMethod() && "SetLog" == request.getCommand())
    {
        return std::make_unique<SetLog>(service, request);
    }
    else if ("POST" == request.getMethod() && "Quit" == request.getCommand())
    {
        return std::make_unique<Quit>(service, request);
    }
    return std::make_unique<Unknown>(request);
}
} // namespace rialto::servermanager
