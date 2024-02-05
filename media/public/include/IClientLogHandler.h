/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Sky UK
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

#ifndef FIREBOLT_RIALTO_I_CLIENT_LOG_HANDLER_H_
#define FIREBOLT_RIALTO_I_CLIENT_LOG_HANDLER_H_

/**
 * @file IClientLogHandler.h
 *
 * The definition of the IClientLogHandler interface.
 *
 * This interface allows the user to define their own log handler for
 * any messages generated by the rialto client library
 */

#include <string>

namespace firebolt::rialto
{
/**
 * @brief IClientLogHandler  allows the user to define their own log handler for
 * any messages generated by the rialto client library
 */
class IClientLogHandler
{
public:
    IClientLogHandler() = default;
    virtual ~IClientLogHandler() = default;
    IClientLogHandler(const IClientLogHandler &) = delete;
    IClientLogHandler &operator=(const IClientLogHandler &) = delete;
    IClientLogHandler(IClientLogHandler &&) = delete;
    IClientLogHandler &operator=(IClientLogHandler &&) = delete;

    /**
     * @brief All possible log levels that could be used in the log callback
     */

    enum class Level
    {
        Fatal,
        Error,
        Warning,
        Milestone,
        Info,
        Debug,
        External
    };

    /**
     * @brief A callback method for every log item generated by the rialto client library
     *
     * @param[in]  level     : The log level
     * @param[in]  file      : The source code file where the log is defined
     * @param[in]  line      : The line number within the file where the log is defined
     * @param[in]  function  : The source code function within which the log is defined
     * @param[in]  message   : The message of the log
     *
     * @retval none
     */
    virtual void log(Level level, const std::string &file, int line, const std::string &function,
                     const std::string &message) = 0;
};
} // namespace firebolt::rialto

#endif // FIREBOLT_RIALTO_I_CLIENT_LOG_HANDLER_H_
