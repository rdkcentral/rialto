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

#include "CaptureRecordFixtures.h"
#include "CaptureSessionMock.h"
#include "CaptureSessionRuntime.h"
#include "ClientMock.h"
#include "ClosureMock.h"
#include "IpcControllerMock.h"
#include "VideoFrameCaptureModuleService.h"
#include "VideoFrameCaptureServiceMock.h"

#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server::ipc;
using namespace firebolt::rialto::server::service;
using firebolt::rialto::capture::CaptureCapabilities;
using firebolt::rialto::capture::CapturedFrame;
using firebolt::rialto::capture::CaptureSessionMock;
using firebolt::rialto::capture::CaptureStatus;
using firebolt::rialto::capture::test::CaptureRecordFixtures;
using firebolt::rialto::server::CaptureSessionRuntime;
using testing::_;
using testing::DoAll;
using testing::Invoke;
using testing::NiceMock;
using testing::Return;
using testing::SetArgReferee;
using testing::StrictMock;

namespace
{
constexpr std::uint32_t kCaptureId{7};
constexpr std::uint32_t kPoolId{3};
constexpr std::uint32_t kSlots{3};
constexpr std::uint64_t kModifier{0x0100000000000001ULL};

CaptureCapabilities admittedCapabilities()
{
    CaptureCapabilities caps{};
    caps.drmFormat = capture::test::kDrmFormatNv12;
    caps.drmModifier = kModifier;
    caps.maximumContentSize = {capture::test::kWidth, capture::test::kHeight};
    caps.minimumSlots = 1;
    caps.maximumSlots = 4;
    return caps;
}
} // namespace

/**
 * The capabilities and pool queries against a real session runtime over a mocked HAL session (R8 3.2, 3.3).
 */
class VideoFrameCaptureQueryTests : public ::testing::Test
{
protected:
    VideoFrameCaptureQueryTests()
    {
        EXPECT_CALL(*m_client, exportService(_));
        m_sut->clientConnected(m_client);
        CaptureRecordFixtures::makeNv12Pool(m_pool, kSlots);
        m_runtime = std::make_shared<
            CaptureSessionRuntime>(firebolt::rialto::wrappers::CaptureSessionPtr{m_session,
                                                                                 [](capture::ICaptureSession *session)
                                                                                 { delete session; }},
                                   kPoolId, m_pool, admittedCapabilities());
    }

    GetVideoFrameCaptureCapabilitiesResponse getCapabilities(VideoFrameCaptureGraphicsAPIType api = GRAPHICS_API_TYPE_GL,
                                                             std::uint32_t captureId = kCaptureId)
    {
        GetVideoFrameCaptureCapabilitiesRequest request;
        request.set_capture_id(captureId);
        request.set_api(api);
        GetVideoFrameCaptureCapabilitiesResponse response;
        EXPECT_CALL(m_controller, getClient()).WillOnce(Return(m_client));
        EXPECT_CALL(m_closure, Run());
        m_sut->getCapabilities(&m_controller, &request, &response, &m_closure);
        return response;
    }

    void glCaptureExists()
    {
        EXPECT_CALL(m_service, captureApi(1, kCaptureId)).WillRepeatedly(Return(GraphicsAPIType::GL));
    }
    void captureIsCommitted()
    {
        glCaptureExists();
        EXPECT_CALL(m_service, captureRuntime(1, kCaptureId)).WillRepeatedly(Return(m_runtime));
    }
    void captureIsNotCommitted()
    {
        glCaptureExists();
        EXPECT_CALL(m_service, captureRuntime(1, kCaptureId)).WillRepeatedly(Return(nullptr));
    }

    CaptureRecordFixtures m_records;
    capture::CapturePool m_pool;
    NiceMock<CaptureSessionMock> *m_session{new NiceMock<CaptureSessionMock>()};
    std::shared_ptr<CaptureSessionRuntime> m_runtime;
    StrictMock<VideoFrameCaptureServiceMock> m_service;
    StrictMock<firebolt::rialto::ipc::ControllerMock> m_controller;
    StrictMock<firebolt::rialto::ipc::ClosureMock> m_closure;
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::ClientMock>> m_client{
        std::make_shared<StrictMock<firebolt::rialto::ipc::ClientMock>>()};
    std::shared_ptr<VideoFrameCaptureModuleService> m_sut{std::make_shared<VideoFrameCaptureModuleService>(m_service)};
};

