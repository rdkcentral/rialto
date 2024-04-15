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

#ifndef CONTROL_CLIENT_TESTS_FIXTURE_H_
#define CONTROL_CLIENT_TESTS_FIXTURE_H_

#include "ClientMock.h"
#include "ControlClientServerInternal.h"
#include "controlmodule.pb.h"
#include <gtest/gtest.h>
#include <memory>

using testing::StrictMock;
using testing::Test;

class ControlClientTests : public Test
{
public:
    ControlClientTests();
    ~ControlClientTests() override = default;

    void clientWillSendApplicationStateEvent(firebolt::rialto::ApplicationStateChangeEvent_ApplicationState state);
    void clientWillSendPingEvent();

    void triggerNotifyApplicationState(firebolt::rialto::ApplicationState state);
    void triggerPing();

private:
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::ClientMock>> m_clientMock;
    firebolt::rialto::server::ipc::ControlClientServerInternal m_sut;
};

#endif // CONTROL_CLIENT_TESTS_FIXTURE_H_
