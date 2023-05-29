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

#include "ControlServiceTestsFixture.h"

using testing::_;
using testing::Return;

namespace
{
constexpr int kPingId{7};
} // namespace

ControlServiceTests::ControlServiceTests()
    : m_controlClientMock{std::make_shared<StrictMock<ControlClientServerInternalMock>>()}, m_sut{}
{
}

void ControlServiceTests::controlClientServerInternalWillAck() {}

void ControlServiceTests::controlClientServerInternalWillNotifyApplicationState(
    const firebolt::rialto::ApplicationState &appState)
{
    EXPECT_CALL(*m_controlClientMock, notifyApplicationState(appState));
}

void ControlServiceTests::triggerAddControl(int id)
{
    m_sut.addControl(id, m_controlClientMock);
}

void ControlServiceTests::triggerRemoveControl(int id)
{
    m_sut.removeControl(id);
}

bool ControlServiceTests::triggerAck(int id)
{
    return m_sut.ack(id, kPingId);
}

void ControlServiceTests::triggerSetApplicationState(const firebolt::rialto::ApplicationState &appState)
{
    m_sut.setApplicationState(appState);
}
