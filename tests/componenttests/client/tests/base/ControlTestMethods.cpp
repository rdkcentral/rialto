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

#include "ControlTestMethods.h"
#include <memory>

namespace
{
constexpr int32_t kControlId = 1;
} // namespace

ControlTestMethods::ControlTestMethods()
    : m_controlClientMock{std::make_shared<StrictMock<ControlClientMock>>()},
      m_controlModuleMock{std::make_shared<StrictMock<ControlModuleMock>>()}
{
}

ControlTestMethods::~ControlTestMethods()
{
}

void ControlTestMethods::createControl()
{
    m_controlFactory = firebolt::rialto::IControlFactory::createFactory();
    m_control = m_controlFactory->createControl();
    EXPECT_NE(m_control, nullptr);
}

void ControlTestMethods::shouldRegisterClient()
{
    const firebolt::rialto::common::SchemaVersion kSchemaVersion = firebolt::rialto::common::getCurrentSchemaVersion();
    EXPECT_CALL(*m_controlModuleMock, registerClient(_, _, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_controlModuleMock->getRegisterClientResponse(kControlId, kSchemaVersion)),
                        WithArgs<0, 3>(Invoke(&(*m_controlModuleMock), &ControlModuleMock::defaultReturn))));
}

void ControlTestMethods::registerClient()
{
    ApplicationState appState;
    EXPECT_TRUE(m_control->registerClient(m_controlClientMock, appState));
    EXPECT_EQ(ApplicationState::UNKNOWN, appState);
}

void ControlTestMethods::shouldNotifyApplicationStateInactive()
{
    EXPECT_CALL(*m_controlClientMock, notifyApplicationState(ApplicationState::INACTIVE))
        .WillOnce(Invoke(this, &ControlTestMethods::notifyEvent));
}

void ControlTestMethods::sendNotifyApplicationStateInactive()
{
    getServerStub()->notifyApplicationStateEvent(kControlId, ApplicationState::INACTIVE);
    waitEvent();
}

void ControlTestMethods::shouldNotifyApplicationStateUnknown()
{
    EXPECT_CALL(*m_controlClientMock, notifyApplicationState(ApplicationState::UNKNOWN))
        .WillOnce(Invoke(this, &ControlTestMethods::notifyEvent));
}

void ControlTestMethods::shouldNotifyApplicationStateRunning()
{
    EXPECT_CALL(*m_controlClientMock, notifyApplicationState(ApplicationState::RUNNING))
        .WillOnce(Invoke(this, &ControlTestMethods::notifyEvent));
    EXPECT_CALL(*m_controlModuleMock, getSharedMemory(_, _, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_controlModuleMock->getSharedMemoryResponse(getShmFd(), getShmSize())),
                        WithArgs<0, 3>(Invoke(&(*m_controlModuleMock), &ControlModuleMock::defaultReturn))));
}

void ControlTestMethods::sendNotifyApplicationStateRunning()
{
    getServerStub()->notifyApplicationStateEvent(kControlId, ApplicationState::RUNNING);
    waitEvent();
}
