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
        EXPECT_CALL(m_service, sessionForRelease(1, kCaptureId, _))
            .WillRepeatedly(Invoke([this](std::uint64_t, std::uint32_t, std::uint32_t poolId)
                                   { return poolId == kPoolId ? m_runtime : nullptr; }));
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
    EXPECT_CALL(m_controller, setDeliveryCallback(_));

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

TEST_F(VideoFrameCaptureAcquireTests, DeliveryCallbackResolvesTheFrame)
{
    captureIsCommitted();
    EXPECT_CALL(*m_session, acquireCurrentFrame(_))
        .WillOnce(Invoke(
            [&](CapturedFrame &frame)
            {
                m_records.makeExampleB(frame, 2, 9);
                frame.dmaBufObjectsFlushed = true;
                return CaptureStatus::OK;
            }));
    EXPECT_CALL(m_controller, setReplyFdsOwned(true));
    std::function<void(bool)> callback;
    EXPECT_CALL(m_controller, setDeliveryCallback(_))
        .WillOnce(Invoke([&](std::function<void(bool)> cb) { callback = cb; }));
    EXPECT_TRUE(acquire().dma_buf_objects_flushed());
    ASSERT_TRUE(callback);

    // not delivered: the HAL lock is released once and the invalidation stays pending
    EXPECT_CALL(*m_session, releaseFrame(_))
        .WillOnce(Invoke(
            [](CapturedFrame &frame)
            {
                EXPECT_EQ(frame.slotIndex, 2U);
                EXPECT_EQ(frame.acquisitionToken, 9U);
                return CaptureStatus::OK;
            }));
    callback(false);
    EXPECT_FALSE(m_runtime->isSlotLocked(2));
    EXPECT_TRUE(m_runtime->isFlushPending());
    callback(false); // resolved already: no second release
}

TEST_F(VideoFrameCaptureAcquireTests, PoolMetadataIsSentOncePerDescriptionWithoutDescriptors)
{
    captureIsCommitted();
    std::vector<std::function<void(bool)>> callbacks;
    EXPECT_CALL(m_controller, setReplyFdsOwned(true)).Times(3);
    EXPECT_CALL(m_controller, setDeliveryCallback(_))
        .Times(3)
        .WillRepeatedly(Invoke([&](std::function<void(bool)> registered) { callbacks.push_back(registered); }));
    EXPECT_CALL(*m_session, acquireCurrentFrame(_))
        .WillOnce(Invoke(
            [&](CapturedFrame &frame)
            {
                m_records.makeExampleB(frame, 0, 1);
                CaptureRecordFixtures::setSingleObjectId(frame, 100); // its own memory
                return CaptureStatus::OK;
            }))
        .WillOnce(Invoke(
            [&](CapturedFrame &frame)
            {
                m_records.makeExampleB(frame, 1, 2);
                CaptureRecordFixtures::setSingleObjectId(frame, 101); // its own memory
                return CaptureStatus::OK;
            }))
        .WillOnce(Invoke(
            [&](CapturedFrame &frame)
            {
                m_records.makeExampleB(frame, 2, 3);
                CaptureRecordFixtures::setSingleObjectId(frame, 102); // its own memory
                frame.backingSize.width = 1280; // a new backing size, with a visible region to match
                frame.visibleRegion.size.width = 1280;
                return CaptureStatus::OK;
            }));

    const auto kFirst = acquire();
    ASSERT_TRUE(kFirst.has_pool_metadata());
    EXPECT_EQ(kFirst.pool_metadata().pool_id(), kPoolId);
    EXPECT_EQ(kFirst.pool_metadata().slots_size(), 3);
    EXPECT_EQ(kFirst.pool_metadata().texture_width(), capture::test::kWidth);
    EXPECT_EQ(kFirst.exported_dma_bufs_size(), 1); // the only descriptors are the frame's own
    callbacks[0](true);

    EXPECT_FALSE(acquire().has_pool_metadata()); // unchanged
    callbacks[1](true);

    const auto kResized = acquire();
    ASSERT_TRUE(kResized.has_pool_metadata());
    EXPECT_EQ(kResized.pool_metadata().texture_width(), 1280U);
    callbacks[2](true);
}

class VideoFrameCaptureReleaseTests : public VideoFrameCaptureAcquireTests
{
protected:
    ReleaseVideoFrameResponse release(std::uint32_t poolId, std::uint32_t slot, std::uint64_t token)
    {
        ReleaseVideoFrameRequest request;
        request.set_capture_id(kCaptureId);
        request.set_pool_id(poolId);
        request.set_slot_index(slot);
        request.set_acquisition_token(token);
        ReleaseVideoFrameResponse response;
        EXPECT_CALL(m_controller, getClient()).WillOnce(Return(m_client));
        EXPECT_CALL(m_closure, Run());
        m_sut->releaseFrame(&m_controller, &request, &response, &m_closure);
        return response;
    }

    // A frame delivered to the client in slot 1 with token 42.
    void frameIsHeld()
    {
        captureIsCommitted();
        EXPECT_CALL(*m_session, acquireCurrentFrame(_))
            .WillOnce(Invoke(
                [&](CapturedFrame &frame)
                {
                    m_records.makeExampleB(frame, 1, 42);
                    return CaptureStatus::OK;
                }));
        EXPECT_CALL(m_controller, setReplyFdsOwned(true));
        EXPECT_CALL(m_controller, setDeliveryCallback(_))
            .WillOnce(Invoke([](std::function<void(bool)> callback) { callback(true); }));
        ASSERT_EQ(acquire().status(), VIDEO_FRAME_CAPTURE_STATUS_OK);
    }
};

