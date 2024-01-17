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

#include <gtest/gtest.h>

#include "ClientControllerMock.h"
#include "Control.h"
#include "ControlClientMock.h"
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
constexpr uint32_t kMaxTimeoutMs{2000};
const std::string kLogTestStr("Test ABC");
} // namespace

class RialtoClientControlTest : public ::testing::Test
{
protected:
    std::shared_ptr<StrictMock<ControlClientMock>> m_controlClientMock;
    StrictMock<ClientControllerMock> m_clientControllerMock;
};

/**
 * Test the factory
 */
TEST_F(RialtoClientControlTest, FactoryFails)
{
    std::shared_ptr<firebolt::rialto::IControlFactory> factory = firebolt::rialto::IControlFactory::createFactory();
    EXPECT_NE(factory, nullptr);
    EXPECT_EQ(factory->createControl(), nullptr);
}

TEST_F(RialtoClientControlTest, CreateDestroy)
{
    std::unique_ptr<IControl> control;

    // Create
    EXPECT_NO_THROW(control = std::make_unique<Control>(m_clientControllerMock));

    // Destroy
    control.reset();
}

TEST_F(RialtoClientControlTest, RegisterAndUnregisterClient)
{
    m_controlClientMock = std::make_shared<StrictMock<ControlClientMock>>();
    ApplicationState appState;
    constexpr ApplicationState kExpectedAppState{ApplicationState::RUNNING};
    std::unique_ptr<IControl> control;

    EXPECT_NO_THROW(control = std::make_unique<Control>(m_clientControllerMock));

    EXPECT_CALL(m_clientControllerMock, registerClient(m_controlClientMock.get(), _))
        .WillOnce(DoAll(SetArgReferee<1>(kExpectedAppState), Return(true)));
    control->registerClient(m_controlClientMock, appState);
    EXPECT_EQ(appState, kExpectedAppState);

    EXPECT_CALL(m_clientControllerMock, unregisterClient(m_controlClientMock.get())).WillOnce(Return(true));

    // Destroy
    control.reset();
}

TEST_F(RialtoClientControlTest, RegisterClientFailDueToNullControlClient)
{
    ApplicationState appState;
    std::unique_ptr<IControl> control;

    EXPECT_NO_THROW(control = std::make_unique<Control>(m_clientControllerMock));

    control->registerClient(m_controlClientMock, appState);

    // Destroy
    control.reset();
}

TEST_F(RialtoClientControlTest, RegisterClientFailureDueToOperationFailure)
{
    m_controlClientMock = std::make_shared<StrictMock<ControlClientMock>>();
    ApplicationState appState;
    std::unique_ptr<IControl> control;

    EXPECT_NO_THROW(control = std::make_unique<Control>(m_clientControllerMock));

    EXPECT_CALL(m_clientControllerMock, registerClient(m_controlClientMock.get(), _)).WillOnce(Return(false));
    control->registerClient(m_controlClientMock, appState);

    // Destroy
    control.reset();
}

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

TEST_F(RialtoClientControlTest, RegisterLogHandler)
{
    std::unique_ptr<IControl> control;

    EXPECT_NO_THROW(control = std::make_unique<Control>(m_clientControllerMock));

    std::shared_ptr<TestClientLogHandler> logHandler = std::make_shared<TestClientLogHandler>();
    {
        std::shared_ptr<IClientLogHandler> tmp = logHandler;
        control->registerLogHandler(tmp);
    }

    // Generate a log entry
    RIALTO_CLIENT_LOG_ERROR("%s", kLogTestStr.c_str());

    ASSERT_TRUE(logHandler->m_gotExpectedLogMessage);

    // Destroy
    control.reset();
}
