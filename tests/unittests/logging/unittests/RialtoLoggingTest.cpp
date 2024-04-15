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

#include "RialtoLogging.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto::logging;

uint32_t g_handlerCalledCount = 0U;

class RialtoLoggingTest : public ::testing::Test
{
protected:
    virtual void SetUp() { g_handlerCalledCount = 0U; }

    virtual void TearDown() {}

    static void TestLogHandler(RIALTO_DEBUG_LEVEL level, const char *file, int line, const char *function,
                               const char *message, size_t messageLen)
    {
        g_handlerCalledCount++;
    }

    static void EmptyLogHandler(RIALTO_DEBUG_LEVEL level, const char *file, int line, const char *function,
                                const char *message, size_t messageLen)
    {
    }

    void TestExpectLogCalls(RIALTO_COMPONENT component, RIALTO_DEBUG_LEVEL logLevels)
    {
        uint32_t expectedHandlerCalledCount = g_handlerCalledCount;

        RIALTO_LOG_FATAL(component, "RIALTO_LOG_FATAL");
        EXPECT_EQ(g_handlerCalledCount,
                  (logLevels & RIALTO_DEBUG_LEVEL_FATAL) ? ++expectedHandlerCalledCount : expectedHandlerCalledCount);

        RIALTO_LOG_SYS_FATAL(component, 1, "RIALTO_LOG_SYS_FATAL");
        EXPECT_EQ(g_handlerCalledCount,
                  (logLevels & RIALTO_DEBUG_LEVEL_FATAL) ? ++expectedHandlerCalledCount : expectedHandlerCalledCount);

        RIALTO_LOG_ERROR(component, "RIALTO_LOG_ERROR");
        EXPECT_EQ(g_handlerCalledCount,
                  (logLevels & RIALTO_DEBUG_LEVEL_ERROR) ? ++expectedHandlerCalledCount : expectedHandlerCalledCount);

        RIALTO_LOG_SYS_ERROR(component, 1, "RIALTO_LOG_SYS_ERROR");
        EXPECT_EQ(g_handlerCalledCount,
                  (logLevels & RIALTO_DEBUG_LEVEL_ERROR) ? ++expectedHandlerCalledCount : expectedHandlerCalledCount);

        RIALTO_LOG_WARN(component, "RIALTO_LOG_WARN");
        EXPECT_EQ(g_handlerCalledCount,
                  (logLevels & RIALTO_DEBUG_LEVEL_WARNING) ? ++expectedHandlerCalledCount : expectedHandlerCalledCount);

        RIALTO_LOG_SYS_WARN(component, 1, "RIALTO_LOG_SYS_WARN");
        EXPECT_EQ(g_handlerCalledCount,
                  (logLevels & RIALTO_DEBUG_LEVEL_WARNING) ? ++expectedHandlerCalledCount : expectedHandlerCalledCount);

        RIALTO_LOG_MIL(component, "RIALTO_LOG_MIL");
        EXPECT_EQ(g_handlerCalledCount, (logLevels & RIALTO_DEBUG_LEVEL_MILESTONE) ? ++expectedHandlerCalledCount
                                                                                   : expectedHandlerCalledCount);

        RIALTO_LOG_INFO(component, "RIALTO_LOG_INFO");
        EXPECT_EQ(g_handlerCalledCount,
                  (logLevels & RIALTO_DEBUG_LEVEL_INFO) ? ++expectedHandlerCalledCount : expectedHandlerCalledCount);

        RIALTO_LOG_DEBUG(component, "RIALTO_LOG_DEBUG");
        EXPECT_EQ(g_handlerCalledCount,
                  (logLevels & RIALTO_DEBUG_LEVEL_DEBUG) ? ++expectedHandlerCalledCount : expectedHandlerCalledCount);
    }
};

/**
 * Test that RialtoLogging can function without having to set the log handler or log levels.
 */
TEST_F(RialtoLoggingTest, DefaultHandler)
{
    TestExpectLogCalls(RIALTO_COMPONENT_DEFAULT, static_cast<RIALTO_DEBUG_LEVEL>(0U));
}