TEST_F(VideoFrameCaptureReleaseTests, MatchingReleaseEndsTheLock)
{
    frameIsHeld();
    EXPECT_CALL(*m_session, releaseFrame(_))
        .WillOnce(Invoke(
            [](CapturedFrame &frame)
            {
                EXPECT_EQ(frame.slotIndex, 1U);
                EXPECT_EQ(frame.acquisitionToken, 42U);
                return CaptureStatus::OK;
            }));
    EXPECT_EQ(release(kPoolId, 1, 42).status(), VIDEO_FRAME_CAPTURE_STATUS_OK);
    EXPECT_FALSE(m_runtime->isSlotLocked(1));
    EXPECT_EQ(release(kPoolId, 1, 42).status(), VIDEO_FRAME_CAPTURE_STATUS_INVALID_ARGUMENT); // only once
}

TEST_F(VideoFrameCaptureReleaseTests, MismatchedReleasesAreInvalidAndKeepTheLock)
{
    frameIsHeld();
    EXPECT_CALL(*m_session, releaseFrame(_)).Times(0);
    EXPECT_EQ(release(kPoolId, 2, 42).status(), VIDEO_FRAME_CAPTURE_STATUS_INVALID_ARGUMENT);     // wrong slot
    EXPECT_EQ(release(kPoolId, 0, 42).status(), VIDEO_FRAME_CAPTURE_STATUS_INVALID_ARGUMENT);     // unlocked slot
    EXPECT_EQ(release(kPoolId, 1, 43).status(), VIDEO_FRAME_CAPTURE_STATUS_INVALID_ARGUMENT);     // wrong token
    EXPECT_EQ(release(kPoolId, 99, 42).status(), VIDEO_FRAME_CAPTURE_STATUS_INVALID_ARGUMENT);    // outside the pool
    EXPECT_EQ(release(kPoolId + 1, 1, 42).status(), VIDEO_FRAME_CAPTURE_STATUS_INVALID_ARGUMENT); // another pool
    EXPECT_TRUE(m_runtime->isSlotLocked(1));
}

TEST_F(VideoFrameCaptureReleaseTests, HalReleaseFailureKeepsTheRecord)
{
    frameIsHeld();
    EXPECT_CALL(*m_session, releaseFrame(_)).WillOnce(Return(CaptureStatus::NO_RESOURCES));
    EXPECT_EQ(release(kPoolId, 1, 42).status(), VIDEO_FRAME_CAPTURE_STATUS_NO_RESOURCES);
    EXPECT_TRUE(m_runtime->isSlotLocked(1));
}

TEST_F(VideoFrameCaptureReleaseTests, ReleaseWithoutACommittedSessionIsInvalid)
{
    EXPECT_CALL(m_service, sessionForRelease(1, kCaptureId, kPoolId)).WillOnce(Return(nullptr));
    EXPECT_EQ(release(kPoolId, 1, 42).status(), VIDEO_FRAME_CAPTURE_STATUS_INVALID_ARGUMENT);
}

TEST_F(VideoFrameCaptureReleaseTests, ReleaseReachesARetiredSession)
{
    frameIsHeld();
    m_runtime->retire(); // stop, FAILURE, pipeline or capture destruction
    EXPECT_CALL(*m_session, releaseFrame(_)).WillOnce(Return(CaptureStatus::OK));
    EXPECT_EQ(release(kPoolId, 1, 42).status(), VIDEO_FRAME_CAPTURE_STATUS_OK);
}

TEST_F(VideoFrameCaptureAcquireTests, PendingSendKeepsARetiredSessionUntilItsCallback)
{
    // the session's deletion is observable through a deleter on a dedicated runtime
    bool deleted{false};
    auto *session = new NiceMock<CaptureSessionMock>();
    capture::CapturePool pool;
    CaptureRecordFixtures::makeNv12Pool(pool, 3);
    auto runtime = std::make_shared<
        CaptureSessionRuntime>(firebolt::rialto::wrappers::CaptureSessionPtr{session,
                                                                             [&deleted](capture::ICaptureSession *doomed)
                                                                             {
                                                                                 deleted = true;
                                                                                 delete doomed;
                                                                             }},
                               kPoolId, pool);
    EXPECT_CALL(m_service, hasCapture(1, kCaptureId)).WillOnce(Return(true));
    EXPECT_CALL(m_service, captureRuntime(1, kCaptureId)).WillOnce(Return(runtime));
    EXPECT_CALL(*session, acquireCurrentFrame(_))
        .WillOnce(Invoke(
            [&](CapturedFrame &frame)
            {
                m_records.makeExampleB(frame, 0, 1);
                return CaptureStatus::OK;
            }));
    ON_CALL(*session, releaseFrame(_)).WillByDefault(Return(CaptureStatus::OK));
    EXPECT_CALL(m_controller, setReplyFdsOwned(true));
    std::function<void(bool)> callback;
    EXPECT_CALL(m_controller, setDeliveryCallback(_))
        .WillOnce(Invoke([&](std::function<void(bool)> registered) { callback = registered; }));
    ASSERT_EQ(acquire().status(), VIDEO_FRAME_CAPTURE_STATUS_OK);

    // the pipeline and the capture let go while the reply is still being sent
    std::weak_ptr<CaptureSessionRuntime> watched{runtime};
    runtime->retire();
    runtime->setSinkAtNull();
    runtime.reset();
    EXPECT_FALSE(watched.expired()); // the callback holds it
    EXPECT_FALSE(deleted);

    callback(false); // the send failed: released, and the last obligation goes
    EXPECT_TRUE(deleted);
    callback = nullptr;
    EXPECT_TRUE(watched.expired());
}