TEST_F(VideoFrameCaptureQueryTests, UnknownCaptureIsAnInvalidArgument)
{
    EXPECT_CALL(m_service, captureApi(1, 99)).WillOnce(Return(std::nullopt));
    EXPECT_EQ(getCapabilities(GRAPHICS_API_TYPE_GL, 99).status(), VIDEO_FRAME_CAPTURE_STATUS_INVALID_ARGUMENT);
}

TEST_F(VideoFrameCaptureQueryTests, WrongApiIsAnInvalidArgument)
{
    glCaptureExists();
    EXPECT_CALL(*m_session, getCapabilities(_)).Times(0);
    EXPECT_EQ(getCapabilities(GRAPHICS_API_TYPE_VULKAN).status(), VIDEO_FRAME_CAPTURE_STATUS_INVALID_ARGUMENT);
    EXPECT_EQ(getCapabilities(GRAPHICS_API_TYPE_UNKNOWN).status(), VIDEO_FRAME_CAPTURE_STATUS_INVALID_ARGUMENT);
}

TEST_F(VideoFrameCaptureQueryTests, UncommittedCaptureIsNotReady)
{
    captureIsNotCommitted();
    EXPECT_EQ(getCapabilities().status(), VIDEO_FRAME_CAPTURE_STATUS_SESSION_NOT_READY);
}

TEST_F(VideoFrameCaptureQueryTests, RetiredSessionIsNotReady)
{
    captureIsCommitted(); // the runtime retires between the lookup and the query
    m_runtime->retire();
    EXPECT_CALL(*m_session, getCapabilities(_)).Times(0);
    EXPECT_EQ(getCapabilities().status(), VIDEO_FRAME_CAPTURE_STATUS_SESSION_NOT_READY);
}

TEST_F(VideoFrameCaptureQueryTests, FatalSessionIsAFatalError)
{
    captureIsCommitted();
    EXPECT_CALL(*m_session, acquireCurrentFrame(_))
        .WillOnce(Invoke(
            [](CapturedFrame &frame)
            {
                frame.slotIndex = 99; // invalid: the session becomes capture-fatal
                frame.acquisitionToken = 1;
                return CaptureStatus::OK;
            }));
    CaptureSessionRuntime::DeliveryTicket ticket;
    ASSERT_EQ(m_runtime->acquire([](const CapturedFrame &, bool, bool) {}, ticket), CaptureStatus::FATAL_ERROR);
    EXPECT_CALL(*m_session, getCapabilities(_)).Times(0);
    EXPECT_EQ(getCapabilities().status(), VIDEO_FRAME_CAPTURE_STATUS_FATAL_ERROR);
}

TEST_F(VideoFrameCaptureQueryTests, HalErrorIsPassedOn)
{
    captureIsCommitted();
    EXPECT_CALL(*m_session, getCapabilities(_)).WillOnce(Return(CaptureStatus::NO_RESOURCES));
    EXPECT_EQ(getCapabilities().status(), VIDEO_FRAME_CAPTURE_STATUS_NO_RESOURCES);
}

TEST_F(VideoFrameCaptureQueryTests, OkReportsTheAdmittedSession)
{
    captureIsCommitted();
    EXPECT_CALL(*m_session, getCapabilities(_))
        .WillOnce(DoAll(SetArgReferee<0>(admittedCapabilities()), Return(CaptureStatus::OK)));
    const auto kResponse = getCapabilities();
    ASSERT_EQ(kResponse.status(), VIDEO_FRAME_CAPTURE_STATUS_OK);
    EXPECT_EQ(kResponse.max_concurrent_frames(), kSlots); // the admitted N, not maximumSlots
    EXPECT_EQ(kResponse.max_resources_per_frame(), 1U);
    EXPECT_EQ(kResponse.supports_protected_content(), 0U);
    EXPECT_EQ(kResponse.supports_hdr(), 0U);
    ASSERT_EQ(kResponse.drm_fourccs_size(), 1);
    EXPECT_EQ(kResponse.drm_fourccs(0), capture::test::kDrmFormatNv12);
    ASSERT_EQ(kResponse.drm_modifiers_size(), 1);
    EXPECT_EQ(kResponse.drm_modifiers(0), kModifier);
}