/**
 * Test that RialtoLogging returns error if setLogLevels passed an invalid RIALTO_COMPONENT.
 */
TEST_F(RialtoLoggingTest, SetLogLevelsInvalidComponent)
{
    RIALTO_DEBUG_LEVEL logLevel = RIALTO_DEBUG_LEVEL_INFO;
    setLogLevels(RIALTO_COMPONENT_LAST, logLevel);
}

/**
 * Test that RialtoLogging returns error if setLogHandler passed an invalid RIALTO_COMPONENT.
 */
TEST_F(RialtoLoggingTest, SetLogHandlerInvalidComponent)
{
    setLogHandler(RIALTO_COMPONENT_LAST, RialtoLoggingTest::TestLogHandler, false);
}
/**
 * Test that all the defined components can set handler and levels .
 */
TEST_F(RialtoLoggingTest, AllComponents)
{
    RIALTO_DEBUG_LEVEL logLevel = RIALTO_DEBUG_LEVEL_INFO;
    RIALTO_DEBUG_LEVEL defaultLogLevel = RIALTO_DEBUG_LEVEL_DEFAULT;

    for (uint32_t i = RIALTO_COMPONENT_DEFAULT; i < RIALTO_COMPONENT_LAST; i++)
    {
        RIALTO_COMPONENT component = static_cast<RIALTO_COMPONENT>(i);
        setLogHandler(component, RialtoLoggingTest::TestLogHandler, false);
        setLogLevels(component, logLevel);
        TestExpectLogCalls(component, logLevel);

        /* Reset to default incase all components are using the variables */
        setLogHandler(component, RialtoLoggingTest::EmptyLogHandler, false);
        setLogLevels(component, defaultLogLevel);
    }
}

/**
 * Test that setting the log levels to FATAL, only processes FATAL logs.
 */
TEST_F(RialtoLoggingTest, SetLogLevelFatal)
{
    RIALTO_DEBUG_LEVEL logLevel = RIALTO_DEBUG_LEVEL_FATAL;

    setLogHandler(RIALTO_COMPONENT_DEFAULT, RialtoLoggingTest::TestLogHandler, false);
    setLogLevels(RIALTO_COMPONENT_DEFAULT, logLevel);

    TestExpectLogCalls(RIALTO_COMPONENT_DEFAULT, logLevel);
}

/**
 * Test that setting the log levels to ERROR, only processes ERROR logs.
 */
TEST_F(RialtoLoggingTest, SetLogLevelError)
{
    RIALTO_DEBUG_LEVEL logLevel = RIALTO_DEBUG_LEVEL_ERROR;

    setLogHandler(RIALTO_COMPONENT_DEFAULT, RialtoLoggingTest::TestLogHandler, false);
    setLogLevels(RIALTO_COMPONENT_DEFAULT, logLevel);

    TestExpectLogCalls(RIALTO_COMPONENT_DEFAULT, logLevel);
}

/**
 * Test that setting the log levels to WARNING, only processes WARNING logs.
 */
TEST_F(RialtoLoggingTest, SetLogLevelWarning)
{
    RIALTO_DEBUG_LEVEL logLevel = RIALTO_DEBUG_LEVEL_WARNING;

    setLogHandler(RIALTO_COMPONENT_DEFAULT, RialtoLoggingTest::TestLogHandler, false);
    setLogLevels(RIALTO_COMPONENT_DEFAULT, logLevel);

    TestExpectLogCalls(RIALTO_COMPONENT_DEFAULT, logLevel);
}

/**
 * Test that setting the log levels to MILESTONE, only processes MILESTONE logs.
 */
TEST_F(RialtoLoggingTest, SetLogLevelMilestone)
{
    RIALTO_DEBUG_LEVEL logLevel = RIALTO_DEBUG_LEVEL_MILESTONE;

    setLogHandler(RIALTO_COMPONENT_DEFAULT, RialtoLoggingTest::TestLogHandler, false);
    setLogLevels(RIALTO_COMPONENT_DEFAULT, logLevel);

    TestExpectLogCalls(RIALTO_COMPONENT_DEFAULT, logLevel);
}

