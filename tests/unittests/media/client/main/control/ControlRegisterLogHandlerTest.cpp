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

#include "ClientControllerMock.h"
#include "Control.h"
#include "RialtoClientLogging.h"

using namespace firebolt::rialto;
using namespace firebolt::rialto::client;

using ::testing::_;
using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::StrictMock;

namespace
{
const std::string kLogTestStr("Test ABC");
} // namespace

class RialtoClientControlRegisterLogHandlerTest : public ::testing::Test
{
protected:
    StrictMock<ClientControllerMock> m_clientControllerMock;
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

TEST_F(RialtoClientControlRegisterLogHandlerTest, RegisterLogHandler)
{
    std::unique_ptr<IControl> control;

    EXPECT_NO_THROW(control = std::make_unique<Control>(m_clientControllerMock));

    std::shared_ptr<TestClientLogHandler> logHandler = std::make_shared<TestClientLogHandler>();
    {
        std::shared_ptr<IClientLogHandler> tmp = logHandler;
        EXPECT_TRUE(control->registerLogHandler(tmp, true));
    }

    // Generate a log entry
    RIALTO_CLIENT_LOG_ERROR("%s", kLogTestStr.c_str());

    EXPECT_TRUE(logHandler->m_gotExpectedLogMessage);

    // Destroy
    control.reset();
}

TEST_F(RialtoClientControlRegisterLogHandlerTest, PreRegisterLogHandler)
{
    auto factory = IControlFactory::createFactory();

    std::shared_ptr<TestClientLogHandler> logHandler = std::make_shared<TestClientLogHandler>();
    {
        std::shared_ptr<IClientLogHandler> tmp = logHandler;
        factory->preRegisterLogHandler(tmp, true);
    }

    std::shared_ptr<IControl> control;
    // Can't use the real factory in test environment
    EXPECT_NO_THROW(control = std::make_unique<Control>(m_clientControllerMock));
    ASSERT_NE(control, nullptr);

    // Generate a log entry
    RIALTO_CLIENT_LOG_FATAL("%s", kLogTestStr.c_str());
    // For code coverage we try the other levels...
    RIALTO_CLIENT_LOG_MIL("%s", kLogTestStr.c_str());
    RIALTO_CLIENT_LOG_WARN("%s", kLogTestStr.c_str());
    RIALTO_CLIENT_LOG_INFO("%s", kLogTestStr.c_str());
    RIALTO_CLIENT_LOG_SYS_ERROR(0, "%s", kLogTestStr.c_str());

    EXPECT_TRUE(logHandler->m_gotExpectedLogMessage);

    // Destroy
    control.reset();
}

TEST_F(RialtoClientControlRegisterLogHandlerTest, ShouldUpdateLogHandler)
{
    auto factory = IControlFactory::createFactory();

    std::shared_ptr<TestClientLogHandler> logHandler1 = std::make_shared<TestClientLogHandler>();
    {
        std::shared_ptr<IClientLogHandler> tmp = logHandler1;
        factory->preRegisterLogHandler(tmp, true);
    }

    std::shared_ptr<IControl> control;
    // Can't use the real factory in test environment
    EXPECT_NO_THROW(control = std::make_unique<Control>(m_clientControllerMock));
    ASSERT_NE(control, nullptr);

    // Generate a log entry
    RIALTO_CLIENT_LOG_ERROR("%s", kLogTestStr.c_str());

    std::shared_ptr<TestClientLogHandler> logHandler2 = std::make_shared<TestClientLogHandler>();
    {
        // Update the log handler
        std::shared_ptr<IClientLogHandler> tmp = logHandler2;
        EXPECT_TRUE(control->registerLogHandler(tmp, true));
    }

    // Generate a log entry
    RIALTO_CLIENT_LOG_ERROR("%s", kLogTestStr.c_str());

    EXPECT_TRUE(logHandler1->m_gotExpectedLogMessage);
    EXPECT_TRUE(logHandler2->m_gotExpectedLogMessage);

    // Destroy
    control.reset();
}

TEST_F(RialtoClientControlRegisterLogHandlerTest, ShouldCancelLogHandler)
{
    auto factory = IControlFactory::createFactory();

    std::shared_ptr<TestClientLogHandler> logHandler = std::make_shared<TestClientLogHandler>();
    {
        std::shared_ptr<IClientLogHandler> tmp = logHandler;
        factory->preRegisterLogHandler(tmp, true);
    }

    std::shared_ptr<IControl> control;
    // Can't use the real factory in test environment
    EXPECT_NO_THROW(control = std::make_unique<Control>(m_clientControllerMock));
    ASSERT_NE(control, nullptr);

    // Generate a log entry
    RIALTO_CLIENT_LOG_ERROR("%s", kLogTestStr.c_str());
    EXPECT_TRUE(logHandler->m_gotExpectedLogMessage);

    // Cancel the log handler
    {
        std::shared_ptr<IClientLogHandler> tmp; // nullptr
        EXPECT_TRUE(control->registerLogHandler(tmp, false));
    }

    logHandler->m_gotExpectedLogMessage = false;

    // Generate a log entry
    RIALTO_CLIENT_LOG_ERROR("%s", kLogTestStr.c_str());

    // Log handler should not have been used
    EXPECT_FALSE(logHandler->m_gotExpectedLogMessage);

    // Destroy
    control.reset();
}
