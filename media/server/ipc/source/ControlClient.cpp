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

#include "ControlClient.h"
#include "RialtoServerLogging.h"
#include "controlmodule.pb.h"
#include <IIpcServer.h>

namespace
{
firebolt::rialto::ApplicationStateChangeEvent_ApplicationState
convertApplicationState(const firebolt::rialto::ApplicationState &state)
{
    switch (state)
    {
    case firebolt::rialto::ApplicationState::UNKNOWN:
        return firebolt::rialto::ApplicationStateChangeEvent_ApplicationState_UNKNOWN;
    case firebolt::rialto::ApplicationState::RUNNING:
        return firebolt::rialto::ApplicationStateChangeEvent_ApplicationState_RUNNING;
    case firebolt::rialto::ApplicationState::INACTIVE:
        return firebolt::rialto::ApplicationStateChangeEvent_ApplicationState_INACTIVE;
    }
    return firebolt::rialto::ApplicationStateChangeEvent_ApplicationState_UNKNOWN;
}
} // namespace

namespace firebolt::rialto::server::ipc
{
ControlClient::ControlClient(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient)
    : m_ipcClient{ipcClient}
{
}

void ControlClient::notifyApplicationState(ApplicationState state)
{
    RIALTO_SERVER_LOG_DEBUG("Sending ApplicationStateChangeEvent");

    auto event = std::make_shared<firebolt::rialto::ApplicationStateChangeEvent>();
    event->set_application_state(convertApplicationState(state));

    m_ipcClient->sendEvent(event);
}

void ControlClient::ping(uint32_t id)
{
    RIALTO_SERVER_LOG_WARN("Not implemented");
}
} // namespace firebolt::rialto::server::ipc
