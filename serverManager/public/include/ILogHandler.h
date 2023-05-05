/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Sky UK
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
 * @file ILogHandler.h
 *
 * This file comprises the class definition of ILogHandler.
 * An interface, which provides API of custom server manager logging handler
 */

#ifndef RIALTO_SERVERMANAGER_SERVICE_I_LOG_HANDLER_H_
#define RIALTO_SERVERMANAGER_SERVICE_I_LOG_HANDLER_H_

#include <string>

namespace rialto::servermanager::service
{
class ILogHandler
{
public:
    ILogHandler() = default;
    virtual ~ILogHandler() = default;
    ILogHandler(const ILogHandler &) = delete;
    ILogHandler &operator=(const ILogHandler &) = delete;
    ILogHandler(ILogHandler &&) = delete;
    ILogHandler &operator=(ILogHandler &&) = delete;

    enum Level
    {
        Fatal,
        Error,
        Warning,
        Milestone,
        Info,
        Debug,
        External
    };

    virtual void log(Level level, const std::string &file, int line, const std::string &function,
                     const std::string &message) const = 0;
};
} // namespace rialto::servermanager::service

#endif // RIALTO_SERVERMANAGER_SERVICE_I_LOG_HANDLER_H_
