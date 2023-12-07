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

#include "ActionTraits.h"
#include "ConfigureAction.h"
#include "ExpectMessage.h"
#include "MessageBuilders.h"
#include "RialtoServerComponentTest.h"

using namespace firebolt::rialto::server::ct;

class SessionServerStateChangeTest : public RialtoServerComponentTest
{
public:
    SessionServerStateChangeTest() = default;
    ~SessionServerStateChangeTest() override = default;

    void configureSutInInactiveState()
    {
        ::rialto::SetConfigurationRequest request{createGenericSetConfigurationReq()};
        request.set_initialsessionserverstate(::rialto::SessionServerState::INACTIVE);

        ExpectMessage<::rialto::StateChangedEvent> expectedMessage(m_serverManagerStub);

        ConfigureAction<SetConfiguration>(m_serverManagerStub).send(request).expectSuccess();

        auto receivedMessage = expectedMessage.getMessage();
        ASSERT_TRUE(receivedMessage);
        EXPECT_EQ(receivedMessage->sessionserverstate(), ::rialto::SessionServerState::INACTIVE);
    }

    void setStateActive()
    {
        ::rialto::SetStateRequest request{createSetStateRequest(::rialto::SessionServerState::ACTIVE)};

        ExpectMessage<::rialto::StateChangedEvent> expectedMessage(m_serverManagerStub);

        ConfigureAction<::firebolt::rialto::server::ct::SetStateRequest>(m_serverManagerStub).send(request).expectSuccess();

        auto receivedMessage = expectedMessage.getMessage();
        ASSERT_TRUE(receivedMessage);
        EXPECT_EQ(receivedMessage->sessionserverstate(), ::rialto::SessionServerState::ACTIVE);
    }

    void setStateInactive()
    {
        ::rialto::SetStateRequest request{createSetStateRequest(::rialto::SessionServerState::INACTIVE)};

        ExpectMessage<::rialto::StateChangedEvent> expectedMessage(m_serverManagerStub);

        ConfigureAction<::firebolt::rialto::server::ct::SetStateRequest>(m_serverManagerStub).send(request).expectSuccess();

        auto receivedMessage = expectedMessage.getMessage();
        ASSERT_TRUE(receivedMessage);
        EXPECT_EQ(receivedMessage->sessionserverstate(), ::rialto::SessionServerState::INACTIVE);
    }

    void setLogLevels()
    {
        ::rialto::SetLogLevelsRequest request{createSetLogLevelsRequest()};

        ConfigureAction<::firebolt::rialto::server::ct::SetLogLevelsRequest>(m_serverManagerStub)
            .send(request)
            .expectSuccess();
    }
};

TEST_F(SessionServerStateChangeTest, ShouldConfigureInInactiveState)
{
    willConfigureSocket();
    configureSutInInactiveState();
}

TEST_F(SessionServerStateChangeTest, ShouldConfigureInActiveState)
{
    willConfigureSocket();
    configureSutInActiveState();
}

TEST_F(SessionServerStateChangeTest, ShouldChangeFromInactiveToActive)
{
    willConfigureSocket();
    configureSutInInactiveState();
    setStateActive();
}

TEST_F(SessionServerStateChangeTest, ShouldChangeFromActiveToInactive)
{
    willConfigureSocket();
    configureSutInActiveState();
    setStateInactive();
}

TEST_F(SessionServerStateChangeTest, ShouldSetLogLevels)
{
    willConfigureSocket();
    configureSutInInactiveState();
    setLogLevels();
}
