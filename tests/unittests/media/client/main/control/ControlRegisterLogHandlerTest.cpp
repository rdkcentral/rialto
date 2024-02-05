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

#include <gtest/gtest.h>

#include "ClientLogControl.h"
#include "RialtoClientLogging.h"

using namespace firebolt::rialto;
using namespace firebolt::rialto::client;

namespace
{
const std::string kLogTestStr("Test ABC");
} // namespace

class ClientLogControlTest : public ::testing::Test
{
protected:
};

class TestClientLogHandler : public IClientLogHandler
{
public:
    ~TestClientLogHandler() {}

    void log(Level level, const std::string &file, int line, const std::string &function, const std::string &message)
    {
        if (message == kLogTestStr)
        {
            m_gotExpectedLogMessage = true;
        }
    }

    bool m_gotExpectedLogMessage{false};
};

TEST_F(ClientLogControlTest, RegisterLogHandler)
{
    std::shared_ptr<TestClientLogHandler> logHandler = std::make_shared<TestClientLogHandler>();
    {
        IClientLogControl &control = ClientLogControlFactory::createFactory()->createClientLogControl();
        std::shared_ptr<IClientLogHandler> tmp = logHandler;
        EXPECT_TRUE(control.registerLogHandler(tmp, true));
    }

    // Generate a log entry
    RIALTO_CLIENT_LOG_ERROR("%s", kLogTestStr.c_str());

    EXPECT_TRUE(logHandler->m_gotExpectedLogMessage);
}

TEST_F(ClientLogControlTest, ShouldUpdateLogHandler)
{
    IClientLogControl &control1 = IClientLogControlFactory::createFactory()->createClientLogControl();

    std::shared_ptr<TestClientLogHandler> logHandler1 = std::make_shared<TestClientLogHandler>();
    {
        std::shared_ptr<IClientLogHandler> tmp = logHandler1;
        EXPECT_TRUE(control1.registerLogHandler(tmp, true));
    }
    // Generate a log entry
    RIALTO_CLIENT_LOG_ERROR("%s", kLogTestStr.c_str());

    IClientLogControl &control2 = IClientLogControlFactory::createFactory()->createClientLogControl();
    EXPECT_EQ(&control1, &control2); // IClientLogControl Ought to be a singleton

    std::shared_ptr<TestClientLogHandler> logHandler2 = std::make_shared<TestClientLogHandler>();
    {
        std::shared_ptr<IClientLogHandler> tmp = logHandler2;
        control2.registerLogHandler(tmp, true);
    }
    // Generate a log entry
    RIALTO_CLIENT_LOG_ERROR("%s", kLogTestStr.c_str());

    EXPECT_TRUE(logHandler1->m_gotExpectedLogMessage);
    EXPECT_TRUE(logHandler2->m_gotExpectedLogMessage);
}

TEST_F(ClientLogControlTest, ShouldCancelLogHandler)
{
    IClientLogControl &control = IClientLogControlFactory::createFactory()->createClientLogControl();

    std::shared_ptr<TestClientLogHandler> logHandler = std::make_shared<TestClientLogHandler>();
    {
        std::shared_ptr<IClientLogHandler> tmp = logHandler;
        control.registerLogHandler(tmp, true);
    }
    // Generate a log entry
    RIALTO_CLIENT_LOG_ERROR("%s", kLogTestStr.c_str());
    EXPECT_TRUE(logHandler->m_gotExpectedLogMessage);

    // Cancel the log handler
    {
        std::shared_ptr<IClientLogHandler> tmp; // nullptr
        EXPECT_TRUE(control.registerLogHandler(tmp, false));
    }

    logHandler->m_gotExpectedLogMessage = false;

    // Generate a log entry
    RIALTO_CLIENT_LOG_ERROR("%s", kLogTestStr.c_str());

    // Log handler should not have been used
    EXPECT_FALSE(logHandler->m_gotExpectedLogMessage);
}
