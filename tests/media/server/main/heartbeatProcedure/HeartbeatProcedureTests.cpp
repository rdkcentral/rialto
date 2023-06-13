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

#include "AckSenderMock.h"
#include "IHeartbeatProcedure.h"
#include <gtest/gtest.h>

using firebolt::rialto::server::AckSenderMock;
using firebolt::rialto::server::IHeartbeatProcedureFactory;
using testing::StrictMock;
using testing::Test;

namespace
{
constexpr std::int32_t kPingId{13};
constexpr int kFirstControlId{0};
constexpr int kSecondControlId{1};
} // namespace

class HeartbeatProcedureTests : public Test
{
public:
    HeartbeatProcedureTests() = default;
    ~HeartbeatProcedureTests() override = default;

    std::shared_ptr<StrictMock<AckSenderMock>> m_ackSenderMock{std::make_shared<StrictMock<AckSenderMock>>()};
};

TEST_F(HeartbeatProcedureTests, shouldSendSuccessResponseWhenNoHandlerIsCreated)
{
    auto sut{IHeartbeatProcedureFactory::createFactory()->createHeartbeatProcedure(m_ackSenderMock, kPingId)};
    EXPECT_CALL(*m_ackSenderMock, send(kPingId, true));
    sut.reset();
}

TEST_F(HeartbeatProcedureTests, shouldSendSuccessResponseWhenSingleHandlerSucceeds)
{
    auto sut{IHeartbeatProcedureFactory::createFactory()->createHeartbeatProcedure(m_ackSenderMock, kPingId)};
    auto handler = sut->createHandler(kFirstControlId);
    handler.reset();
    EXPECT_CALL(*m_ackSenderMock, send(kPingId, true));
    sut.reset();
}

TEST_F(HeartbeatProcedureTests, shouldSendFailResponseWhenSingleHandlerFails)
{
    auto sut{IHeartbeatProcedureFactory::createFactory()->createHeartbeatProcedure(m_ackSenderMock, kPingId)};
    auto handler = sut->createHandler(kFirstControlId);
    handler->error();
    handler.reset();
    EXPECT_CALL(*m_ackSenderMock, send(kPingId, false));
    sut.reset();
}

TEST_F(HeartbeatProcedureTests, handlerShouldReturnCorrectPingId)
{
    auto sut{IHeartbeatProcedureFactory::createFactory()->createHeartbeatProcedure(m_ackSenderMock, kPingId)};
    auto handler = sut->createHandler(kFirstControlId);
    EXPECT_EQ(handler->id(), kPingId);
    handler.reset();
    EXPECT_CALL(*m_ackSenderMock, send(kPingId, true));
    sut.reset();
}

TEST_F(HeartbeatProcedureTests, shouldSendSuccessResponseWhenTwoHandlersSucceed)
{
    auto sut{IHeartbeatProcedureFactory::createFactory()->createHeartbeatProcedure(m_ackSenderMock, kPingId)};
    auto handler1 = sut->createHandler(kFirstControlId);
    auto handler2 = sut->createHandler(kSecondControlId);
    handler1.reset();
    handler2.reset();
    EXPECT_CALL(*m_ackSenderMock, send(kPingId, true));
    sut.reset();
}

TEST_F(HeartbeatProcedureTests, shouldSendSuccessResponseWhenOneOfHandlersFail)
{
    auto sut{IHeartbeatProcedureFactory::createFactory()->createHeartbeatProcedure(m_ackSenderMock, kPingId)};
    auto handler1 = sut->createHandler(kFirstControlId);
    auto handler2 = sut->createHandler(kSecondControlId);
    handler1->error();
    handler1.reset();
    handler2.reset();
    EXPECT_CALL(*m_ackSenderMock, send(kPingId, false));
    sut.reset();
}
