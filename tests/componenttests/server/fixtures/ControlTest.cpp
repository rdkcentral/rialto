/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Sky UK
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
 *
 */

#include "ControlTest.h"
#include "ActionTraits.h"
#include "ConfigureAction.h"
#include "ExpectMessage.h"
#include "MessageBuilders.h"

namespace firebolt::rialto::server::ct
{
ControlTest::ControlTest()
{
    willConfigureSocket();
    configureSutInActiveState();
    connectClient();
    registerClient();
}

void ControlTest::registerClient()
{
    ExpectMessage<ApplicationStateChangeEvent> expectedAppStateChange{m_clientStub};

    auto registerClientReq(createRegisterClientRequest());
    ConfigureAction<RegisterClient>(m_clientStub)
        .send(registerClientReq)
        .expectSuccess()
        .matchResponse([&](const auto &resp) { m_controlHandle = resp.control_handle(); });

    auto receivedMessage = expectedAppStateChange.getMessage();
    ASSERT_TRUE(receivedMessage);
    EXPECT_EQ(receivedMessage->application_state(), ApplicationStateChangeEvent_ApplicationState_RUNNING);
}
} // namespace firebolt::rialto::server::ct
