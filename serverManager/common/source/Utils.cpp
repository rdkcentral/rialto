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

#include "Utils.h"

namespace rialto::servermanager::common
{
const char *toString(const firebolt::rialto::common::SessionServerState &state)
{
    switch (state)
    {
    case firebolt::rialto::common::SessionServerState::UNINITIALIZED:
        return "Uninitialized";
    case firebolt::rialto::common::SessionServerState::INACTIVE:
        return "Inactive";
    case firebolt::rialto::common::SessionServerState::ACTIVE:
        return "Active";
    case firebolt::rialto::common::SessionServerState::NOT_RUNNING:
        return "NotRunning";
    case firebolt::rialto::common::SessionServerState::ERROR:
        return "Error";
    }
    return "Unknown";
}

RIALTO_DEBUG_LEVEL convert(const service::LoggingLevel &loggingLevel)
{
    switch (loggingLevel)
    {
    case service::LoggingLevel::FATAL:
    {
        return RIALTO_DEBUG_LEVEL_FATAL;
    }
    case service::LoggingLevel::ERROR:
    {
        return RIALTO_DEBUG_LEVEL(RIALTO_DEBUG_LEVEL_FATAL | RIALTO_DEBUG_LEVEL_ERROR);
    }
    case service::LoggingLevel::WARNING:
    {
        return RIALTO_DEBUG_LEVEL(RIALTO_DEBUG_LEVEL_FATAL | RIALTO_DEBUG_LEVEL_ERROR | RIALTO_DEBUG_LEVEL_WARNING);
    }
    case service::LoggingLevel::MILESTONE:
    {
        return RIALTO_DEBUG_LEVEL(RIALTO_DEBUG_LEVEL_FATAL | RIALTO_DEBUG_LEVEL_ERROR | RIALTO_DEBUG_LEVEL_WARNING |
                                  RIALTO_DEBUG_LEVEL_MILESTONE);
    }
    case service::LoggingLevel::INFO:
    {
        return RIALTO_DEBUG_LEVEL(RIALTO_DEBUG_LEVEL_FATAL | RIALTO_DEBUG_LEVEL_ERROR | RIALTO_DEBUG_LEVEL_WARNING |
                                  RIALTO_DEBUG_LEVEL_MILESTONE | RIALTO_DEBUG_LEVEL_INFO);
    }
    case service::LoggingLevel::DEBUG:
    {
        return RIALTO_DEBUG_LEVEL(RIALTO_DEBUG_LEVEL_FATAL | RIALTO_DEBUG_LEVEL_ERROR | RIALTO_DEBUG_LEVEL_WARNING |
                                  RIALTO_DEBUG_LEVEL_MILESTONE | RIALTO_DEBUG_LEVEL_INFO | RIALTO_DEBUG_LEVEL_DEBUG);
    }
    case service::LoggingLevel::DEFAULT:
    case service::LoggingLevel::UNCHANGED:
    {
        return RIALTO_DEBUG_LEVEL_DEFAULT;
    }
    }
    return RIALTO_DEBUG_LEVEL_DEFAULT;
}

void setLocalLogLevels(const service::LoggingLevels &logLevels)
{
    if (logLevels.defaultLoggingLevel != service::LoggingLevel::UNCHANGED)
    {
        firebolt::rialto::logging::setLogLevels(RIALTO_COMPONENT_DEFAULT, convert(logLevels.defaultLoggingLevel));
    }
    if (logLevels.clientLoggingLevel != service::LoggingLevel::UNCHANGED)
    {
        firebolt::rialto::logging::setLogLevels(RIALTO_COMPONENT_CLIENT, convert(logLevels.clientLoggingLevel));
    }
    if (logLevels.sessionServerLoggingLevel != service::LoggingLevel::UNCHANGED)
    {
        firebolt::rialto::logging::setLogLevels(RIALTO_COMPONENT_SERVER, convert(logLevels.sessionServerLoggingLevel));
    }
    if (logLevels.ipcLoggingLevel != service::LoggingLevel::UNCHANGED)
    {
        firebolt::rialto::logging::setLogLevels(RIALTO_COMPONENT_IPC, convert(logLevels.ipcLoggingLevel));
    }
    if (logLevels.serverManagerLoggingLevel != service::LoggingLevel::UNCHANGED)
    {
        firebolt::rialto::logging::setLogLevels(RIALTO_COMPONENT_SERVER_MANAGER,
                                                convert(logLevels.serverManagerLoggingLevel));
    }
    if (logLevels.commonLoggingLevel != service::LoggingLevel::UNCHANGED)
    {
        firebolt::rialto::logging::setLogLevels(RIALTO_COMPONENT_COMMON, convert(logLevels.commonLoggingLevel));
    }
}
} // namespace rialto::servermanager::common
