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
#include <gtest/gtest.h>
// #include <iostream>

using firebolt::rialto::common::SessionServerState;
using rialto::servermanager::common::convert;
using rialto::servermanager::common::setLocalLogLevels;
using rialto::servermanager::common::toString;
using rialto::servermanager::service::LoggingLevel;
using rialto::servermanager::service::LoggingLevels;

int printsToExpect(RIALTO_COMPONENT component, const rialto::servermanager::service::LoggingLevels& logLevels)
{
    rialto::servermanager::service::LoggingLevel lvl = rialto::servermanager::service::LoggingLevel::UNCHANGED;
    int expectedPrints = 0;
    (void) lvl; // When all Log Levels are disabled, lvl throws an unused but set variable error
    switch (component)
    {
        case RIALTO_COMPONENT_DEFAULT:
            lvl = logLevels.defaultLoggingLevel;
            break;
        case RIALTO_COMPONENT_CLIENT:
            lvl = logLevels.clientLoggingLevel;
           break;
        case RIALTO_COMPONENT_SERVER:
            lvl = logLevels.sessionServerLoggingLevel;
            break;
        case RIALTO_COMPONENT_IPC:
            lvl = logLevels.ipcLoggingLevel;
           break;
        case RIALTO_COMPONENT_SERVER_MANAGER:
            lvl = logLevels.serverManagerLoggingLevel;
            break;
        case RIALTO_COMPONENT_COMMON:
            lvl = logLevels.commonLoggingLevel;
            break;
        case RIALTO_COMPONENT_EXTERNAL:
        case RIALTO_COMPONENT_LAST:
        default:
            lvl = rialto::servermanager::service::LoggingLevel::UNCHANGED;
            break;
    }

    #ifdef RIALTO_LOG_FATAL_ENABLED
        if (lvl >= rialto::servermanager::service::LoggingLevel::FATAL)
            expectedPrints++;
    #endif
    #ifdef RIALTO_LOG_ERROR_ENABLED
        if (lvl >= rialto::servermanager::service::LoggingLevel::ERROR)
            expectedPrints++;
    #endif
    #ifdef RIALTO_LOG_WARN_ENABLED
        if (lvl >= rialto::servermanager::service::LoggingLevel::WARNING)
            expectedPrints++;
    #endif
    #ifdef RIALTO_LOG_MIL_ENABLED
        if (lvl >= rialto::servermanager::service::LoggingLevel::MILESTONE)
            expectedPrints++;
    #endif
    #ifdef RIALTO_LOG_INFO_ENABLED
        if (lvl >= rialto::servermanager::service::LoggingLevel::INFO &&
        !(lvl == rialto::servermanager::service::LoggingLevel::DEFAULT ||
          lvl == rialto::servermanager::service::LoggingLevel::UNCHANGED))
            expectedPrints++;
    #endif
    #ifdef RIALTO_LOG_DEBUG_ENABLED
        if (lvl >= rialto::servermanager::service::LoggingLevel::DEBUG &&
        !(lvl == rialto::servermanager::service::LoggingLevel::DEFAULT ||
          lvl == rialto::servermanager::service::LoggingLevel::UNCHANGED))
            expectedPrints++;
    #endif
        std::cout << static_cast <int> (lvl) << std::endl;
        return expectedPrints;
        
}

TEST(UtilsTest, ShouldReturnProperString)
{
    EXPECT_EQ(std::string("Uninitialized"), std::string(toString(SessionServerState::UNINITIALIZED)));
    EXPECT_EQ(std::string("Inactive"), std::string(toString(SessionServerState::INACTIVE)));
    EXPECT_EQ(std::string("Active"), std::string(toString(SessionServerState::ACTIVE)));
    EXPECT_EQ(std::string("NotRunning"), std::string(toString(SessionServerState::NOT_RUNNING)));
    EXPECT_EQ(std::string("Error"), std::string(toString(SessionServerState::ERROR)));
}

