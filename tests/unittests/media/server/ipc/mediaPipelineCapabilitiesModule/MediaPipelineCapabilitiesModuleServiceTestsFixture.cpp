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

#include "MediaPipelineCapabilitiesModuleServiceTestsFixture.h"
#include "MediaCommon.h"
#include "MediaPipelineCapabilitiesModuleService.h"
#include "RialtoCommonModule.h"

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
const std::vector<std::string> kMimeTypes{"video/h264", "video/h265"};
const firebolt::rialto::MediaSourceType kSourceType{firebolt::rialto::MediaSourceType::VIDEO};
const firebolt::rialto::ProtoMediaSourceType kMediaSourceType{firebolt::rialto::ProtoMediaSourceType::VIDEO};
const std::string kPropertyName{"test-property"};
} // namespace

MediaPipelineCapabilitiesModuleServiceTests::MediaPipelineCapabilitiesModuleServiceTests()
    : m_clientMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ClientMock>>()},
      m_closureMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ClosureMock>>()},
      m_controllerMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ControllerMock>>()},
      m_invalidControllerMock{std::make_shared<StrictMock<firebolt::rialto::ipc::RpcControllerMock>>()}
{
    m_service = std::make_shared<firebolt::rialto::server::ipc::MediaPipelineCapabilitiesModuleService>(
        m_mediaPipelineServiceMock);
}

MediaPipelineCapabilitiesModuleServiceTests::~MediaPipelineCapabilitiesModuleServiceTests() {}

void MediaPipelineCapabilitiesModuleServiceTests::clientWillConnect()
{
    EXPECT_CALL(*m_clientMock, exportService(_));
}

void MediaPipelineCapabilitiesModuleServiceTests::mediaPipelineServiceWillGetSupportedMimeTypes()
{
    expectRequestSuccess();
    EXPECT_CALL(m_mediaPipelineServiceMock, getSupportedMimeTypes(kSourceType)).WillOnce(Return(kMimeTypes));
}

void MediaPipelineCapabilitiesModuleServiceTests::mediaPipelineWillCheckIfMimeTypeIsSupported()
{
    expectRequestSuccess();
    EXPECT_CALL(m_mediaPipelineServiceMock, isMimeTypeSupported(kMimeTypes[0])).WillOnce(Return(true));
}

void MediaPipelineCapabilitiesModuleServiceTests::mediaPipelineWillDoesSinkOrDecoderHaveProperty()
{
    expectRequestSuccess();
    EXPECT_CALL(m_mediaPipelineServiceMock,
                doesSinkOrDecoderHaveProperty(firebolt::rialto::server::ipc::convertMediaSourceType(kMediaSourceType),
                                              kPropertyName))
        .WillOnce(Return(true));
}

void MediaPipelineCapabilitiesModuleServiceTests::expectRequestSuccess()
{
    EXPECT_CALL(*m_closureMock, Run());
}

void MediaPipelineCapabilitiesModuleServiceTests::expectInvalidControlFailure()
{
    EXPECT_CALL(*m_invalidControllerMock, SetFailed(_));
    EXPECT_CALL(*m_closureMock, Run());
}

void MediaPipelineCapabilitiesModuleServiceTests::sendClientConnected()
{
    m_service->clientConnected(m_clientMock);
}

void MediaPipelineCapabilitiesModuleServiceTests::sendGetSupportedMimeTypesRequestAndReceiveResponse()
{
    firebolt::rialto::GetSupportedMimeTypesRequest request;
    firebolt::rialto::GetSupportedMimeTypesResponse response;

    request.set_media_type(firebolt::rialto::ProtoMediaSourceType::VIDEO);

    m_service->getSupportedMimeTypes(m_controllerMock.get(), &request, &response, m_closureMock.get());

    EXPECT_EQ((std::vector<std::string>{response.mime_types().begin(), response.mime_types().end()}), kMimeTypes);
}

void MediaPipelineCapabilitiesModuleServiceTests::sendGetSupportedMimeTypesRequestAndExpectFailure()
{
    firebolt::rialto::GetSupportedMimeTypesRequest request;
    firebolt::rialto::GetSupportedMimeTypesResponse response;

    m_service->getSupportedMimeTypes(m_invalidControllerMock.get(), &request, &response, m_closureMock.get());

    EXPECT_TRUE(response.mime_types().empty());
}

void MediaPipelineCapabilitiesModuleServiceTests::sendIsMimeTypeSupportedRequestAndReceiveResponse()
{
    firebolt::rialto::IsMimeTypeSupportedRequest request;
    firebolt::rialto::IsMimeTypeSupportedResponse response;

    request.set_mime_type(kMimeTypes[0]);

    m_service->isMimeTypeSupported(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(response.is_supported(), true);
}

void MediaPipelineCapabilitiesModuleServiceTests::sendDoesSinkOrDecoderHavePropertyWithSuccess()
{
    firebolt::rialto::DoesSinkOrDecoderHavePropertyRequest request;
    firebolt::rialto::DoesSinkOrDecoderHavePropertyResponse response;

    request.set_media_type(kMediaSourceType);
    request.set_property_name(kPropertyName);

    m_service->doesSinkOrDecoderHaveProperty(m_controllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(response.has_property(), true);
}

void MediaPipelineCapabilitiesModuleServiceTests::sendIsMimeTypeSupportedRequestAndExpectFailure()
{
    firebolt::rialto::IsMimeTypeSupportedRequest request;
    firebolt::rialto::IsMimeTypeSupportedResponse response;

    m_service->isMimeTypeSupported(m_invalidControllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(response.is_supported(), false);
}
