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

#include "IControl.h"
#include "ServerStub.h"
#include "ControlModuleMock.h"
#include "ControlClientMock.h"
#include <gtest/gtest.h>
#include <condition_variable>
#include <mutex>

using namespace firebolt::rialto;
using namespace firebolt::rialto::ipc;

using ::testing::_;
using ::testing::ByMove;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::StrictMock;
using ::testing::WithArgs;
using ::testing::DoAll;

namespace
{
constexpr std::chrono::milliseconds kEventTimeout{200};
}

class ApplicationStateChangeTest : public ::testing::Test
{
protected:
    std::shared_ptr<StrictMock<ControlClientMock>> m_controlClientMock;
    std::shared_ptr<StrictMock<ControlModuleMock>> m_controlModuleMock;
    std::shared_ptr<ServerStub> m_serverStub;

    std::mutex m_eventsLock;
    std::condition_variable m_eventsCond;
    bool m_eventReceived {false};

    ApplicationStateChangeTest()
    {
        m_controlClientMock = std::make_shared<StrictMock<ControlClientMock>>();
        m_controlModuleMock = std::make_shared<StrictMock<ControlModuleMock>>();

        m_serverStub = std::make_shared<ServerStub>(m_controlModuleMock);
    }

    ~ApplicationStateChangeTest(){}
};

/**
 * Test that IPC can send a request with a single variable.
 */
TEST_F(ApplicationStateChangeTest, test)
{
    // Init control
    std::shared_ptr<firebolt::rialto::IControlFactory> factory = firebolt::rialto::IControlFactory::createFactory();
    std::shared_ptr<IControl> control = factory->createControl();
    EXPECT_NE(control, nullptr);

    constexpr int32_t kControlId = 1;
    const firebolt::rialto::common::SchemaVersion kSchemaVersion = firebolt::rialto::common::getCurrentSchemaVersion();
    EXPECT_CALL(*m_controlModuleMock, registerClient(_, _, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_controlModuleMock->getRegisterClientResponse(kControlId, kSchemaVersion)),
                        WithArgs<0, 3>(Invoke(&(*m_controlModuleMock), &ControlModuleMock::defaultReturn))));

    ApplicationState appState;
    EXPECT_TRUE(control->registerClient(m_controlClientMock, appState));
    EXPECT_EQ(ApplicationState::UNKNOWN, appState);

    // State changed to INACTIVE
    EXPECT_CALL(*m_controlClientMock, notifyApplicationState(ApplicationState::INACTIVE))
        .WillOnce(Invoke([this](ApplicationState state)
            {
                std::unique_lock<std::mutex> locker(m_eventsLock);
                m_eventReceived = true;
                m_eventsCond.notify_all();
            }));
    m_serverStub->notifyApplicationStateEvent(kControlId, ApplicationState::INACTIVE);

    {
        std::unique_lock<std::mutex> locker(m_eventsLock);
        if (!m_eventReceived)
        {
            m_eventsCond.wait_for(locker, kEventTimeout);
        }
    }

    // State changed to RUNNING
    //m_serverStub->notifyApplicationStateEvent(kControlId, ApplicationState::RUNNING);

    // State changed to INACTIVE
    //m_serverStub->notifyApplicationStateEvent(kControlId, ApplicationState::INACTIVE);
}
