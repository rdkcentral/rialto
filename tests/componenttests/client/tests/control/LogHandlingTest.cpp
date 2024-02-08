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
 */

#include "ClientComponentTest.h"
#include "RialtoLogging.h"
#include <gtest/gtest.h>

namespace firebolt::rialto::client::ct
{
class LogHandlingTest : public ClientComponentTest
{
};

/*
 * Component Test: Test the client log control
 * Test Objective:
 *  Test that the client can successfully set a log handler using client log control, and the correct
 *  log messages are forwarded.
 *
 * Sequence Diagrams:
 *  Logging - https://wiki.rdkcentral.com/display/ASP/Logging
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: ClientLogControl
 *
 * Test Initialize:
 *
 * Test Steps:
 *  Step 1: Create client log control
 *   Create an instance of ClientLogControl.
 *   Check that the object returned is valid.
 *
 *  Step 2: Set log level to Error only
 *   setLogLevel to error for client component.
 *
 *  Step 3: Set log handler with ignoreLogLevels
 *   registerLogHandler with ignoreLogLevels.
 *   Expect success.
 *
 *  Step 4: Log Error
 *   Log a client component error.
 *   Expect that the log handler receives the new log.
 *
 *  Step 5: Log Debug, Info, Fatal, Warning & Milstone
 *   Log all other levels.
 *   Expect that the log handler receives all the logs.
 *
 *  Step 6: Set log handler without ignoreLogLevels
 *   registerLogHandler without ignoreLogLevels.
 *   Expect success.
 *
 *  Step 7: Log Error
 *   Log a client component error.
 *   Expect that the log handler receives the new log.
 *
 *  Step 8: Does not log Debug, Info, Fatal, Warning & Milstone
 *   Log all other levels.
 *   Expect none of the logs are received by the log handler.
 *
 *  Step 9: Set log handler to default
 *   registerLogHandler with ignoreLogLevels.
 *   Expect success.
 *
 *  Step 10: Set log level to all logging
 *   setLogLevel to Error, Debug, Info, Fatal, Warning & Milstone for client component.
 *
 *  Step 11: Does not log Error, Debug, Info, Fatal, Warning & Milstone
 *   Log all levels.
 *   Expect none of the logs are received by the log handler.
 *
 * Test Teardown:
 *
 * Expected Results:
 *  That the client can register a log handler and retreive log.
 *  That log levels are ignored when ignoreLogLevels is set.
 *  That a log handler can be removed.
 *
 * Code:
 */
TEST_F(LogHandlingTest, clientControl)
{
    // Step 1: Create client log control
    ClientLogControlTestMethods::createClientLogControl();

    // Step 2: Set log level to Error only
    ClientLogControlTestMethods::setLogLevel(RIALTO_DEBUG_LEVEL(RIALTO_DEBUG_LEVEL_ERROR));

    // Step 3: Set log handler with ignoreLogLevels
    ClientLogControlTestMethods::registerLogHandlerWithIgnoreLevels();

    // Step 4: Log Error
    ClientLogControlTestMethods::shouldLog(IClientLogHandler::Level::Error);
    ClientLogControlTestMethods::log(IClientLogHandler::Level::Error);

    // Step 5: Log Debug, Info, Fatal, Warning & Milstone
    for (int lvl = 0; lvl < static_cast<int>(IClientLogHandler::Level::External); lvl++)
    {
        if (lvl != static_cast<int>(IClientLogHandler::Level::Error))
        {
            ClientLogControlTestMethods::shouldLog(static_cast<IClientLogHandler::Level>(lvl));
            ClientLogControlTestMethods::log(static_cast<IClientLogHandler::Level>(lvl));
        }
    }

    // Step 6: Set log handler without ignoreLogLevels
    ClientLogControlTestMethods::shouldReplaceLogHandler(); // Removal of log handler triggers logs in rialto
    ClientLogControlTestMethods::registerLogHandler();

    // Step 7: Log Error
    ClientLogControlTestMethods::shouldLog(IClientLogHandler::Level::Error);
    ClientLogControlTestMethods::log(IClientLogHandler::Level::Error);

    // Step 8: Does not log Debug, Info, Fatal, Warning & Milstone
    for (int lvl = 0; lvl < static_cast<int>(IClientLogHandler::Level::External); lvl++)
    {
        if (lvl != static_cast<int>(IClientLogHandler::Level::Error))
        {
            ClientLogControlTestMethods::log(static_cast<IClientLogHandler::Level>(lvl));
        }
    }

    // Step 9: Set log handler to default
    ClientLogControlTestMethods::unregisterLogHandler();

    // Step 10: Set log level to all logging
    ClientLogControlTestMethods::setLogLevel(
        RIALTO_DEBUG_LEVEL(RIALTO_DEBUG_LEVEL_FATAL | RIALTO_DEBUG_LEVEL_ERROR | RIALTO_DEBUG_LEVEL_WARNING |
                           RIALTO_DEBUG_LEVEL_MILESTONE | RIALTO_DEBUG_LEVEL_INFO | RIALTO_DEBUG_LEVEL_DEBUG));

    // Step 11: Does not log Error, Debug, Info, Fatal, Warning & Milstone
    for (int lvl = 0; lvl < static_cast<int>(IClientLogHandler::Level::External); lvl++)
    {
        ClientLogControlTestMethods::log(static_cast<IClientLogHandler::Level>(lvl));
    }
}
} // namespace firebolt::rialto::client::ct
