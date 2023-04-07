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
constexpr int32_t fd{123};
constexpr uint32_t size{456U};
constexpr int kControlId{4};
} // namespace

namespace firebolt::rialto
{
} // namespace firebolt::rialto

ControlModuleServiceTests::ControlModuleServiceTests()
    : m_clientMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ClientMock>>()},
      m_serverMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ServerMock>>()},
      m_closureMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ClosureMock>>()},
      m_controllerMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ControllerMock>>()}
{
    m_service = std::make_shared<firebolt::rialto::server::ipc::ControlModuleService>(m_playbackServiceMock,
                                                                                      m_controlServiceMock);
}

ControlModuleServiceTests::~ControlModuleServiceTests() {}

void ControlModuleServiceTests::clientWillConnect()
{
    EXPECT_CALL(m_controlServiceMock, addControl(_)).WillOnce(Return(kControlId));
    EXPECT_CALL(*m_clientMock, exportService(_));
}

void ControlModuleServiceTests::controlServiceWillRemoveControl()
{
    EXPECT_CALL(m_controlServiceMock, removeControl(kControlId));
}

void ControlModuleServiceTests::playbackServiceWillGetSharedMemory()
{
    EXPECT_CALL(*m_closureMock, Run());
    EXPECT_CALL(m_playbackServiceMock, getSharedMemory(_, _))
        .WillOnce(DoAll(SetArgReferee<0>(fd), SetArgReferee<1>(size), Return(true)));
}

void ControlModuleServiceTests::playbackServiceWillFailToGetSharedMemory()
{
    EXPECT_CALL(*m_controllerMock, SetFailed(_));
    EXPECT_CALL(*m_closureMock, Run());
    EXPECT_CALL(m_playbackServiceMock, getSharedMemory(_, _)).WillOnce(Return(false));
}

void ControlModuleServiceTests::sendClientConnected()
{
    m_service->clientConnected(m_clientMock);
}

void ControlModuleServiceTests::sendClientDisconnected()
{
    m_service->clientDisconnected(m_clientMock);
}

void ControlModuleServiceTests::sendGetSharedMemoryRequestAndReceiveResponse()
{
    firebolt::rialto::GetSharedMemoryRequest request;
    firebolt::rialto::GetSharedMemoryResponse response;

    m_service->getSharedMemory(m_controllerMock.get(), &request, &response, m_closureMock.get());

    EXPECT_EQ(response.fd(), fd);
    EXPECT_EQ(response.size(), size);
}

void ControlModuleServiceTests::sendGetSharedMemoryRequestAndExpectFailure()
{
    firebolt::rialto::GetSharedMemoryRequest request;
    firebolt::rialto::GetSharedMemoryResponse response;

    m_service->getSharedMemory(m_controllerMock.get(), &request, &response, m_closureMock.get());
}
