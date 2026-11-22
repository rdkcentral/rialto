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

#include "VideoFrameCaptureIpcMock.h"
#include "VulkanImportTestBase.h"
#include "VulkanVideoFrame.h"
#include "VulkanVideoFrameCapture.h"

#include <sys/mman.h>

#include <algorithm>
#include <string>
#include <thread>

using namespace firebolt::rialto;
using namespace firebolt::rialto::client;
using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::StrictMock;

/**
 * Queue-family transfers and deferred slot releases of a Vulkan capture (R13 design D4, D5), over the mock table.
 */
class VulkanCaptureTransferTest : public VulkanImportTestBase
{
protected:
    static constexpr std::uint32_t kCaptureId{5};
    static constexpr std::uint32_t kPoolId{2};
    static constexpr std::uint32_t kFamily{2};

    VulkanCaptureTransferTest()
    {
        ON_CALL(*m_api, cmdPipelineBarrier(_, _, _, _, _))
            .WillByDefault(Invoke([this](VkCommandBuffer, VkPipelineStageFlags, VkPipelineStageFlags, uint32_t count,
                                         const VkImageMemoryBarrier *barriers)
                                  { m_barriers.insert(m_barriers.end(), barriers, barriers + count); }));
        ON_CALL(*m_api, queueSubmit(_, _, _, _))
            .WillByDefault(Invoke(
                [this](VkQueue queue, uint32_t, const VkSubmitInfo *, VkFence)
                {
                    EXPECT_EQ(queue, reinterpret_cast<VkQueue>(0x30));
                    EXPECT_EQ(std::this_thread::get_id(), m_queueThread) << "submitted on a releasing thread";
                    ++m_submits;
                    return VK_SUCCESS;
                }));
        ON_CALL(*m_api, getFenceStatus(_, _))
            .WillByDefault(Invoke([this](VkDevice, VkFence) { return m_fenceSignalled ? VK_SUCCESS : VK_NOT_READY; }));
    }

    CaptureAcquireResult okResult(std::uint32_t slot, std::uint64_t token)
    {
        CaptureAcquireResult result;
        result.status = CaptureAcquireStatus::OK;
        result.description = frame(kNv12, {100 + slot, 100 + slot});
        result.description.poolId = kPoolId;
        result.description.slotIndex = slot;
        result.description.acquisitionToken = token;
        result.fds = {{100 + slot, memfd_create("vulkan-transfer-test", MFD_CLOEXEC)}};
        result.pool = CapturePoolDescription{kPoolId, 3};
        return result;
    }
    static CaptureAcquireResult noNewFrame()
    {
        CaptureAcquireResult result;
        result.status = CaptureAcquireStatus::NO_NEW_FRAME;
        return result;
    }

    std::shared_ptr<VulkanVideoFrameCapture> makeCapture()
    {
        const VkImageLayout kLayout{m_context->requiredImageLayout()};
        CaptureCore::FrameMaker maker{[kLayout](const CaptureFrameDescription &description,
                                                const std::shared_ptr<ImportCacheEntry> &import,
                                                std::unique_ptr<FrameLease> lease) -> std::shared_ptr<IVideoFrame> {
            return std::make_shared<VulkanVideoFrame>(description, import, std::move(lease), kLayout);
        }};
        return std::make_shared<VulkanVideoFrameCapture>(m_ipc, kCaptureId, m_context,
                                                         std::make_shared<VulkanFrameImporter>(m_context), maker);
    }

    static VkImage imageOf(const std::shared_ptr<IVulkanVideoFrame> &frame) { return frame->images().at(0).image; }

    bool isAcquire(const VkImageMemoryBarrier &barrier, VkImage image) const
    {
        return barrier.image == image && barrier.srcQueueFamilyIndex == VK_QUEUE_FAMILY_FOREIGN_EXT &&
               barrier.dstQueueFamilyIndex == kFamily && barrier.oldLayout == VK_IMAGE_LAYOUT_GENERAL &&
               barrier.newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }
    bool isRelease(const VkImageMemoryBarrier &barrier, VkImage image) const
    {
        return barrier.image == image && barrier.srcQueueFamilyIndex == kFamily &&
               barrier.dstQueueFamilyIndex == VK_QUEUE_FAMILY_FOREIGN_EXT &&
               barrier.oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
               barrier.newLayout == VK_IMAGE_LAYOUT_GENERAL;
    }

