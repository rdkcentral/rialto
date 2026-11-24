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

#include "VideoFrameCaptureIpc.h"
#include "IpcModuleBase.h"
#include <gtest/gtest.h>

#include <sys/mman.h>
#include <unistd.h>

using ::testing::Invoke;
using ::testing::Return;
using ::testing::WithArgs;

class VideoFrameCaptureIpcTest : public IpcModuleBase, public ::testing::Test
{
protected:
    std::unique_ptr<VideoFrameCaptureIpc> m_sut;

    void createVideoFrameCaptureIpc()
    {
        expectInitIpc();
        EXPECT_NO_THROW(m_sut = std::make_unique<VideoFrameCaptureIpc>(*m_ipcClientMock));
    }

public:
    void setCreateResponse(google::protobuf::Message *response, firebolt::rialto::VideoFrameCaptureWireStatus status,
                           bool withId)
    {
        auto *createResponse = dynamic_cast<firebolt::rialto::CreateVideoFrameCaptureResponse *>(response);
        createResponse->set_status(status);
        if (withId)
            createResponse->set_capture_id(7);
    }
};

TEST_F(VideoFrameCaptureIpcTest, ShouldThrowWhenChannelCannotBeAttached)
{
    expectInitIpcButAttachChannelFailure();
    EXPECT_THROW(m_sut = std::make_unique<VideoFrameCaptureIpc>(*m_ipcClientMock), std::runtime_error);
}

TEST_F(VideoFrameCaptureIpcTest, ShouldMapSuccessToCaptureId)
{
    createVideoFrameCaptureIpc();
    expectIpcApiCallSuccess();
    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("createVideoFrameCapture"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(
            Invoke([this](google::protobuf::Message *response)
                   { setCreateResponse(response, firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_OK, true); })));

    std::uint32_t captureId{0};
    EXPECT_TRUE(m_sut->createVideoFrameCapture(firebolt::rialto::GraphicsAPIType::GL, captureId));
    EXPECT_EQ(captureId, 7u);
}

TEST_F(VideoFrameCaptureIpcTest, ShouldMapUnsupportedToFailure)
{
    createVideoFrameCaptureIpc();
    expectIpcApiCallSuccess();
    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("createVideoFrameCapture"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(
            Invoke([this](google::protobuf::Message *response)
                   { setCreateResponse(response, firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_UNSUPPORTED, false); })));

    std::uint32_t captureId{0};
    EXPECT_FALSE(m_sut->createVideoFrameCapture(firebolt::rialto::GraphicsAPIType::GL, captureId));
}

TEST_F(VideoFrameCaptureIpcTest, ShouldMapFailedCallToFailure)
{
    createVideoFrameCaptureIpc();
    expectIpcApiCallFailure();
    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("createVideoFrameCapture"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()));

    std::uint32_t captureId{0};
    EXPECT_FALSE(m_sut->createVideoFrameCapture(firebolt::rialto::GraphicsAPIType::GL, captureId));
}

TEST_F(VideoFrameCaptureIpcTest, ShouldFailCreateWhenDisconnected)
{
    createVideoFrameCaptureIpc();
    expectIpcApiCallDisconnected();

    std::uint32_t captureId{0};
    EXPECT_FALSE(m_sut->createVideoFrameCapture(firebolt::rialto::GraphicsAPIType::GL, captureId));
}

TEST_F(VideoFrameCaptureIpcTest, ShouldDestroy)
{
    createVideoFrameCaptureIpc();
    expectIpcApiCallSuccess();
    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("destroyVideoFrameCapture"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()));
    EXPECT_TRUE(m_sut->destroyVideoFrameCapture(7));
}

TEST_F(VideoFrameCaptureIpcTest, ShouldFailDestroyWhenCallFails)
{
    createVideoFrameCaptureIpc();
    expectIpcApiCallFailure();
    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("destroyVideoFrameCapture"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()));
    EXPECT_FALSE(m_sut->destroyVideoFrameCapture(7));
}

namespace
{
void fillFrame(google::protobuf::Message *message, int fd)
{
    auto *response = dynamic_cast<firebolt::rialto::AcquireCurrentVideoFrameResponse *>(message);
    response->set_status(firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_OK);
    response->set_pool_id(4);
    response->set_slot_index(1);
    response->set_acquisition_token(99);
    response->set_presentation_time_ns(40000000);
    response->mutable_visible_region()->set_width(1280);
    response->mutable_visible_region()->set_height(720);
    response->set_dma_buf_objects_flushed(true);
    auto *metadata = response->mutable_frame_metadata();
    metadata->set_drm_format(0x3231564e);
    metadata->set_backing_width(1920);
    metadata->set_backing_height(1088);
    auto *object = metadata->add_dma_buf_objects();
    object->set_object_id(101);
    object->set_size_bytes(4096);
    for (uint32_t index = 0; index < 2; ++index)
    {
        auto *plane = metadata->add_planes();
        plane->set_plane_index(index);
        plane->set_dma_buf_object_id(101);
        plane->set_offset_bytes(index * 2048);
        plane->set_length_bytes(2048);
        plane->set_stride_bytes(1920);
    }
    metadata->mutable_color_description()->set_ycbcr_color_space(
        firebolt::rialto::VIDEO_FRAME_CAPTURE_YCBCR_COLOR_SPACE_BT709);
    auto *exported = response->add_exported_dma_bufs();
    exported->set_dma_buf_object_id(101);
    exported->set_fd(fd);
    auto *pool = response->mutable_pool_metadata();
    pool->set_pool_id(4);
    pool->add_slots();
    pool->add_slots();
    pool->add_slots();
}
} // namespace

