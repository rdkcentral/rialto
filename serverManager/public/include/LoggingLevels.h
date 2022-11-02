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
 *
 * @file LoggingLevels.h
 *
 * This file comprises the struct definition of LoggingLevels
 * Struct represents rialto logging levels for each component
 */

#ifndef RIALTO_SERVERMANAGER_SERVICE_LOGGING_LEVELS_H_
#define RIALTO_SERVERMANAGER_SERVICE_LOGGING_LEVELS_H_

namespace rialto::servermanager::service
{
/**
 * @brief Represents all rialto logging levels
 */
enum class LoggingLevel
{
    FATAL,
    ERROR,
    WARNING,
    MILESTONE,
    INFO,
    DEBUG,
    DEFAULT,
    UNCHANGED
};

/**
 * @brief Represents logging levels for each component
 */
struct LoggingLevels
{
    LoggingLevel defaultLoggingLevel{LoggingLevel::UNCHANGED};
    LoggingLevel clientLoggingLevel{LoggingLevel::UNCHANGED};
    LoggingLevel sessionServerLoggingLevel{LoggingLevel::UNCHANGED};
    LoggingLevel ipcLoggingLevel{LoggingLevel::UNCHANGED};
    LoggingLevel serverManagerLoggingLevel{LoggingLevel::UNCHANGED};
    LoggingLevel commonLoggingLevel{LoggingLevel::UNCHANGED};
};
} // namespace rialto::servermanager::service

#endif // RIALTO_SERVERMANAGER_SERVICE_LOGGING_LEVELS_H_
