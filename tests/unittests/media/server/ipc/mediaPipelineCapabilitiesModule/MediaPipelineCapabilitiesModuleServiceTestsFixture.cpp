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
const std::vector<std::string> kPropertyNames{"test-property", "another-property"};
constexpr bool kIsVideoMaster{true};
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

void MediaPipelineCapabilitiesModuleServiceTests::mediaPipelineWillGetSupportedProperties()
{
    expectRequestSuccess();
    EXPECT_CALL(m_mediaPipelineServiceMock,
                getSupportedProperties(firebolt::rialto::server::ipc::convertMediaSourceType(kMediaSourceType),
                                       kPropertyNames))
        .WillOnce(Return(kPropertyNames));
}

void MediaPipelineCapabilitiesModuleServiceTests::mediaPipelineWillCheckIfVideoIsMaster()
{
    expectRequestSuccess();
    EXPECT_CALL(m_mediaPipelineServiceMock, isVideoMaster(_))
        .WillOnce(DoAll(SetArgReferee<0>(kIsVideoMaster), Return(true)));
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

void MediaPipelineCapabilitiesModuleServiceTests::sendIsMimeTypeSupportedRequestAndExpectFailure()
{
    firebolt::rialto::IsMimeTypeSupportedRequest request;
    firebolt::rialto::IsMimeTypeSupportedResponse response;

    m_service->isMimeTypeSupported(m_invalidControllerMock.get(), &request, &response, m_closureMock.get());
    EXPECT_EQ(response.is_supported(), false);
}

void MediaPipelineCapabilitiesModuleServiceTests::sendGetSupportedPropertiesRequestWithSuccess()
{
    firebolt::rialto::GetSupportedPropertiesRequest request;
    firebolt::rialto::GetSupportedPropertiesResponse response;

    request.set_media_type(kMediaSourceType);
    for (const std::string &property : kPropertyNames)
        request.add_property_names(property.c_str());

    m_service->getSupportedProperties(m_controllerMock.get(), &request, &response, m_closureMock.get());

    std::vector<std::string> supportedProperties{response.supported_properties().begin(),
                                                 response.supported_properties().end()};
    EXPECT_EQ(supportedProperties, kPropertyNames);
}

void MediaPipelineCapabilitiesModuleServiceTests::sendGetSupportedPropertiesRequestAndExpectFailure()
{
    firebolt::rialto::GetSupportedPropertiesRequest request;
    firebolt::rialto::GetSupportedPropertiesResponse response;

    request.set_media_type(kMediaSourceType);
    for (const std::string &property : kPropertyNames)
        request.add_property_names(property.c_str());

    m_service->getSupportedProperties(m_invalidControllerMock.get(), &request, &response, m_closureMock.get());

    std::vector<std::string> supportedProperties{response.supported_properties().begin(),
                                                 response.supported_properties().end()};
    EXPECT_TRUE(supportedProperties.empty());
}

void MediaPipelineCapabilitiesModuleServiceTests::sendIsVideoMasterRequestWithSuccess()
{
    firebolt::rialto::IsVideoMasterRequest request;
    firebolt::rialto::IsVideoMasterResponse response;

    m_service->isVideoMaster(m_controllerMock.get(), &request, &response, m_closureMock.get());

    EXPECT_EQ(response.is_video_master(), kIsVideoMaster);
}

void MediaPipelineCapabilitiesModuleServiceTests::sendIsVideoMasterRequestAndExpectFailure()
{
    firebolt::rialto::IsVideoMasterRequest request;
    firebolt::rialto::IsVideoMasterResponse response;
    m_service->isVideoMaster(m_invalidControllerMock.get(), &request, &response, m_closureMock.get());
}

void MediaPipelineCapabilitiesModuleServiceTests::expectCorrectMediaTypeConversion()
{
    EXPECT_EQ(firebolt::rialto::server::ipc::convertMediaSourceType(firebolt::rialto::ProtoMediaSourceType::UNKNOWN),
              firebolt::rialto::MediaSourceType::UNKNOWN);
    EXPECT_EQ(firebolt::rialto::server::ipc::convertMediaSourceType(firebolt::rialto::ProtoMediaSourceType::AUDIO),
              firebolt::rialto::MediaSourceType::AUDIO);
    EXPECT_EQ(firebolt::rialto::server::ipc::convertMediaSourceType(firebolt::rialto::ProtoMediaSourceType::VIDEO),
              firebolt::rialto::MediaSourceType::VIDEO);
    EXPECT_EQ(firebolt::rialto::server::ipc::convertMediaSourceType(firebolt::rialto::ProtoMediaSourceType::SUBTITLE),
              firebolt::rialto::MediaSourceType::SUBTITLE);
    EXPECT_EQ(firebolt::rialto::server::ipc::convertMediaSourceType(
                  static_cast<firebolt::rialto::ProtoMediaSourceType>(4)),
              firebolt::rialto::MediaSourceType::UNKNOWN);
}
