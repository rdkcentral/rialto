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

#include "FakeVulkanControl.h"
#include "GlCaptureFixture.h"

#include <vulkan/vulkan_core.h>

namespace firebolt::rialto::client::ct
{
/**
 * A Vulkan capture end to end in the client: the server stub answers with memfd stand-ins for DMA-BUFs, and frames
 * are imported through the fake libvulkan, which counts live objects, submissions and barriers and signals every
 * fence at once.
 */
class VulkanVideoFrameCaptureTest : public GlCaptureFixture
{
public:
    ~VulkanVideoFrameCaptureTest()
    {
        if (m_fakeVulkan)
            dlclose(m_fakeVulkan);
    }

    std::shared_ptr<IVulkanVideoFrameCapture> createVulkanCapture()
    {
        EXPECT_CALL(*m_videoFrameCaptureModuleMock, createVideoFrameCapture(_, _, _, _))
            .WillOnce(Invoke(
                [](::google::protobuf::RpcController *, const CreateVideoFrameCaptureRequest *request,
                   CreateVideoFrameCaptureResponse *response, ::google::protobuf::Closure *done)
                {
                    EXPECT_EQ(request->api(), GRAPHICS_API_TYPE_VULKAN);
                    response->set_status(VIDEO_FRAME_CAPTURE_STATUS_OK);
                    response->set_capture_id(kCaptureId);
                    done->Run();
                }));
        auto capture = IVideoFrameCaptureFactory::createFactory()
                           ->createVulkanVideoFrameCapture(reinterpret_cast<VkPhysicalDevice>(0x10),
                                                           reinterpret_cast<VkDevice>(0x20),
                                                           reinterpret_cast<VkQueue>(0x30), kQueueFamily,
                                                           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        m_fakeVulkan = dlopen(FAKE_VULKAN_PATH, RTLD_NOW | RTLD_NOLOAD);
        EXPECT_NE(m_fakeVulkan, nullptr);
        if (m_fakeVulkan)
        {
            m_liveObjects = reinterpret_cast<int (*)()>(dlsym(m_fakeVulkan, "fakeVulkanLiveObjects"));
            m_submits = reinterpret_cast<int (*)()>(dlsym(m_fakeVulkan, "fakeVulkanSubmitCount"));
            m_barriers = reinterpret_cast<int (*)()>(dlsym(m_fakeVulkan, "fakeVulkanBarrierCount"));
        }
        return capture;
    }

    static constexpr std::uint32_t kQueueFamily{1};
    void *m_fakeVulkan{nullptr};
    int (*m_liveObjects)(){nullptr};
    int (*m_submits)(){nullptr};
    int (*m_barriers)(){nullptr};
};

/*
 * Component Test: Vulkan capture lifecycle without leaks
 * Test Objective:
 *  Check acquisition, a colour change, release on another thread, a further acquisition and teardown, and that no
 *  descriptor or Vulkan object is left afterwards.
 *
 * Test Steps:
 *  Step 1: Create a Vulkan capture and acquire a BT709 frame.
 *   Expect registration as VULKAN, a frame in the required layout with a sampler and conversion, and one submission
 *   carrying its acquire barrier.
 *  Step 2: Acquire a BT601 frame while the first is held.
 *   Expect a new sampler and conversion, the first frame keeping its own, and one more acquire barrier.
 *  Step 3: Drop the first frame on another thread.
 *   Expect no submission and no slot release yet.
 *  Step 4: Acquire again (no new frame).
 *   Expect the first frame's release barrier to be submitted and, its fence having signalled, its slot released.
 *  Step 5: Destroy the capture and drop the last frame.
 *   Expect the capture's destruction and the last slot's release.
 *
 * Expected Results:
 *  No Vulkan object created by Rialto remains, and every descriptor of the test's frames is closed.
 *
 * Code:
 */
TEST_F(VulkanVideoFrameCaptureTest, LifecycleWithoutLeaks)
{
    // Step 1: create and acquire a BT709 frame
    auto capture = createVulkanCapture();
    ASSERT_NE(capture, nullptr);
    ASSERT_NE(m_liveObjects, nullptr);
    EXPECT_EQ(capture->apiType(), GraphicsAPIType::VULKAN);
    const int kSubmitsBefore{m_submits()};
    serverWillReturnFrame(0, 1, false, 0, VIDEO_FRAME_CAPTURE_YCBCR_COLOR_SPACE_BT709);
    VulkanFrameAcquisition bt709 = capture->acquireCurrentFrame();
    ASSERT_TRUE(bt709.changed);
    ASSERT_NE(bt709.frame, nullptr);
    EXPECT_EQ(bt709.frame->imageLayout(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    EXPECT_EQ(bt709.frame->imageFormat(), VK_FORMAT_G8_B8R8_2PLANE_420_UNORM);
    ASSERT_EQ(bt709.frame->images().size(), 1U);
    const VulkanImageResource kBt709Resource{bt709.frame->images()[0]};
    EXPECT_NE(kBt709Resource.sampler, VK_NULL_HANDLE);
    EXPECT_NE(kBt709Resource.ycbcrConversion, VK_NULL_HANDLE);
    EXPECT_EQ(m_submits(), kSubmitsBefore + 1);
    EXPECT_EQ(m_barriers(), 1);

    // Step 2: a BT601 frame while the BT709 one is held
    serverWillReturnFrame(1, 2, false, 0, VIDEO_FRAME_CAPTURE_YCBCR_COLOR_SPACE_BT601);
    VulkanFrameAcquisition bt601 = capture->acquireCurrentFrame();
    ASSERT_TRUE(bt601.changed);
    ASSERT_NE(bt601.frame, nullptr);
    EXPECT_NE(bt601.frame->images()[0].sampler, kBt709Resource.sampler);
    EXPECT_NE(bt601.frame->images()[0].ycbcrConversion, kBt709Resource.ycbcrConversion);
    EXPECT_EQ(bt709.frame->images()[0].sampler, kBt709Resource.sampler);
    EXPECT_EQ(m_submits(), kSubmitsBefore + 2);
    EXPECT_EQ(m_barriers(), 2);

    // Step 3: drop the BT709 frame on another thread: nothing is submitted or released there
    std::thread releaser{[&bt709]() { bt709.frame.reset(); }};
    releaser.join();
    EXPECT_EQ(m_submits(), kSubmitsBefore + 2);

    // Step 4: the next acquisition submits its release barrier and, the fence signalled, releases its slot
    serverWillReturnNoNewFrame();
    serverWillRelease(0, 1);
    VulkanFrameAcquisition unchanged = capture->acquireCurrentFrame();
    EXPECT_FALSE(unchanged.changed);
    EXPECT_EQ(m_submits(), kSubmitsBefore + 3);
    EXPECT_EQ(m_barriers(), 3);

    // Step 5: teardown
    serverWillDestroyCapture();
    serverWillRelease(1, 2);
    capture.reset();
    bt601.frame.reset();
    unchanged.frame.reset();

    EXPECT_EQ(m_liveObjects(), 0);
    EXPECT_EQ(openTestMemfds(), 0);
}
} // namespace firebolt::rialto::client::ct
