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

#include "ControlModuleServiceTestsFixture.h"
#include "ControlModuleService.h"
#include <string>
#include <vector>

using testing::_;
using testing::DoAll;
using testing::Invoke;
using testing::Return;
using testing::SetArgReferee;

namespace
{
constexpr int32_t kFd{123};
constexpr uint32_t kSize{456U};
constexpr int kControlId{8};
constexpr int kPingId{35};
} // namespace

namespace firebolt::rialto
{
} // namespace firebolt::rialto

ControlModuleServiceTests::ControlModuleServiceTests()
    : m_clientMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ClientMock>>()},
      m_serverMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ServerMock>>()},
      m_closureMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ClosureMock>>()},
      m_controllerMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ControllerMock>>()},
      m_invalidControllerMock{std::make_shared<StrictMock<firebolt::rialto::ipc::RpcControllerMock>>()}
{
    m_service = std::make_shared<firebolt::rialto::server::ipc::ControlModuleService>(m_playbackServiceMock,
                                                                                      m_controlServiceMock);
}

ControlModuleServiceTests::~ControlModuleServiceTests() {}

void ControlModuleServiceTests::clientWillConnect()
{
    EXPECT_CALL(*m_clientMock, exportService(_));
}

void ControlModuleServiceTests::controlServiceWillRemoveControl()
{
    EXPECT_CALL(m_controlServiceMock, removeControl(_));
}

void ControlModuleServiceTests::controlServiceWillRegisterClient()
{
    EXPECT_CALL(*m_closureMock, Run());
    EXPECT_CALL(*m_controllerMock, getClient()).WillOnce(Return(m_clientMock));
    EXPECT_CALL(m_controlServiceMock, addControl(_, _));
}

void ControlModuleServiceTests::willFailDueToInvalidController()
{
    EXPECT_CALL(*m_invalidControllerMock, SetFailed(_));
    EXPECT_CALL(*m_closureMock, Run());
}

void ControlModuleServiceTests::playbackServiceWillGetSharedMemory()
{
    EXPECT_CALL(*m_closureMock, Run());
    EXPECT_CALL(m_playbackServiceMock, getSharedMemory(_, _))
        .WillOnce(DoAll(SetArgReferee<0>(kFd), SetArgReferee<1>(kSize), Return(true)));
}

void ControlModuleServiceTests::playbackServiceWillFailToGetSharedMemory()
{
    EXPECT_CALL(*m_controllerMock, SetFailed(_));
    EXPECT_CALL(*m_closureMock, Run());
    EXPECT_CALL(m_playbackServiceMock, getSharedMemory(_, _)).WillOnce(Return(false));
}

void ControlModuleServiceTests::playbackServiceWillAck()
{
    EXPECT_CALL(m_controlServiceMock, ack(kControlId, kPingId)).WillOnce(Return(true));
}

void ControlModuleServiceTests::playbackServiceWillFailToAck()
{
    EXPECT_CALL(*m_controllerMock, SetFailed(_));
    EXPECT_CALL(m_controlServiceMock, ack(kControlId, kPingId)).WillOnce(Return(false));
}

void ControlModuleServiceTests::sendClientConnected()
{
    m_service->clientConnected(m_clientMock);
}

void ControlModuleServiceTests::sendClientDisconnected()
{
    m_service->clientDisconnected(m_clientMock);
}

void ControlModuleServiceTests::sendRegisterClientRequestAndReceiveResponse()
{
    firebolt::rialto::RegisterClientRequest request;
    firebolt::rialto::RegisterClientResponse response;

    m_service->registerClient(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void ControlModuleServiceTests::sendRegisterClientRequestWithInvalidControllerAndReceiveFailure()
{
    firebolt::rialto::RegisterClientRequest request;
    firebolt::rialto::RegisterClientResponse response;

    m_service->registerClient(m_invalidControllerMock.get(), &request, &response, m_closureMock.get());
}

void ControlModuleServiceTests::sendGetSharedMemoryRequestAndReceiveResponse()
{
    firebolt::rialto::GetSharedMemoryRequest request;
    firebolt::rialto::GetSharedMemoryResponse response;

    m_service->getSharedMemory(m_controllerMock.get(), &request, &response, m_closureMock.get());

    EXPECT_EQ(response.fd(), kFd);
    EXPECT_EQ(response.size(), kSize);
}

void ControlModuleServiceTests::sendGetSharedMemoryRequestAndExpectFailure()
{
    firebolt::rialto::GetSharedMemoryRequest request;
    firebolt::rialto::GetSharedMemoryResponse response;

    m_service->getSharedMemory(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void ControlModuleServiceTests::sendAckRequestAndReceiveResponse()
{
    firebolt::rialto::AckRequest request;
    firebolt::rialto::AckResponse response;
    request.set_control_handle(kControlId);
    request.set_id(kPingId);

    EXPECT_CALL(*m_closureMock, Run());

    m_service->ack(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void ControlModuleServiceTests::sendAckRequestAndExpectFailure()
{
    firebolt::rialto::AckRequest request;
    firebolt::rialto::AckResponse response;
    request.set_control_handle(kControlId);
    request.set_id(kPingId);

    EXPECT_CALL(*m_closureMock, Run());

    m_service->ack(m_controllerMock.get(), &request, &response, m_closureMock.get());
}

void ControlModuleServiceTests::testFactoryCreatesObject()
{
    std::shared_ptr<firebolt::rialto::server::ipc::IControlModuleServiceFactory> factory =
      firebolt::rialto::server::ipc::IControlModuleServiceFactory::createFactory();
    EXPECT_NE(factory, nullptr);
    EXPECT_NE(factory->create(m_playbackServiceMock, m_controlServiceMock), nullptr);
}
