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

#include "EnvVariableParser.h"
#include <cstdlib>
#include <gtest/gtest.h>

using namespace firebolt::rialto::logging;

class EnvVariableParserTest : public ::testing::Test
{
public:
    EnvVariableParserTest() = default;
    ~EnvVariableParserTest() = default;
};

TEST_F(EnvVariableParserTest, SetDefaultForAllComponentsWhenEnvVariableNotSet)
{
    unsetenv("RIALTO_DEBUG");
    EnvVariableParser parser;
    for (uint32_t i = RIALTO_COMPONENT_DEFAULT; i < RIALTO_COMPONENT_LAST; i++)
    {
        RIALTO_COMPONENT component = static_cast<RIALTO_COMPONENT>(i);
        EXPECT_EQ(RIALTO_DEBUG_LEVEL_DEFAULT, parser.getLevel(component));
    }
}

TEST_F(EnvVariableParserTest, SetErrorForAllComponentsUsingIntValue)
{
    const RIALTO_DEBUG_LEVEL expectedLevel = RIALTO_DEBUG_LEVEL(RIALTO_DEBUG_LEVEL_FATAL | RIALTO_DEBUG_LEVEL_ERROR);
    setenv("RIALTO_DEBUG", "1", 1);
    EnvVariableParser parser;
    for (uint32_t i = RIALTO_COMPONENT_CLIENT; i < RIALTO_COMPONENT_LAST; i++)
    {
        RIALTO_COMPONENT component = static_cast<RIALTO_COMPONENT>(i);
        EXPECT_EQ(expectedLevel, parser.getLevel(component));
    }
}

TEST_F(EnvVariableParserTest, SetErrorForAllComponentsUsingStringValue)
{
    const RIALTO_DEBUG_LEVEL expectedLevel = RIALTO_DEBUG_LEVEL(RIALTO_DEBUG_LEVEL_FATAL | RIALTO_DEBUG_LEVEL_ERROR);
    setenv("RIALTO_DEBUG", "*:1", 1);
    EnvVariableParser parser;
    for (uint32_t i = RIALTO_COMPONENT_CLIENT; i < RIALTO_COMPONENT_LAST; i++)
    {
        RIALTO_COMPONENT component = static_cast<RIALTO_COMPONENT>(i);
        EXPECT_EQ(expectedLevel, parser.getLevel(component));
    }
}

TEST_F(EnvVariableParserTest, SetFakeComponentShouldNotCauseError)
{
    const RIALTO_DEBUG_LEVEL expectedLevel = RIALTO_DEBUG_LEVEL(RIALTO_DEBUG_LEVEL_FATAL | RIALTO_DEBUG_LEVEL_ERROR);
    setenv("RIALTO_DEBUG", "*:1;fakecomponent:3", 1);
    EnvVariableParser parser;
    for (uint32_t i = RIALTO_COMPONENT_CLIENT; i < RIALTO_COMPONENT_LAST; i++)
    {
        RIALTO_COMPONENT component = static_cast<RIALTO_COMPONENT>(i);
        EXPECT_EQ(expectedLevel, parser.getLevel(component));
    }
}

TEST_F(EnvVariableParserTest, SetWithSyntaxErrorShouldNotCauseError)
{
    const RIALTO_DEBUG_LEVEL expectedLevel = RIALTO_DEBUG_LEVEL(RIALTO_DEBUG_LEVEL_FATAL | RIALTO_DEBUG_LEVEL_ERROR);
    setenv("RIALTO_DEBUG", "*:1;syntaxError1;syntax:error:2;ipc:error3;sessionserver=4", 1);
    EnvVariableParser parser;
    for (uint32_t i = RIALTO_COMPONENT_CLIENT; i < RIALTO_COMPONENT_LAST; i++)
    {
        RIALTO_COMPONENT component = static_cast<RIALTO_COMPONENT>(i);
        EXPECT_EQ(expectedLevel, parser.getLevel(component));
    }
}