TEST_F(VideoFrameCaptureIpcTest, ShouldTranslateAcquiredFrame)
{
    createVideoFrameCaptureIpc();
    expectIpcApiCallSuccess();
    const int kFd = memfd_create("ipc-test", MFD_CLOEXEC);
    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("acquireCurrentFrame"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke([&](google::protobuf::Message *response) { fillFrame(response, kFd); })));

    const CaptureAcquireResult kResult = m_sut->acquireCurrentFrame(7);
    ASSERT_EQ(kResult.status, CaptureAcquireStatus::OK);
    EXPECT_EQ(kResult.description.poolId, 4u);
    EXPECT_EQ(kResult.description.slotIndex, 1u);
    EXPECT_EQ(kResult.description.acquisitionToken, 99u);
    EXPECT_EQ(kResult.description.visibleWidth, 1280u);
    EXPECT_EQ(kResult.description.backingHeight, 1088u);
    EXPECT_TRUE(kResult.description.dmaBufObjectsFlushed);
    ASSERT_EQ(kResult.description.planes.size(), 2u);
    EXPECT_EQ(kResult.description.planes[1].offsetBytes, 2048u);
    EXPECT_EQ(kResult.description.colour.matrix,
              static_cast<uint32_t>(firebolt::rialto::VIDEO_FRAME_CAPTURE_YCBCR_COLOR_SPACE_BT709));
    ASSERT_EQ(kResult.fds.size(), 1u);
    EXPECT_EQ(kResult.fds[0].objectId, 101u);
    EXPECT_EQ(kResult.fds[0].fd, kFd); // handed to the caller, not closed
    ASSERT_TRUE(kResult.pool.has_value());
    EXPECT_EQ(kResult.pool->slotCount, 3u);
    close(kFd);
}

TEST_F(VideoFrameCaptureIpcTest, ShouldMapAcquireStatuses)
{
    createVideoFrameCaptureIpc();
    const std::vector<std::pair<firebolt::rialto::VideoFrameCaptureWireStatus, CaptureAcquireStatus>> kCases{
        {firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_NO_NEW_FRAME, CaptureAcquireStatus::NO_NEW_FRAME},
        {firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_NO_FRAME, CaptureAcquireStatus::NO_FRAME},
        {firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_NO_RESOURCES, CaptureAcquireStatus::NO_RESOURCES},
        {firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_FATAL_ERROR, CaptureAcquireStatus::FATAL_ERROR},
    };
    for (const auto &kCase : kCases)
    {
        expectIpcApiCallSuccess();
        EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("acquireCurrentFrame"), m_controllerMock.get(), _, _,
                                               m_blockingClosureMock.get()))
            .WillOnce(WithArgs<3>(Invoke(
                [&](google::protobuf::Message *message) {
                    dynamic_cast<firebolt::rialto::AcquireCurrentVideoFrameResponse *>(message)->set_status(kCase.first);
                })));
        EXPECT_EQ(m_sut->acquireCurrentFrame(7).status, kCase.second);
    }
}

TEST_F(VideoFrameCaptureIpcTest, ShouldReportIpcFailureForFailedAcquire)
{
    createVideoFrameCaptureIpc();
    expectIpcApiCallFailure();
    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("acquireCurrentFrame"), m_controllerMock.get(), _, _,
                                           m_blockingClosureMock.get()));
    EXPECT_EQ(m_sut->acquireCurrentFrame(7).status, CaptureAcquireStatus::IPC_FAILURE);
}

TEST_F(VideoFrameCaptureIpcTest, ShouldReleaseFrame)
{
    createVideoFrameCaptureIpc();
    expectIpcApiCallSuccess();
    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("releaseFrame"), m_controllerMock.get(), _, _, m_blockingClosureMock.get()))
        .WillOnce(WithArgs<2, 3>(Invoke(
            [](const google::protobuf::Message *message, google::protobuf::Message *response)
            {
                auto *request = dynamic_cast<const firebolt::rialto::ReleaseVideoFrameRequest *>(message);
                EXPECT_EQ(request->capture_id(), 7u);
                EXPECT_EQ(request->pool_id(), 4u);
                EXPECT_EQ(request->slot_index(), 1u);
                EXPECT_EQ(request->acquisition_token(), 99u);
                dynamic_cast<firebolt::rialto::ReleaseVideoFrameResponse *>(response)->set_status(
                    firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_OK);
            })));
    EXPECT_TRUE(m_sut->releaseFrame(7, 4, 1, 99));
}

TEST_F(VideoFrameCaptureIpcTest, ShouldGetPool)
{
    createVideoFrameCaptureIpc();
    expectIpcApiCallSuccess();
    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("getPool"), m_controllerMock.get(), _, _, m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(
            [](google::protobuf::Message *message)
            {
                auto *response = dynamic_cast<firebolt::rialto::GetVideoFrameCapturePoolResponse *>(message);
                response->set_status(firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_OK);
                response->set_pool_id(4);
                response->add_slots();
                response->add_slots();
            })));
    CapturePoolDescription pool;
    EXPECT_TRUE(m_sut->getPool(7, pool));
    EXPECT_EQ(pool.poolId, 4u);
    EXPECT_EQ(pool.slotCount, 2u);
}
