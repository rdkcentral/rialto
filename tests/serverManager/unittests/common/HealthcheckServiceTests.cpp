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

#include "HealthcheckService.h"
#include "SessionServerAppManagerMock.h"
#include "TimerFactoryMock.h"
#include "TimerMock.h"
#include <gtest/gtest.h>

using firebolt::rialto::common::SessionServerState;
using firebolt::rialto::server::TimerFactoryMock;
using firebolt::rialto::server::TimerMock;
using rialto::servermanager::common::HealthcheckService;
using rialto::servermanager::common::SessionServerAppManagerMock;
using testing::_;
using testing::ByMove;
using testing::DoAll;
using testing::Return;
using testing::SaveArg;
using testing::StrictMock;

namespace
{
constexpr std::chrono::seconds kHealthcheckFrequency{1};
constexpr int kServerId{12};
constexpr bool kSuccess{true};
constexpr bool kFailure{false};
} // namespace

class HealthcheckServiceTests : public testing::Test
{
public:
    HealthcheckServiceTests() = default;
    ~HealthcheckServiceTests() override = default;

    void timerWillBeCreated()
    {
        EXPECT_CALL(*m_timerMock, isActive()).WillOnce(Return(true));
        EXPECT_CALL(*m_timerMock, cancel());
        EXPECT_CALL(*m_timerFactoryMock, createTimer(static_cast<std::chrono::milliseconds>(kHealthcheckFrequency), _,
                                                     firebolt::rialto::common::TimerType::PERIODIC))
            .WillOnce(DoAll(SaveArg<1>(&m_timerCallback), Return(ByMove(std::move(m_timerMock)))));
    }

    void pingWillBeSent(int &pingId)
    {
        EXPECT_CALL(m_sessionServerAppManagerMock, sendPingEvents(_)).WillOnce(SaveArg<0>(&pingId));
    }

    void errorIndicationWillBeSent()
    {
        EXPECT_CALL(m_sessionServerAppManagerMock, onSessionServerStateChanged(kServerId, SessionServerState::ERROR));
    }

    void createSut(std::chrono::seconds healthcheckInterval = kHealthcheckFrequency)
    {
        m_sut = std::make_unique<HealthcheckService>(m_sessionServerAppManagerMock, m_timerFactoryMock,
                                                     healthcheckInterval);
    }

    void triggerPingTimeout()
    {
        ASSERT_TRUE(m_timerCallback);
        m_timerCallback();
    }

    void triggerOnPingSent(int pingId) { m_sut->onPingSent(kServerId, pingId); }

    void triggerOnServerRemoved() { m_sut->onServerRemoved(kServerId); }

    void triggerOnAckReceived(int serverId, int pingId, bool success)
    {
        m_sut->onAckReceived(serverId, pingId, success);
    }

private:
    StrictMock<SessionServerAppManagerMock> m_sessionServerAppManagerMock;
    std::shared_ptr<StrictMock<TimerFactoryMock>> m_timerFactoryMock{std::make_shared<StrictMock<TimerFactoryMock>>()};
    std::unique_ptr<StrictMock<TimerMock>> m_timerMock{std::make_unique<StrictMock<TimerMock>>()};
    std::function<void()> m_timerCallback;
    std::unique_ptr<HealthcheckService> m_sut;
};

TEST_F(HealthcheckServiceTests, WillCreateSutWithPingDisabled)
{
    createSut(std::chrono::seconds(0));
}

TEST_F(HealthcheckServiceTests, WillCreateSutWithPingEnabled)
{
    timerWillBeCreated();
    createSut();
}

TEST_F(HealthcheckServiceTests, WillRequestToSendPingMessages)
{
    int pingId{-1};
    timerWillBeCreated();
    createSut();
    pingWillBeSent(pingId);
    triggerPingTimeout();
}

TEST_F(HealthcheckServiceTests, WillFailToAddPingWithWrongId)
{
    int pingId{-1};
    timerWillBeCreated();
    createSut();
    pingWillBeSent(pingId);
    triggerPingTimeout();
    triggerOnPingSent(pingId + 1);
    // There should be no error indication here.
    pingWillBeSent(pingId);
    triggerPingTimeout();
}

TEST_F(HealthcheckServiceTests, WillSendErrorDueToTimeoutedPing)
{
    int pingId{-1};
    timerWillBeCreated();
    createSut();
    pingWillBeSent(pingId);
    triggerPingTimeout();
    triggerOnPingSent(pingId);
    errorIndicationWillBeSent();
    pingWillBeSent(pingId);
    triggerPingTimeout();
}

TEST_F(HealthcheckServiceTests, WillNotSendErrorWhenServerIsRemoved)
{
    int pingId{-1};
    timerWillBeCreated();
    createSut();
    pingWillBeSent(pingId);
    triggerPingTimeout();
    triggerOnPingSent(pingId);
    triggerOnServerRemoved();
    // There should be no error indication here.
    pingWillBeSent(pingId);
    triggerPingTimeout();
}

TEST_F(HealthcheckServiceTests, WillFailToAckWhenPingIdIsWrong)
{
    int pingId{-1};
    timerWillBeCreated();
    createSut();
    pingWillBeSent(pingId);
    triggerPingTimeout();
    triggerOnPingSent(pingId);
    triggerOnAckReceived(kServerId, pingId + 1, kSuccess);
    errorIndicationWillBeSent();
    pingWillBeSent(pingId);
    triggerPingTimeout();
}

TEST_F(HealthcheckServiceTests, WillFailWhenAckIsReceivedForOtherServer)
{
    int pingId{-1};
    timerWillBeCreated();
    createSut();
    pingWillBeSent(pingId);
    triggerPingTimeout();
    triggerOnPingSent(pingId);
    triggerOnAckReceived(kServerId + 1, pingId, kSuccess);
    errorIndicationWillBeSent();
    pingWillBeSent(pingId);
    triggerPingTimeout();
}

TEST_F(HealthcheckServiceTests, WillFailWhenFailAckIsReceived)
{
    int pingId{-1};
    timerWillBeCreated();
    createSut();
    pingWillBeSent(pingId);
    triggerPingTimeout();
    triggerOnPingSent(pingId);
    errorIndicationWillBeSent();
    triggerOnAckReceived(kServerId, pingId, kFailure);
    // There should be no error indication here.
    pingWillBeSent(pingId);
    triggerPingTimeout();
}

TEST_F(HealthcheckServiceTests, WillPingAndAckSuccessfully)
{
    int pingId{-1};
    timerWillBeCreated();
    createSut();
    pingWillBeSent(pingId);
    triggerPingTimeout();
    triggerOnPingSent(pingId);
    triggerOnAckReceived(kServerId, pingId, kSuccess);
    // There should be no error indication here.
    pingWillBeSent(pingId);
    triggerPingTimeout();
}