/**
 * Test that setting the log levels to INFO, only processes INFO logs.
 */
TEST_F(RialtoLoggingTest, SetLogLevelInfo)
{
    RIALTO_DEBUG_LEVEL logLevel = RIALTO_DEBUG_LEVEL_INFO;

    setLogHandler(RIALTO_COMPONENT_DEFAULT, RialtoLoggingTest::TestLogHandler, false);
    setLogLevels(RIALTO_COMPONENT_DEFAULT, logLevel);

    TestExpectLogCalls(RIALTO_COMPONENT_DEFAULT, logLevel);
}

/**
 * Test that setting the log levels to DEBUG, only processes DEBUG logs.
 */
TEST_F(RialtoLoggingTest, SetLogLevelDebug)
{
    RIALTO_DEBUG_LEVEL logLevel = RIALTO_DEBUG_LEVEL_DEBUG;

    setLogHandler(RIALTO_COMPONENT_DEFAULT, RialtoLoggingTest::TestLogHandler, false);
    setLogLevels(RIALTO_COMPONENT_DEFAULT, logLevel);

    TestExpectLogCalls(RIALTO_COMPONENT_DEFAULT, logLevel);
}

/**
 * Test that setting the log levels to multiple levels, only processes the requested logs.
 */
TEST_F(RialtoLoggingTest, SetLogLevelMultiple)
{
    RIALTO_DEBUG_LEVEL logLevel = static_cast<RIALTO_DEBUG_LEVEL>(RIALTO_DEBUG_LEVEL_INFO | RIALTO_DEBUG_LEVEL_ERROR |
                                                                  RIALTO_DEBUG_LEVEL_MILESTONE);

    setLogHandler(RIALTO_COMPONENT_DEFAULT, RialtoLoggingTest::TestLogHandler, false);
    setLogLevels(RIALTO_COMPONENT_DEFAULT, logLevel);

    TestExpectLogCalls(RIALTO_COMPONENT_DEFAULT, logLevel);
}

/**
 * Test that setting the log handler to ignore levels works
 */
TEST_F(RialtoLoggingTest, SetLogLevelIgnoreForHandler)
{
    RIALTO_DEBUG_LEVEL logLevelNone = static_cast<RIALTO_DEBUG_LEVEL>(0);

    // Set up the logging to report NOTHING...
    setLogLevels(RIALTO_COMPONENT_DEFAULT, logLevelNone);

    // Ask the log handler to ignore any log levels...
    setLogHandler(RIALTO_COMPONENT_DEFAULT, RialtoLoggingTest::TestLogHandler, true);

    // Because we're ignoring log levels now, we expect the following...
    ASSERT_EQ(getLogLevels(RIALTO_COMPONENT_DEFAULT), RIALTO_DEBUG_LEVEL_EXTERNAL);

    // Expect the log handler to be called for all types of log despite
    // the current log level of nothing...
    RIALTO_DEBUG_LEVEL logLevelAll = static_cast<RIALTO_DEBUG_LEVEL>(
        RIALTO_DEBUG_LEVEL_INFO | RIALTO_DEBUG_LEVEL_ERROR | RIALTO_DEBUG_LEVEL_WARNING | RIALTO_DEBUG_LEVEL_FATAL |
        RIALTO_DEBUG_LEVEL_MILESTONE | RIALTO_DEBUG_LEVEL_DEBUG);
    TestExpectLogCalls(RIALTO_COMPONENT_DEFAULT, logLevelAll);
}

/**
 * Test that getting the log levels returns set log level.
 */
TEST_F(RialtoLoggingTest, GetLogLevelDebug)
{
    RIALTO_DEBUG_LEVEL logLevel = RIALTO_DEBUG_LEVEL_DEBUG;

    setLogHandler(RIALTO_COMPONENT_DEFAULT, RialtoLoggingTest::TestLogHandler, false);
    setLogLevels(RIALTO_COMPONENT_DEFAULT, logLevel);

    ASSERT_EQ(getLogLevels(RIALTO_COMPONENT_DEFAULT), logLevel);
}