    std::shared_ptr<StrictMock<VideoFrameCaptureIpcMock>> m_ipc{std::make_shared<StrictMock<VideoFrameCaptureIpcMock>>()};
    std::vector<VkImageMemoryBarrier> m_barriers;
    int m_submits{0};
    bool m_fenceSignalled{true};
    std::thread::id m_queueThread{std::this_thread::get_id()};
};

TEST_F(VulkanCaptureTransferTest, ChangedFrameGetsOneAcquireBarrierAndUnchangedNone)
{
    EXPECT_CALL(*m_ipc, acquireCurrentFrame(kCaptureId)).WillOnce(Return(okResult(0, 1))).WillOnce(Return(noNewFrame()));
    auto capture = makeCapture();

    VulkanFrameAcquisition first = capture->acquireCurrentFrame();
    ASSERT_TRUE(first.changed);
    ASSERT_NE(first.frame, nullptr);
    EXPECT_EQ(first.frame->imageLayout(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    EXPECT_EQ(m_submits, 1);
    ASSERT_EQ(m_barriers.size(), 1U);
    EXPECT_TRUE(isAcquire(m_barriers[0], imageOf(first.frame)));
    EXPECT_EQ(m_barriers[0].dstAccessMask, static_cast<VkAccessFlags>(VK_ACCESS_SHADER_READ_BIT));

    VulkanFrameAcquisition second = capture->acquireCurrentFrame();
    EXPECT_FALSE(second.changed);
    EXPECT_EQ(m_submits, 1) << "no barrier for an unchanged frame";
    EXPECT_EQ(m_barriers.size(), 1U);

    // teardown: the frames are released once each
    EXPECT_CALL(*m_ipc, destroyVideoFrameCapture(kCaptureId)).WillOnce(Return(true));
    EXPECT_CALL(*m_ipc, releaseFrame(kCaptureId, kPoolId, 0, 1)).WillOnce(Return(true));
    capture.reset();
    first.frame.reset();
    second.frame.reset();
}

TEST_F(VulkanCaptureTransferTest, ReleaseIsSentOnlyAfterItsBarrierFenceSignals)
{
    EXPECT_CALL(*m_ipc, acquireCurrentFrame(kCaptureId))
        .WillOnce(Return(okResult(0, 1)))
        .WillOnce(Return(okResult(1, 2)))
        .WillRepeatedly(Return(noNewFrame()));
    auto capture = makeCapture();

    std::shared_ptr<IVulkanVideoFrame> first = capture->acquireCurrentFrame().frame;
    ASSERT_NE(first, nullptr);
    const VkImage kFirstImage{imageOf(first)};
    std::shared_ptr<IVulkanVideoFrame> second = capture->acquireCurrentFrame().frame; // the capture drops first
    ASSERT_NE(second, nullptr);
    EXPECT_EQ(m_submits, 2);

    // the application drops the first frame on another thread: no submission, no release yet
    std::thread releaser{[&first]() { first.reset(); }};
    releaser.join();
    EXPECT_EQ(m_submits, 2);

    // next acquisition: the release barrier is submitted; its fence has not signalled, so no slot release
    m_fenceSignalled = false;
    m_barriers.clear();
    capture->acquireCurrentFrame();
    EXPECT_EQ(m_submits, 3);
    ASSERT_EQ(m_barriers.size(), 1U);
    EXPECT_TRUE(isRelease(m_barriers[0], kFirstImage));
    ::testing::Mock::VerifyAndClearExpectations(m_ipc.get()); // no releaseFrame so far

    // the fence signals: the next acquisition sends the release, once, and submits nothing
    m_fenceSignalled = true;
    EXPECT_CALL(*m_ipc, acquireCurrentFrame(kCaptureId)).WillRepeatedly(Return(noNewFrame()));
    EXPECT_CALL(*m_ipc, releaseFrame(kCaptureId, kPoolId, 0, 1)).WillOnce(Return(true));
    capture->acquireCurrentFrame();
    capture->acquireCurrentFrame();
    EXPECT_EQ(m_submits, 3);

    EXPECT_CALL(*m_ipc, destroyVideoFrameCapture(kCaptureId)).WillOnce(Return(true));
    EXPECT_CALL(*m_ipc, releaseFrame(kCaptureId, kPoolId, 1, 2)).WillOnce(Return(true));
    capture.reset();
    second.reset();
}

TEST_F(VulkanCaptureTransferTest, ReleasedImageIsKeptUntilItsReleaseIsSent)
{
    // slot count 1: the third import evicts the first, whose release is still in flight
    auto oneSlot = [this](std::uint32_t slot, std::uint64_t token)
    {
        auto result = okResult(slot, token);
        result.pool->slotCount = 1;
        return result;
    };
    EXPECT_CALL(*m_ipc, acquireCurrentFrame(kCaptureId))
        .WillOnce(Return(oneSlot(0, 1)))
        .WillOnce(Return(oneSlot(1, 2)))
        .WillOnce(Return(oneSlot(2, 3)))
        .WillRepeatedly(Return(noNewFrame()));
    auto capture = makeCapture();
    m_fenceSignalled = false;
    const VkImage kFirstImage{imageOf(capture->acquireCurrentFrame().frame)};
    capture->acquireCurrentFrame(); // drops the first frame: its release barrier is submitted
    capture->acquireCurrentFrame(); // evicts the first import while that barrier is in flight
    EXPECT_EQ(m_live.count(reinterpret_cast<std::uintptr_t>(kFirstImage)), 1U) << "destroyed under its barrier";

    m_fenceSignalled = true;
    EXPECT_CALL(*m_ipc, releaseFrame(kCaptureId, kPoolId, 0, 1)).WillOnce(Return(true));
    EXPECT_CALL(*m_ipc, releaseFrame(kCaptureId, kPoolId, 1, 2)).WillOnce(Return(true));
    capture->acquireCurrentFrame(); // sends the releases
    capture->acquireCurrentFrame(); // drains: now the first import can go
    EXPECT_EQ(m_live.count(reinterpret_cast<std::uintptr_t>(kFirstImage)), 0U);

    EXPECT_CALL(*m_ipc, destroyVideoFrameCapture(kCaptureId)).WillOnce(Return(true));
    EXPECT_CALL(*m_ipc, releaseFrame(kCaptureId, kPoolId, 2, 3)).WillOnce(Return(true));
    capture.reset();
}

TEST_F(VulkanCaptureTransferTest, TeardownWaitsDestroysInOrderThenReleasesEverySlotOnce)
{
    m_context.reset(); // replaced below by a context whose objects are tracked too
    std::vector<std::string> events;
    auto log = [&events](const char *event) { events.emplace_back(event); };
    ON_CALL(*m_api, createCommandPool(_, _, _))
        .WillByDefault(
            Invoke([this](VkDevice, const VkCommandPoolCreateInfo *, VkCommandPool *pool) { return created(pool); }));
    ON_CALL(*m_api, createFence(_, _, _))
        .WillByDefault(Invoke([this](VkDevice, const VkFenceCreateInfo *, VkFence *fence) { return created(fence); }));
    ON_CALL(*m_api, destroyCommandPool(_, _)).WillByDefault(Invoke([this](VkDevice, VkCommandPool h) { destroyed(h); }));
    ON_CALL(*m_api, destroyFence(_, _)).WillByDefault(Invoke([this](VkDevice, VkFence h) { destroyed(h); }));
    ON_CALL(*m_api, waitForFences(_, _, _, _, _))
        .WillByDefault(Invoke(
            [&log](VkDevice, uint32_t, const VkFence *, VkBool32, uint64_t)
            {
                log("wait");
                return VK_SUCCESS;
            }));
    ON_CALL(*m_api, destroyImageView(_, _))
        .WillByDefault(Invoke(
            [this, &log](VkDevice, VkImageView h)
            {
                log("view");
                destroyed(h);
            }));
    ON_CALL(*m_api, destroySampler(_, _))
        .WillByDefault(Invoke(
            [this, &log](VkDevice, VkSampler h)
            {
                log("sampler");
                destroyed(h);
            }));
    ON_CALL(*m_api, destroySamplerYcbcrConversion(_, _))
        .WillByDefault(Invoke(
            [this, &log](VkDevice, VkSamplerYcbcrConversion h)
            {
                log("sampler");
                destroyed(h);
            }));
    ON_CALL(*m_api, destroyImage(_, _))
        .WillByDefault(Invoke(
            [this, &log](VkDevice, VkImage h)
            {
                log("image");
                destroyed(h);
            }));
    ON_CALL(*m_api, freeMemory(_, _))
        .WillByDefault(Invoke(
            [this, &log](VkDevice, VkDeviceMemory h)
            {
                log("memory");
                destroyed(h);
            }));
    m_context = VulkanDeviceContext::create(m_api, reinterpret_cast<VkPhysicalDevice>(0x10),
                                            reinterpret_cast<VkDevice>(0x20), reinterpret_cast<VkQueue>(0x30), kFamily,
                                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    ASSERT_NE(m_context, nullptr);
    EXPECT_CALL(*m_api, freeCommandBuffers(_, _, VulkanDeviceContext::kRingSize, _)).Times(1);

    EXPECT_CALL(*m_ipc, acquireCurrentFrame(kCaptureId))
        .WillOnce(Return(okResult(0, 1)))
        .WillOnce(Return(okResult(1, 2)))
        .WillRepeatedly(Return(noNewFrame()));
    auto capture = makeCapture();
    m_fenceSignalled = false;
    std::shared_ptr<IVulkanVideoFrame> first = capture->acquireCurrentFrame().frame;
    std::shared_ptr<IVulkanVideoFrame> second = capture->acquireCurrentFrame().frame;
    first.reset();
    capture->acquireCurrentFrame(); // first's release barrier is in flight; its fence has not signalled

    // every slot is released exactly once, after every object is destroyed
    EXPECT_CALL(*m_ipc, destroyVideoFrameCapture(kCaptureId)).WillOnce(Return(true));
    EXPECT_CALL(*m_ipc, releaseFrame(kCaptureId, kPoolId, 0, 1))
        .WillOnce(Invoke(
            [&log](std::uint32_t, std::uint32_t, std::uint32_t, std::uint64_t)
            {
                log("release");
                return true;
            }));
    EXPECT_CALL(*m_ipc, releaseFrame(kCaptureId, kPoolId, 1, 2))
        .WillOnce(Invoke(
            [&log](std::uint32_t, std::uint32_t, std::uint32_t, std::uint64_t)
            {
                log("release");
                return true;
            }));
    capture.reset();
    EXPECT_TRUE(events.empty()) << "the application still holds a frame, so nothing is torn down yet";
    second.reset(); // the last frame: its release is pending with no barrier, and the importer goes

    // order: wait for Rialto's fences, views, samplers and conversions, images, memory, then the releases
    const std::vector<std::string> kOrder{"wait", "view", "sampler", "image", "memory", "release"};
    std::vector<std::string> phases;
    for (const std::string &kEvent : events)
    {
        if (phases.empty() || phases.back() != kEvent)
            phases.push_back(kEvent);
    }
    EXPECT_EQ(phases, kOrder);
    EXPECT_EQ(std::count(events.begin(), events.end(), "release"), 2);
    EXPECT_EQ(std::count(events.begin(), events.end(), "image"), 2);

    m_context.reset(); // the last reference: the ring and pool go
    EXPECT_TRUE(m_live.empty()) << m_live.size() << " objects leaked";
}
