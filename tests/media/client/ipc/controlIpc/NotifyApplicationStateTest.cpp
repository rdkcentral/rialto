/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Sky UK
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

#include "ControlIpcTestBase.h"
#include "controlmodule.pb.h"

using testing::_;
using testing::Invoke;

class ControlIpcNotifyApplicationStateTest : public ControlIpcTestBase
{
protected:
    ControlIpcNotifyApplicationStateTest()
    {
        createControlIpc();
        expectIpcApiCallSuccess();
        registerClient();
        EXPECT_CALL(*m_eventThreadMock, addImpl(_)).WillOnce(Invoke([](std::function<void()> &&func) { func(); }));
    }

    ~ControlIpcNotifyApplicationStateTest() override { destroyControlIpc(); }

    std::shared_ptr<firebolt::rialto::ApplicationStateChangeEvent>
    createEvent(const firebolt::rialto::ApplicationStateChangeEvent_ApplicationState &state)
    {
        auto appStateChangeEvent = std::make_shared<firebolt::rialto::ApplicationStateChangeEvent>();
        appStateChangeEvent->set_control_handle(m_kHandleId);
        appStateChangeEvent->set_application_state(state);
        return appStateChangeEvent;
    }
};

TEST_F(ControlIpcNotifyApplicationStateTest, shouldNotifyAboutChangeToRunning)
{
    ASSERT_TRUE(m_notifyApplicationStateCb);
    EXPECT_CALL(m_controlClientMock, notifyApplicationState(firebolt::rialto::ApplicationState::RUNNING));
    m_notifyApplicationStateCb(createEvent(firebolt::rialto::ApplicationStateChangeEvent_ApplicationState_RUNNING));
}

TEST_F(ControlIpcNotifyApplicationStateTest, shouldNotifyAboutChangeToInactive)
{
    ASSERT_TRUE(m_notifyApplicationStateCb);
    EXPECT_CALL(m_controlClientMock, notifyApplicationState(firebolt::rialto::ApplicationState::INACTIVE));
    m_notifyApplicationStateCb(createEvent(firebolt::rialto::ApplicationStateChangeEvent_ApplicationState_INACTIVE));
}

TEST_F(ControlIpcNotifyApplicationStateTest, shouldNotifyAboutChangeToUnknown)
{
    ASSERT_TRUE(m_notifyApplicationStateCb);
    EXPECT_CALL(m_controlClientMock, notifyApplicationState(firebolt::rialto::ApplicationState::UNKNOWN));
    m_notifyApplicationStateCb(createEvent(firebolt::rialto::ApplicationStateChangeEvent_ApplicationState_UNKNOWN));
}

TEST_F(ControlIpcNotifyApplicationStateTest, wrongHandleId)
{
    auto event = createEvent(firebolt::rialto::ApplicationStateChangeEvent_ApplicationState_UNKNOWN);
    event->set_control_handle(1234);

    m_notifyApplicationStateCb(event);
}
