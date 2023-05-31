/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Sky UK
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

#include "ControlClientServerInternalMock.h"
#include "ControlServerInternal.h"
#include "HeartbeatHandlerMock.h"
#include "MainThreadFactoryMock.h"
#include "MainThreadMock.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto::server::mock;
using firebolt::rialto::server::ControlClientServerInternalMock;
using firebolt::rialto::server::HeartbeatHandlerMock;
using testing::_;
using testing::Invoke;
using testing::Return;
using testing::StrictMock;
using testing::Test;

namespace
{
constexpr int kControlId{8};
constexpr int kPingId{3};
constexpr int kNextPingId{4};
constexpr firebolt::rialto::ApplicationState kAppState{firebolt::rialto::ApplicationState::INACTIVE};
constexpr int kMainThreadClientId{123};
} // namespace

class ControlServerInternalTests : public Test
{
public:
    ControlServerInternalTests()
        : m_controlClientMock{std::make_shared<StrictMock<ControlClientServerInternalMock>>()},
          m_mainThreadFactoryMock{std::make_shared<StrictMock<MainThreadFactoryMock>>()},
          m_mainThreadMock{std::make_shared<StrictMock<MainThreadMock>>()}
    {
        EXPECT_CALL(*m_mainThreadFactoryMock, getMainThread()).WillOnce(Return(m_mainThreadMock));
        EXPECT_CALL(*m_mainThreadMock, registerClient()).WillOnce(Return(kMainThreadClientId));
        m_sut = std::make_unique<firebolt::rialto::server::ControlServerInternal>(m_mainThreadFactoryMock, kControlId,
                                                                                  m_controlClientMock);
    }

    ~ControlServerInternalTests()
    {
        EXPECT_CALL(*m_mainThreadMock, unregisterClient(kMainThreadClientId));
        m_sut.reset();
    }

    void mainThreadWillEnqueueTaskAndWait()
    {
        EXPECT_CALL(*m_mainThreadMock, enqueueTaskAndWait(kMainThreadClientId, _))
            .WillOnce(Invoke([](uint32_t clientId, firebolt::rialto::server::IMainThread::Task task) { task(); }))
            .RetiresOnSaturation();
    }

    std::shared_ptr<StrictMock<ControlClientServerInternalMock>> m_controlClientMock;
    std::shared_ptr<StrictMock<MainThreadFactoryMock>> m_mainThreadFactoryMock;
    std::shared_ptr<StrictMock<MainThreadMock>> m_mainThreadMock;
    std::unique_ptr<firebolt::rialto::server::ControlServerInternal> m_sut;
};

TEST_F(ControlServerInternalTests, shouldSendPingEvent)
{
    std::unique_ptr<StrictMock<HeartbeatHandlerMock>> heartbeatHandlerMock{
        std::make_unique<StrictMock<HeartbeatHandlerMock>>()};
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*heartbeatHandlerMock, id()).WillRepeatedly(Return(kPingId));
    EXPECT_CALL(*heartbeatHandlerMock, pingSent());
    EXPECT_CALL(*m_controlClientMock, ping(kPingId));
    m_sut->ping(std::move(heartbeatHandlerMock));
}

TEST_F(ControlServerInternalTests, shouldNotifyErrorWhenEarlierPingWasNotFinished)
{
    std::unique_ptr<StrictMock<HeartbeatHandlerMock>> heartbeatHandlerMock{
        std::make_unique<StrictMock<HeartbeatHandlerMock>>()};
    std::unique_ptr<StrictMock<HeartbeatHandlerMock>> heartbeatHandlerMock2{
        std::make_unique<StrictMock<HeartbeatHandlerMock>>()};
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*heartbeatHandlerMock, id()).WillRepeatedly(Return(kPingId));
    EXPECT_CALL(*heartbeatHandlerMock, pingSent());
    EXPECT_CALL(*m_controlClientMock, ping(kPingId));
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*heartbeatHandlerMock, error());
    EXPECT_CALL(*heartbeatHandlerMock2, id()).WillRepeatedly(Return(kNextPingId));
    EXPECT_CALL(*heartbeatHandlerMock2, pingSent());
    EXPECT_CALL(*m_controlClientMock, ping(kNextPingId));
    m_sut->ping(std::move(heartbeatHandlerMock));
    m_sut->ping(std::move(heartbeatHandlerMock2));
}

TEST_F(ControlServerInternalTests, shouldNotAckWhenHeartbeatHandlerIsNotPresent)
{
    mainThreadWillEnqueueTaskAndWait();
    m_sut->ack(kPingId);
}

TEST_F(ControlServerInternalTests, shouldNotAckWhenAckIdIsWrong)
{
    std::unique_ptr<StrictMock<HeartbeatHandlerMock>> heartbeatHandlerMock{
        std::make_unique<StrictMock<HeartbeatHandlerMock>>()};
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*heartbeatHandlerMock, id()).WillRepeatedly(Return(kNextPingId));
    EXPECT_CALL(*heartbeatHandlerMock, pingSent());
    EXPECT_CALL(*m_controlClientMock, ping(kNextPingId));
    m_sut->ping(std::move(heartbeatHandlerMock));

    mainThreadWillEnqueueTaskAndWait();
    m_sut->ack(kPingId);
}

TEST_F(ControlServerInternalTests, shouldAck)
{
    std::unique_ptr<StrictMock<HeartbeatHandlerMock>> heartbeatHandlerMock{
        std::make_unique<StrictMock<HeartbeatHandlerMock>>()};
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*heartbeatHandlerMock, id()).WillRepeatedly(Return(kPingId));
    EXPECT_CALL(*heartbeatHandlerMock, pingSent());
    EXPECT_CALL(*m_controlClientMock, ping(kPingId));
    m_sut->ping(std::move(heartbeatHandlerMock));

    mainThreadWillEnqueueTaskAndWait();
    m_sut->ack(kPingId);
}

TEST_F(ControlServerInternalTests, shouldAckAndSendNextPing)
{
    std::unique_ptr<StrictMock<HeartbeatHandlerMock>> heartbeatHandlerMock{
        std::make_unique<StrictMock<HeartbeatHandlerMock>>()};
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*heartbeatHandlerMock, id()).WillRepeatedly(Return(kPingId));
    EXPECT_CALL(*heartbeatHandlerMock, pingSent());
    EXPECT_CALL(*m_controlClientMock, ping(kPingId));
    m_sut->ping(std::move(heartbeatHandlerMock));

    mainThreadWillEnqueueTaskAndWait();
    m_sut->ack(kPingId);

    std::unique_ptr<StrictMock<HeartbeatHandlerMock>> heartbeatHandlerMock2{
        std::make_unique<StrictMock<HeartbeatHandlerMock>>()};
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*heartbeatHandlerMock2, id()).WillRepeatedly(Return(kNextPingId));
    EXPECT_CALL(*heartbeatHandlerMock2, pingSent());
    EXPECT_CALL(*m_controlClientMock, ping(kNextPingId));
    m_sut->ping(std::move(heartbeatHandlerMock2));

    mainThreadWillEnqueueTaskAndWait();
    m_sut->ack(kNextPingId);
}

TEST_F(ControlServerInternalTests, shouldSetApplicationState)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_controlClientMock, notifyApplicationState(kAppState));
    m_sut->setApplicationState(kAppState);
}

TEST_F(ControlServerInternalTests, shouldRegisterClient)
{
    firebolt::rialto::ApplicationState appState{firebolt::rialto::ApplicationState::INACTIVE};
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_controlClientMock, notifyApplicationState(kAppState));
    m_sut->registerClient(m_controlClientMock, appState);
}