TEST_F(EnvVariableParserTest, SetFatalAndErrorForIpcDefaultForRest)
{
    const RIALTO_DEBUG_LEVEL expectedLevel = RIALTO_DEBUG_LEVEL(RIALTO_DEBUG_LEVEL_FATAL | RIALTO_DEBUG_LEVEL_ERROR);
    setenv("RIALTO_DEBUG", "ipc:1", 1);
    EnvVariableParser parser;
    EXPECT_EQ(RIALTO_DEBUG_LEVEL_DEFAULT, parser.getLevel(RIALTO_COMPONENT_CLIENT));
    EXPECT_EQ(RIALTO_DEBUG_LEVEL_DEFAULT, parser.getLevel(RIALTO_COMPONENT_SERVER));
    EXPECT_EQ(expectedLevel, parser.getLevel(RIALTO_COMPONENT_IPC));
    EXPECT_EQ(RIALTO_DEBUG_LEVEL_DEFAULT, parser.getLevel(RIALTO_COMPONENT_SERVER_MANAGER));
    EXPECT_EQ(RIALTO_DEBUG_LEVEL_DEFAULT, parser.getLevel(RIALTO_COMPONENT_COMMON));
}

TEST_F(EnvVariableParserTest, SetFatalAndErrorForIpcFatalForRestVersion1)
{
    const RIALTO_DEBUG_LEVEL expectedLevel = RIALTO_DEBUG_LEVEL(RIALTO_DEBUG_LEVEL_FATAL | RIALTO_DEBUG_LEVEL_ERROR);
    setenv("RIALTO_DEBUG", "ipc:1;*:0", 1);
    EnvVariableParser parser;
    EXPECT_EQ(RIALTO_DEBUG_LEVEL_FATAL, parser.getLevel(RIALTO_COMPONENT_CLIENT));
    EXPECT_EQ(RIALTO_DEBUG_LEVEL_FATAL, parser.getLevel(RIALTO_COMPONENT_SERVER));
    EXPECT_EQ(expectedLevel, parser.getLevel(RIALTO_COMPONENT_IPC));
    EXPECT_EQ(RIALTO_DEBUG_LEVEL_FATAL, parser.getLevel(RIALTO_COMPONENT_SERVER_MANAGER));
    EXPECT_EQ(RIALTO_DEBUG_LEVEL_FATAL, parser.getLevel(RIALTO_COMPONENT_COMMON));
}

TEST_F(EnvVariableParserTest, SetFatalAndErrorForIpcFatalForRestVersion2)
{
    const RIALTO_DEBUG_LEVEL expectedLevel = RIALTO_DEBUG_LEVEL(RIALTO_DEBUG_LEVEL_FATAL | RIALTO_DEBUG_LEVEL_ERROR);
    setenv("RIALTO_DEBUG", "*:0;ipc:1", 1);
    EnvVariableParser parser;
    EXPECT_EQ(RIALTO_DEBUG_LEVEL_FATAL, parser.getLevel(RIALTO_COMPONENT_CLIENT));
    EXPECT_EQ(RIALTO_DEBUG_LEVEL_FATAL, parser.getLevel(RIALTO_COMPONENT_SERVER));
    EXPECT_EQ(expectedLevel, parser.getLevel(RIALTO_COMPONENT_IPC));
    EXPECT_EQ(RIALTO_DEBUG_LEVEL_FATAL, parser.getLevel(RIALTO_COMPONENT_SERVER_MANAGER));
    EXPECT_EQ(RIALTO_DEBUG_LEVEL_FATAL, parser.getLevel(RIALTO_COMPONENT_COMMON));
}

TEST_F(EnvVariableParserTest, LogToConsoleDisabledWhenNoVarSet)
{
    unsetenv("RIALTO_CONSOLE_LOG");
    EnvVariableParser parser;
    EXPECT_FALSE(parser.isConsoleLoggingEnabled());
}

TEST_F(EnvVariableParserTest, SetLogToConsole)
{
    setenv("RIALTO_CONSOLE_LOG", "1", 1);
    EnvVariableParser parser;
    EXPECT_TRUE(parser.isConsoleLoggingEnabled());
}

TEST_F(EnvVariableParserTest, LogToConsoleDisabled)
{
    setenv("RIALTO_CONSOLE_LOG", "0", 1);
    EnvVariableParser parser;
    EXPECT_FALSE(parser.isConsoleLoggingEnabled());
}