TEST(UtilsTest, ShouldReturnConvertedLogLevel)
{
    EXPECT_EQ(RIALTO_DEBUG_LEVEL_DEFAULT, convert(LoggingLevel::DEFAULT));
    EXPECT_EQ(RIALTO_DEBUG_LEVEL_FATAL, convert(LoggingLevel::FATAL));
    EXPECT_EQ(RIALTO_DEBUG_LEVEL(RIALTO_DEBUG_LEVEL_FATAL | RIALTO_DEBUG_LEVEL_ERROR), convert(LoggingLevel::ERROR));
    EXPECT_EQ(RIALTO_DEBUG_LEVEL(RIALTO_DEBUG_LEVEL_FATAL | RIALTO_DEBUG_LEVEL_ERROR | RIALTO_DEBUG_LEVEL_WARNING),
              convert(LoggingLevel::WARNING));
    EXPECT_EQ(RIALTO_DEBUG_LEVEL(RIALTO_DEBUG_LEVEL_FATAL | RIALTO_DEBUG_LEVEL_ERROR | RIALTO_DEBUG_LEVEL_WARNING |
                                 RIALTO_DEBUG_LEVEL_MILESTONE),
              convert(LoggingLevel::MILESTONE));
    EXPECT_EQ(RIALTO_DEBUG_LEVEL(RIALTO_DEBUG_LEVEL_FATAL | RIALTO_DEBUG_LEVEL_ERROR | RIALTO_DEBUG_LEVEL_WARNING |
                                 RIALTO_DEBUG_LEVEL_MILESTONE | RIALTO_DEBUG_LEVEL_INFO),
              convert(LoggingLevel::INFO));
    EXPECT_EQ(RIALTO_DEBUG_LEVEL(RIALTO_DEBUG_LEVEL_FATAL | RIALTO_DEBUG_LEVEL_ERROR | RIALTO_DEBUG_LEVEL_WARNING |
                                 RIALTO_DEBUG_LEVEL_MILESTONE | RIALTO_DEBUG_LEVEL_INFO | RIALTO_DEBUG_LEVEL_DEBUG),
              convert(LoggingLevel::DEBUG));
}

TEST(UtilsTest, ShouldSetLocalLogLevels)
{
    constexpr rialto::servermanager::service::LoggingLevels
        loggingLevels{rialto::servermanager::service::LoggingLevel::FATAL,
                      rialto::servermanager::service::LoggingLevel::ERROR,
                      rialto::servermanager::service::LoggingLevel::WARNING,
                      rialto::servermanager::service::LoggingLevel::MILESTONE,
                      rialto::servermanager::service::LoggingLevel::INFO,
                      rialto::servermanager::service::LoggingLevel::DEBUG};
    setLocalLogLevels(loggingLevels);

    for (uint32_t i = RIALTO_COMPONENT_DEFAULT; i < RIALTO_COMPONENT_EXTERNAL; i++)
    {

        RIALTO_COMPONENT component = static_cast<RIALTO_COMPONENT>(i);
        int counter{0};
        int expectedPrints = printsToExpect(component, loggingLevels);
        firebolt::rialto::logging::setLogHandler(component,
                                                 [&counter](RIALTO_DEBUG_LEVEL level, const char *file, int line,
                                                            const char *function, const char *message, size_t messageLen)
                                                 { ++counter; });
        RIALTO_LOG_FATAL(component, "RIALTO_LOG_FATAL");
        RIALTO_LOG_ERROR(component, "RIALTO_LOG_ERROR");
        RIALTO_LOG_WARN(component, "RIALTO_LOG_WARN");
        RIALTO_LOG_MIL(component, "RIALTO_LOG_MIL");
        RIALTO_LOG_INFO(component, "RIALTO_LOG_INFO");
        RIALTO_LOG_DEBUG(component, "RIALTO_LOG_DEBUG");
        EXPECT_EQ(counter, expectedPrints);
        firebolt::rialto::logging::setLogHandler(component, nullptr); // Reset to default
        
    }
}
