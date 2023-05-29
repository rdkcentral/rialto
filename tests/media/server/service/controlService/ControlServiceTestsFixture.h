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

#ifndef CONTROL_SERVICE_TESTS_FIXTURE_H_
#define CONTROL_SERVICE_TESTS_FIXTURE_H_

#include "ControlClientServerInternalMock.h"
#include "ControlService.h"
#include <gtest/gtest.h>
#include <memory>

using firebolt::rialto::ApplicationState;
using firebolt::rialto::server::ControlClientServerInternalMock;
using firebolt::rialto::server::service::ControlService;
using testing::StrictMock;

class ControlServiceTests : public testing::Test
{
public:
    ControlServiceTests();
    ~ControlServiceTests() override = default;

    void controlClientServerInternalWillAck();
    void controlClientServerInternalWillNotifyApplicationState(const firebolt::rialto::ApplicationState &appState);

    void triggerAddControl(int id);
    void triggerRemoveControl(int id);
    bool triggerAck(int id);
    void triggerSetApplicationState(const firebolt::rialto::ApplicationState &appState);

private:
    std::shared_ptr<StrictMock<ControlClientServerInternalMock>> m_controlClientMock;
    ControlService m_sut;
};

#endif // CONTROL_SERVICE_TESTS_FIXTURE_H_
