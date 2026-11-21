/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
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

#include "ClientMock.h"
#include "ClosureMock.h"
#include "IpcControllerMock.h"
#include "VideoFrameCaptureModuleService.h"
#include "VideoFrameCaptureServiceMock.h"
#include <gtest/gtest.h>

#include <optional>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server::ipc;
using namespace firebolt::rialto::server::service;
using testing::_;
using testing::DoAll;
using testing::Return;
using testing::SetArgReferee;
using testing::StrictMock;

class VideoFrameCaptureModuleServiceTests : public ::testing::Test
{
protected:
    void connect(const std::shared_ptr<StrictMock<firebolt::rialto::ipc::ClientMock>> &client)
    {
        EXPECT_CALL(*client, exportService(_));
        m_sut->clientConnected(client);
    }

    CreateVideoFrameCaptureResponse create(const std::shared_ptr<firebolt::rialto::ipc::IClient> &client,
                                           VideoFrameCaptureGraphicsAPIType api)
    {
        CreateVideoFrameCaptureRequest request;
        request.set_api(api);
        CreateVideoFrameCaptureResponse response;
        EXPECT_CALL(m_controller, getClient()).WillOnce(Return(client));
        EXPECT_CALL(m_closure, Run());
        m_sut->createVideoFrameCapture(&m_controller, &request, &response, &m_closure);
        return response;
    }

    StrictMock<VideoFrameCaptureServiceMock> m_service;
    StrictMock<firebolt::rialto::ipc::ControllerMock> m_controller;
    StrictMock<firebolt::rialto::ipc::ClosureMock> m_closure;
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::ClientMock>> m_client{
        std::make_shared<StrictMock<firebolt::rialto::ipc::ClientMock>>()};
    std::shared_ptr<VideoFrameCaptureModuleService> m_sut{std::make_shared<VideoFrameCaptureModuleService>(m_service)};
};

TEST_F(VideoFrameCaptureModuleServiceTests, ShouldCreateGlCapture)
{
    connect(m_client);
    EXPECT_CALL(m_service, createCapture(1, std::optional<GraphicsAPIType>{GraphicsAPIType::GL}, _))
        .WillOnce(DoAll(SetArgReferee<2>(7u), Return(VideoFrameCaptureServiceStatus::OK)));

    const auto kResponse = create(m_client, GRAPHICS_API_TYPE_GL);
    EXPECT_EQ(kResponse.status(), VIDEO_FRAME_CAPTURE_STATUS_OK);
    EXPECT_EQ(kResponse.capture_id(), 7u);
}

TEST_F(VideoFrameCaptureModuleServiceTests, ShouldReportUnsupportedWithoutCaptureId)
{
    connect(m_client);
    EXPECT_CALL(m_service, createCapture(1, std::optional<GraphicsAPIType>{GraphicsAPIType::VULKAN}, _))
        .WillOnce(Return(VideoFrameCaptureServiceStatus::UNSUPPORTED));

    const auto kResponse = create(m_client, GRAPHICS_API_TYPE_VULKAN);
    EXPECT_EQ(kResponse.status(), VIDEO_FRAME_CAPTURE_STATUS_UNSUPPORTED);
    EXPECT_FALSE(kResponse.has_capture_id());
}

TEST_F(VideoFrameCaptureModuleServiceTests, ShouldPassUnknownApiAsNone)
{
    connect(m_client);
    EXPECT_CALL(m_service, createCapture(1, std::optional<GraphicsAPIType>{}, _))
        .WillOnce(Return(VideoFrameCaptureServiceStatus::INVALID_ARGUMENT));

    EXPECT_EQ(create(m_client, GRAPHICS_API_TYPE_UNKNOWN).status(), VIDEO_FRAME_CAPTURE_STATUS_INVALID_ARGUMENT);
}

TEST_F(VideoFrameCaptureModuleServiceTests, ShouldGiveEachConnectionItsOwnClientId)
{
    auto otherClient = std::make_shared<StrictMock<firebolt::rialto::ipc::ClientMock>>();
    connect(m_client);
    connect(otherClient);
    EXPECT_CALL(m_service, createCapture(1, _, _)).WillOnce(Return(VideoFrameCaptureServiceStatus::OK));
    EXPECT_CALL(m_service, createCapture(2, _, _)).WillOnce(Return(VideoFrameCaptureServiceStatus::OK));

    create(m_client, GRAPHICS_API_TYPE_GL);
    create(otherClient, GRAPHICS_API_TYPE_GL);
}

TEST_F(VideoFrameCaptureModuleServiceTests, ShouldDestroyOwnCaptureAndRejectForeignOne)
{
    connect(m_client);
    DestroyVideoFrameCaptureRequest request;
    DestroyVideoFrameCaptureResponse response;

    request.set_capture_id(7);
    EXPECT_CALL(m_controller, getClient()).WillOnce(Return(m_client));
    EXPECT_CALL(m_service, destroyCapture(1, 7)).WillOnce(Return(true));
    EXPECT_CALL(m_closure, Run());
    m_sut->destroyVideoFrameCapture(&m_controller, &request, &response, &m_closure);

    request.set_capture_id(99); // not this connection's capture
    EXPECT_CALL(m_controller, getClient()).WillOnce(Return(m_client));
    EXPECT_CALL(m_service, destroyCapture(1, 99)).WillOnce(Return(false));
    EXPECT_CALL(m_controller, SetFailed(_));
    EXPECT_CALL(m_closure, Run());
    m_sut->destroyVideoFrameCapture(&m_controller, &request, &response, &m_closure);
}

TEST_F(VideoFrameCaptureModuleServiceTests, ShouldRejectCallFromUnknownClient)
{
    CreateVideoFrameCaptureRequest request;
    CreateVideoFrameCaptureResponse response;
    EXPECT_CALL(m_controller, getClient()).WillOnce(Return(m_client)); // never connected
    EXPECT_CALL(m_controller, SetFailed(_));
    EXPECT_CALL(m_closure, Run());
    m_sut->createVideoFrameCapture(&m_controller, &request, &response, &m_closure);
}

TEST_F(VideoFrameCaptureModuleServiceTests, ShouldRemoveClientsCapturesOnDisconnect)
{
    connect(m_client);
    EXPECT_CALL(m_service, removeClient(1));
    m_sut->clientDisconnected(m_client);

    // a second disconnect of the same connection does nothing
    m_sut->clientDisconnected(m_client);
}
