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

#ifndef CONTROL_TEST_METHODS_H_
#define CONTROL_TEST_METHODS_H_

#include "ControlClientMock.h"
#include "ControlModuleMock.h"
#include "ServerStub.h"
#include "IControl.h"
#include <gtest/gtest.h>

using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::StrictMock;
using ::testing::WithArgs;

using namespace firebolt::rialto;
using namespace firebolt::rialto::componenttest::mock;
using namespace firebolt::rialto::componenttest::stub;

class ControlTestMethods
{
public:
    ControlTestMethods();
    virtual ~ControlTestMethods();

protected:
    // Strict Mocks
    std::shared_ptr<StrictMock<ControlClientMock>> m_controlClientMock;
    std::shared_ptr<StrictMock<ControlModuleMock>> m_controlModuleMock;

    // Objects
    std::shared_ptr<IControlFactory> m_controlFactory;
    std::shared_ptr<IControl> m_control;

    void createControl();
    void shouldRegisterClient();
    void registerClient();
    void shouldNotifyApplicationStateInactive();
    void sendNotifyApplicationStateInactive();

    // Component test helper
    virtual void notifyEvent() = 0;
    virtual void waitEvent() = 0;
    virtual std::shared_ptr<ServerStub>& getServerStub() = 0;
};

#endif // CONTROL_TEST_METHODS_H_
