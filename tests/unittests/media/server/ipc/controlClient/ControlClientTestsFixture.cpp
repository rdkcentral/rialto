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

#include "ControlClientTestsFixture.h"
#include <memory>

namespace
{
constexpr int kControlId{23};
constexpr int kPingId{56};
} // namespace

MATCHER_P(ApplicationStateChangeEventMatcher, state, "")
{
    std::shared_ptr<firebolt::rialto::ApplicationStateChangeEvent> event =
        std::dynamic_pointer_cast<firebolt::rialto::ApplicationStateChangeEvent>(arg);
    return (kControlId == event->control_handle() && state == event->application_state());
}

MATCHER_P(PingEventMatcher, id, "")
{
    std::shared_ptr<firebolt::rialto::PingEvent> event = std::dynamic_pointer_cast<firebolt::rialto::PingEvent>(arg);
    return (kControlId == event->control_handle() && id == event->id());
}

ControlClientTests::ControlClientTests()
    : m_clientMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ClientMock>>()}, m_sut{kControlId, m_clientMock}
{
}

void ControlClientTests::clientWillSendApplicationStateEvent(
    firebolt::rialto::ApplicationStateChangeEvent_ApplicationState state)
{
    EXPECT_CALL(*m_clientMock, sendEvent(ApplicationStateChangeEventMatcher(state)));
}

void ControlClientTests::clientWillSendPingEvent()
{
    EXPECT_CALL(*m_clientMock, sendEvent(PingEventMatcher(kPingId)));
}

void ControlClientTests::triggerNotifyApplicationState(firebolt::rialto::ApplicationState state)
{
    m_sut.notifyApplicationState(state);
}

void ControlClientTests::triggerPing()
{
    m_sut.ping(kPingId);
}
