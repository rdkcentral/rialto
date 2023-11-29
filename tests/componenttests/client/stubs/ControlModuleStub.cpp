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

#include "ServerStub.h"
#include <IIpcServer.h>
#include <IIpcServerFactory.h>
#include <gtest/gtest.h>
#include <sys/socket.h>
#include <unistd.h>

namespace
{
firebolt::rialto::ApplicationStateChangeEvent_ApplicationState
convertApplicationState(const firebolt::rialto::ApplicationState &state)
{
    switch (state)
    {
    case firebolt::rialto::ApplicationState::RUNNING:
        return firebolt::rialto::ApplicationStateChangeEvent_ApplicationState_RUNNING;
    case firebolt::rialto::ApplicationState::INACTIVE:
        return firebolt::rialto::ApplicationStateChangeEvent_ApplicationState_INACTIVE;
    case firebolt::rialto::ApplicationState::UNKNOWN:
        break;
        // do nothing
    }
    return firebolt::rialto::ApplicationStateChangeEvent_ApplicationState_UNKNOWN;
}
} // namespace

namespace firebolt::rialto::ct::stub
{
ControlModuleStub::ControlModuleStub(const std::shared_ptr<::firebolt::rialto::ControlModule>& controlModuleMock)
{
    m_controlModuleMock = controlModuleMock;
}

ControlModuleStub::~ControlModuleStub()
{
}

void ControlModuleStub::notifyApplicationStateEvent(const int32_t controlId, const firebolt::rialto::ApplicationState &state)
{
    waitForClientConnect();

    auto event = std::make_shared<firebolt::rialto::ApplicationStateChangeEvent>();
    event->set_control_handle(controlId);
    event->set_application_state(convertApplicationState(state));

    getClient()->sendEvent(event);
}

} // namespace firebolt::rialto::ct
