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
using firebolt::rialto::capture::CapturedFrame;
using firebolt::rialto::capture::CaptureSessionMock;
using firebolt::rialto::capture::CaptureStatus;
using firebolt::rialto::capture::test::CaptureRecordFixtures;
using firebolt::rialto::server::CaptureSessionRuntime;
using testing::_;
using testing::Invoke;
using testing::NiceMock;
using testing::Return;
using testing::StrictMock;

namespace
{
constexpr std::uint32_t kCaptureId{7};
constexpr std::uint32_t kPoolId{3};
} // namespace

/**
 * The acquire handler against a real session runtime over a mocked HAL session (R9 2.1).
 */
class VideoFrameCaptureAcquireTests : public ::testing::Test
{
protected:
    VideoFrameCaptureAcquireTests()
    {
        EXPECT_CALL(*m_client, exportService(_));
        m_sut->clientConnected(m_client);
        capture::CapturePool pool;
        CaptureRecordFixtures::makeNv12Pool(pool, 3);
        m_runtime = std::make_shared<
            CaptureSessionRuntime>(firebolt::rialto::wrappers::CaptureSessionPtr{m_session,
                                                                                 [](capture::ICaptureSession *session)
                                                                                 { delete session; }},
                                   kPoolId, pool);
    }

    AcquireCurrentVideoFrameResponse acquire()
    {
        AcquireCurrentVideoFrameRequest request;
        request.set_capture_id(kCaptureId);
        AcquireCurrentVideoFrameResponse response;
        EXPECT_CALL(m_controller, getClient()).WillOnce(Return(m_client));
        EXPECT_CALL(m_closure, Run());
        m_sut->acquireCurrentFrame(&m_controller, &request, &response, &m_closure);
        return response;
    }

    void captureIsCommitted()
    {
        EXPECT_CALL(m_service, hasCapture(1, kCaptureId)).WillRepeatedly(Return(true));
        EXPECT_CALL(m_service, captureRuntime(1, kCaptureId)).WillRepeatedly(Return(m_runtime));
    }

    CaptureRecordFixtures m_records;
    NiceMock<CaptureSessionMock> *m_session{new NiceMock<CaptureSessionMock>()};
    std::shared_ptr<CaptureSessionRuntime> m_runtime;
    StrictMock<VideoFrameCaptureServiceMock> m_service;
    StrictMock<firebolt::rialto::ipc::ControllerMock> m_controller;
    StrictMock<firebolt::rialto::ipc::ClosureMock> m_closure;
    std::shared_ptr<StrictMock<firebolt::rialto::ipc::ClientMock>> m_client{
        std::make_shared<StrictMock<firebolt::rialto::ipc::ClientMock>>()};
    std::shared_ptr<VideoFrameCaptureModuleService> m_sut{std::make_shared<VideoFrameCaptureModuleService>(m_service)};
};

TEST_F(VideoFrameCaptureAcquireTests, OkFillsTheReplyAndHandsOverTheDescriptors)
{
    captureIsCommitted();
    int exportedFd{-1};
    EXPECT_CALL(*m_session, acquireCurrentFrame(_))
        .WillOnce(Invoke(
            [&](CapturedFrame &frame)
            {
                m_records.makeExampleC(frame, 1, 42); // two objects, two descriptors
                exportedFd = frame.exportedDmaBufs[0].fd;
                return CaptureStatus::OK;
            }));
    EXPECT_CALL(m_controller, setReplyFdsOwned(true));

    const auto kResponse = acquire();
    ASSERT_EQ(kResponse.status(), VIDEO_FRAME_CAPTURE_STATUS_OK);
    EXPECT_EQ(kResponse.pool_id(), kPoolId);
    EXPECT_EQ(kResponse.slot_index(), 1U);
    EXPECT_EQ(kResponse.acquisition_token(), 42U);
    EXPECT_EQ(kResponse.frame_metadata().drm_format(), capture::test::kDrmFormatNv12);
    EXPECT_EQ(kResponse.frame_metadata().backing_width(), capture::test::kWidth);
    EXPECT_EQ(kResponse.frame_metadata().backing_height(), capture::test::kHeight);
    EXPECT_EQ(kResponse.frame_metadata().dma_buf_objects_size(), 2);
    EXPECT_EQ(kResponse.frame_metadata().planes_size(), 2);
    EXPECT_EQ(kResponse.visible_region().height(), capture::test::kVisibleHeight);
    ASSERT_EQ(kResponse.exported_dma_bufs_size(), 2);
    EXPECT_EQ(kResponse.exported_dma_bufs(0).fd(), exportedFd);
    EXPECT_EQ(kResponse.exported_dma_bufs(0).dma_buf_object_id(), capture::test::kFirstObjectId);
    EXPECT_EQ(kResponse.exported_dma_bufs(1).dma_buf_object_id(), capture::test::kSecondObjectId);
    EXPECT_FALSE(kResponse.dma_buf_objects_flushed());
    EXPECT_TRUE(m_runtime->isSlotLocked(1));
}

TEST_F(VideoFrameCaptureAcquireTests, NoNewFrameAndNoResourcesArePassedOn)
{
    captureIsCommitted();
    EXPECT_CALL(*m_session, acquireCurrentFrame(_))
        .WillOnce(Return(CaptureStatus::NO_NEW_FRAME))
        .WillOnce(Return(CaptureStatus::NO_RESOURCES));
    const auto kNoNewFrame = acquire();
    EXPECT_EQ(kNoNewFrame.status(), VIDEO_FRAME_CAPTURE_STATUS_NO_NEW_FRAME);
    EXPECT_EQ(kNoNewFrame.exported_dma_bufs_size(), 0);
    EXPECT_EQ(acquire().status(), VIDEO_FRAME_CAPTURE_STATUS_NO_RESOURCES);
}

TEST_F(VideoFrameCaptureAcquireTests, NoFrameFromTheHalIsPassedOn)
{
    captureIsCommitted();
    EXPECT_CALL(*m_session, acquireCurrentFrame(_)).WillOnce(Return(CaptureStatus::NO_FRAME));
    EXPECT_EQ(acquire().status(), VIDEO_FRAME_CAPTURE_STATUS_NO_FRAME);
}

TEST_F(VideoFrameCaptureAcquireTests, NoFrameWithoutACommittedSession)
{
    EXPECT_CALL(m_service, hasCapture(1, kCaptureId)).WillOnce(Return(true));
    EXPECT_CALL(m_service, captureRuntime(1, kCaptureId)).WillOnce(Return(nullptr));
    EXPECT_EQ(acquire().status(), VIDEO_FRAME_CAPTURE_STATUS_NO_FRAME);
}

TEST_F(VideoFrameCaptureAcquireTests, UnknownCaptureIsInvalid)
{
    EXPECT_CALL(m_service, hasCapture(1, kCaptureId)).WillOnce(Return(false));
    EXPECT_EQ(acquire().status(), VIDEO_FRAME_CAPTURE_STATUS_INVALID_ARGUMENT);
}
