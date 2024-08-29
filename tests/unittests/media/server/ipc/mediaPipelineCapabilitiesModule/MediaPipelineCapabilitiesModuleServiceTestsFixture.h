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

#ifndef MEDIA_PIPELINE_CAPABILITIES_MODULE_SERVICE_TESTS_FIXTURE_H_
#define MEDIA_PIPELINE_CAPABILITIES_MODULE_SERVICE_TESTS_FIXTURE_H_

#include "ClientMock.h"
#include "ClosureMock.h"
#include "IMediaPipelineCapabilitiesModuleService.h"
#include "IpcControllerMock.h"
#include "IpcServerMock.h"
#include "MediaPipelineServiceMock.h"
#include "RpcControllerMock.h"
#include <gtest/gtest.h>
#include <memory>

using testing::StrictMock;

class MediaPipelineCapabilitiesModuleServiceTests : public testing::Test
{
public:
    MediaPipelineCapabilitiesModuleServiceTests();
    ~MediaPipelineCapabilitiesModuleServiceTests() override;

    void clientWillConnect();
    void mediaPipelineServiceWillGetSupportedMimeTypes();
    void mediaPipelineWillCheckIfMimeTypeIsSupported();
    void mediaPipelineWillGetSupportedProperties();

    void sendClientConnected();
    void sendClientDisconnected();
    void sendGetSupportedMimeTypesRequestAndReceiveResponse();
    void sendGetSupportedMimeTypesRequestAndExpectFailure();
    void sendIsMimeTypeSupportedRequestAndReceiveResponse();
    void sendIsMimeTypeSupportedRequestAndExpectFailure();
    void sendGetSupportedPropertiesRequestWithSuccess();
    void sendGetSupportedPropertiesRequestAndExpectFailure();
    void expectInvalidControlFailure();

private:
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::ClientMock>> m_clientMock;
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::ClosureMock>> m_closureMock;
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::ControllerMock>> m_controllerMock;
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::RpcControllerMock>> m_invalidControllerMock;
    StrictMock<firebolt::rialto::server::service::MediaPipelineServiceMock> m_mediaPipelineServiceMock;
    std::shared_ptr<firebolt::rialto::server::ipc::IMediaPipelineCapabilitiesModuleService> m_service;

    void expectRequestSuccess();
};

#endif // MEDIA_PIPELINE_CAPABILITIES_MODULE_SERVICE_TESTS_FIXTURE_H_
