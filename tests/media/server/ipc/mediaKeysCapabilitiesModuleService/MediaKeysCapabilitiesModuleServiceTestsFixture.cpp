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

#include "MediaKeysCapabilitiesModuleServiceTestsFixture.h"
#include "MediaKeysCapabilitiesModuleService.h"
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <vector>

using testing::_;
using testing::DoAll;
using testing::Invoke;
using testing::Return;
using testing::SaveArg;
using testing::SetArgReferee;

namespace
{
const std::vector<std::string> keySystems{"expectedKeySystem1", "expectedKeySystem2", "expectedKeySystem3"};
const std::string version{"123"};
constexpr bool isKeySystemSupported{true};
} // namespace

MediaKeysCapabilitiesModuleServiceTests::MediaKeysCapabilitiesModuleServiceTests()
    : m_clientMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ClientMock>>()},
      m_serverMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ServerMock>>()},
      m_closureMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ClosureMock>>()},
      m_controllerMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ControllerMock>>()}
{
    m_service = std::make_shared<firebolt::rialto::server::ipc::MediaKeysCapabilitiesModuleService>(m_cdmServiceMock);
}

MediaKeysCapabilitiesModuleServiceTests::~MediaKeysCapabilitiesModuleServiceTests() {}

void MediaKeysCapabilitiesModuleServiceTests::clientWillConnect()
{
    EXPECT_CALL(*m_clientMock, exportService(_));
}

void MediaKeysCapabilitiesModuleServiceTests::cdmServiceWillGetSupportedKeySystems()
{
    expectRequestSuccess();
    EXPECT_CALL(m_cdmServiceMock, getSupportedKeySystems()).WillOnce(Return(keySystems));
}

void MediaKeysCapabilitiesModuleServiceTests::cdmServiceWillSupportsKeySystem()
{
    expectRequestSuccess();
    EXPECT_CALL(m_cdmServiceMock, supportsKeySystem(keySystems[0])).WillOnce(Return(true));
}

void MediaKeysCapabilitiesModuleServiceTests::cdmServiceWillGetSupportedKeySystemVersion()
{
    expectRequestSuccess();
    EXPECT_CALL(m_cdmServiceMock, getSupportedKeySystemVersion(keySystems[0], _))
        .WillOnce(DoAll(SetArgReferee<1>(version), Return(true)));
}

void MediaKeysCapabilitiesModuleServiceTests::cdmServiceWillFailToGetSupportedKeySystemVersion()
{
    expectRequestFailure();
    EXPECT_CALL(m_cdmServiceMock, getSupportedKeySystemVersion(keySystems[0], _)).WillOnce(Return(false));
}

void MediaKeysCapabilitiesModuleServiceTests::sendClientConnected()
{
    m_service->clientConnected(m_clientMock);
}

void MediaKeysCapabilitiesModuleServiceTests::sendGetSupportedKeySystemsRequestAndReceiveResponse()
{
    firebolt::rialto::GetSupportedKeySystemsRequest request;
    firebolt::rialto::GetSupportedKeySystemsResponse response;

    m_service->getSupportedKeySystems(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ((std::vector<std::string>{response.key_systems().begin(), response.key_systems().end()}), keySystems);
}

void MediaKeysCapabilitiesModuleServiceTests::sendSupportsKeySystemRequestAndReceiveResponse()
{
    firebolt::rialto::SupportsKeySystemRequest request;
    firebolt::rialto::SupportsKeySystemResponse response;

    request.set_key_system(keySystems[0]);

    m_service->supportsKeySystem(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(response.is_supported(), isKeySystemSupported);
}

void MediaKeysCapabilitiesModuleServiceTests::sendGetSupportedKeySystemVersionRequestAndReceiveResponse()
{
    firebolt::rialto::GetSupportedKeySystemVersionRequest request;
    firebolt::rialto::GetSupportedKeySystemVersionResponse response;

    request.set_key_system(keySystems[0]);

    m_service->getSupportedKeySystemVersion(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(response.version(), version);
}

void MediaKeysCapabilitiesModuleServiceTests::sendGetSupportedKeySystemVersionRequestAndExpectFailure()
{
    firebolt::rialto::GetSupportedKeySystemVersionRequest request;
    firebolt::rialto::GetSupportedKeySystemVersionResponse response;

    request.set_key_system(keySystems[0]);

    m_service->getSupportedKeySystemVersion(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(response.version(), "");
}

void MediaKeysCapabilitiesModuleServiceTests::expectRequestSuccess()
{
    EXPECT_CALL(*m_closureMock, Run());
}

void MediaKeysCapabilitiesModuleServiceTests::expectRequestFailure()
{
    EXPECT_CALL(*m_controllerMock, SetFailed(_));
    EXPECT_CALL(*m_closureMock, Run());
}

void MediaKeysCapabilitiesModuleServiceTests::testFactoryCreatesObject()
{
    std::shared_ptr<firebolt::rialto::server::ipc::IMediaKeysCapabilitiesModuleServiceFactory> factory =
      firebolt::rialto::server::ipc::IMediaKeysCapabilitiesModuleServiceFactory::createFactory();
    EXPECT_NE(factory, nullptr);
    EXPECT_NE(factory->create(m_cdmServiceMock), nullptr);
}
