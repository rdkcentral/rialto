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
#include "ClientLogHandlerMock.h"
#include "RialtoClientLogging.h"

using namespace firebolt::rialto;
using namespace firebolt::rialto::client;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Ne;
using ::testing::StrictMock;
using ::testing::StrNe;

namespace
{
const std::string kLogTestStr("Test ABC");
} // namespace

class ClientLogControlTest : public ::testing::Test
{
protected:
    std::shared_ptr<StrictMock<ClientLogHandlerMock>> m_clientLogHandlerMock;

    ClientLogControlTest() : m_clientLogHandlerMock{std::make_shared<StrictMock<ClientLogHandlerMock>>()} {}

    ~ClientLogControlTest() { resetLogHandler(m_clientLogHandlerMock); }

    void resetLogHandler(const std::shared_ptr<StrictMock<ClientLogHandlerMock>> &clientLogHandlerMock)
    {
        EXPECT_CALL(*clientLogHandlerMock, log(_, _, _, _, _)).Times(AnyNumber());
        IClientLogControl &control = ClientLogControlFactory::createFactory()->createClientLogControl();
        EXPECT_TRUE(control.registerLogHandler(nullptr, false));
    }
};

TEST_F(ClientLogControlTest, RegisterLogHandler)
{
    {
        IClientLogControl &control = ClientLogControlFactory::createFactory()->createClientLogControl();
        EXPECT_TRUE(control.registerLogHandler(m_clientLogHandlerMock, true));
    }

    // Generate a log entry
    EXPECT_CALL(*m_clientLogHandlerMock, log(IClientLogHandler::Level::Error, StrNe(""), Ne(0), StrNe(""), StrNe("")));
    RIALTO_CLIENT_LOG_ERROR("%s", kLogTestStr.c_str());
}

TEST_F(ClientLogControlTest, ShouldUpdateLogHandler)
{
    IClientLogControl &control1 = IClientLogControlFactory::createFactory()->createClientLogControl();
    EXPECT_TRUE(control1.registerLogHandler(m_clientLogHandlerMock, true));

    // Generate a log entry
    EXPECT_CALL(*m_clientLogHandlerMock, log(IClientLogHandler::Level::Error, StrNe(""), Ne(0), StrNe(""), StrNe("")));
    RIALTO_CLIENT_LOG_ERROR("%s", kLogTestStr.c_str());

    IClientLogControl &control2 = IClientLogControlFactory::createFactory()->createClientLogControl();
    EXPECT_EQ(&control1, &control2); // IClientLogControl Ought to be a singleton

    // Replace log handler
    std::shared_ptr<StrictMock<ClientLogHandlerMock>> clientLogHandlerMock2 =
        std::make_shared<StrictMock<ClientLogHandlerMock>>();
    EXPECT_CALL(*m_clientLogHandlerMock, log(_, StrNe(""), Ne(0), StrNe(""), StrNe(""))).Times(2);
    EXPECT_TRUE(control2.registerLogHandler(clientLogHandlerMock2, true));

    // Generate a log entry
    EXPECT_CALL(*clientLogHandlerMock2, log(IClientLogHandler::Level::Error, StrNe(""), Ne(0), StrNe(""), StrNe("")));
    RIALTO_CLIENT_LOG_ERROR("%s", kLogTestStr.c_str());

    resetLogHandler(clientLogHandlerMock2);
}

TEST_F(ClientLogControlTest, ShouldCancelLogHandler)
{
    IClientLogControl &control = IClientLogControlFactory::createFactory()->createClientLogControl();
    EXPECT_TRUE(control.registerLogHandler(m_clientLogHandlerMock, true));

    // Cancel the log handler
    EXPECT_CALL(*m_clientLogHandlerMock, log(_, StrNe(""), Ne(0), StrNe(""), StrNe(""))).Times(1);
    EXPECT_TRUE(control.registerLogHandler(nullptr, false));

    // Generate a log entry
    // Log handler should not be used
    RIALTO_CLIENT_LOG_ERROR("%s", kLogTestStr.c_str());
}
