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
#include "VulkanVideoFrameCapture.h"

#include <vector>

using namespace firebolt::rialto;
using namespace firebolt::rialto::client;
using ::testing::_;
using ::testing::DoAll;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SetArgReferee;

/**
 * A Vulkan capture's capabilities: the server's limits plus the formats this device can import (R8 4.3).
 */
class VulkanCaptureCapabilitiesTest : public VulkanImportTestBase
{
protected:
    static constexpr std::uint32_t kCaptureId{5};
    static constexpr std::uint32_t kUnknownFourcc{0x31313131};

    void serverReports(const std::vector<std::uint32_t> &fourccs, const std::vector<std::uint64_t> &modifiers)
    {
        CaptureCapabilitiesReply reply;
        reply.maxConcurrentFrames = 3;
        reply.maxResourcesPerFrame = 1;
        reply.drmFourccs = fourccs;
        reply.drmModifiers = modifiers;
        EXPECT_CALL(*m_ipc, getCapabilities(kCaptureId, GraphicsAPIType::VULKAN, _))
            .WillOnce(DoAll(SetArgReferee<2>(reply), Return(VideoFrameCaptureStatus::OK)));
    }

    VideoFrameCaptureStatus query(VulkanVideoFrameCaptureCaps &caps)
    {
        VideoFrameCaptureStatus status{VideoFrameCaptureStatus::FATAL_ERROR};
        const VideoFrameCaptureStatus kReturned{m_sut->getCapabilities(caps, status)};
        EXPECT_EQ(kReturned, status);
        return kReturned;
    }

    std::shared_ptr<NiceMock<VideoFrameCaptureIpcMock>> m_ipc{std::make_shared<NiceMock<VideoFrameCaptureIpcMock>>()};
    std::shared_ptr<VulkanVideoFrameCapture> m_sut{
        std::make_shared<VulkanVideoFrameCapture>(m_ipc, kCaptureId, m_context,
                                                  std::make_shared<VulkanFrameImporter>(m_context))};
};

TEST_F(VulkanCaptureCapabilitiesTest, SupportedNv12IsListed)
{
    serverReports({kNv12}, {0});
    VulkanVideoFrameCaptureCaps caps{};
    ASSERT_EQ(query(caps), VideoFrameCaptureStatus::OK);
    EXPECT_EQ(caps.size, sizeof(VulkanVideoFrameCaptureCaps));
    EXPECT_EQ(caps.common.size, sizeof(VideoFrameCaptureCaps));
    EXPECT_EQ(caps.common.maxConcurrentFrames, 3u);
    EXPECT_EQ(caps.common.maxResourcesPerFrame, 1u);
    EXPECT_EQ(caps.supportedFormats, std::vector<int32_t>{VK_FORMAT_G8_B8R8_2PLANE_420_UNORM});
}

TEST_F(VulkanCaptureCapabilitiesTest, FormatTheDeviceCannotImportIsOmitted)
{
    ON_CALL(*m_api, getPhysicalDeviceImageFormatProperties2(_, _, _)).WillByDefault(Return(VK_ERROR_FORMAT_NOT_SUPPORTED));
    serverReports({kNv12}, {0});
    VulkanVideoFrameCaptureCaps caps{};
    ASSERT_EQ(query(caps), VideoFrameCaptureStatus::OK);
    EXPECT_TRUE(caps.supportedFormats.empty());
    EXPECT_EQ(caps.common.maxConcurrentFrames, 3u);
}

TEST_F(VulkanCaptureCapabilitiesTest, ModifierTheDeviceCannotSampleIsOmitted)
{
    ON_CALL(*m_api, getPhysicalDeviceFormatProperties2(_, _, _))
        .WillByDefault(::testing::Invoke(
            [](VkPhysicalDevice, VkFormat, VkFormatProperties2 *properties)
            { static_cast<VkDrmFormatModifierPropertiesListEXT *>(properties->pNext)->drmFormatModifierCount = 0; }));
    serverReports({kNv12}, {0});
    VulkanVideoFrameCaptureCaps caps{};
    ASSERT_EQ(query(caps), VideoFrameCaptureStatus::OK);
    EXPECT_TRUE(caps.supportedFormats.empty());
}

TEST_F(VulkanCaptureCapabilitiesTest, UnknownFourccIsOmitted)
{
    serverReports({kUnknownFourcc, kAr24}, {0, 0});
    VulkanVideoFrameCaptureCaps caps{};
    ASSERT_EQ(query(caps), VideoFrameCaptureStatus::OK);
    EXPECT_EQ(caps.supportedFormats, std::vector<int32_t>{VK_FORMAT_B8G8R8A8_UNORM});
}

TEST_F(VulkanCaptureCapabilitiesTest, FailureLeavesTheRecordUnchanged)
{
    EXPECT_CALL(*m_ipc, getCapabilities(kCaptureId, GraphicsAPIType::VULKAN, _))
        .WillOnce(Return(VideoFrameCaptureStatus::SESSION_NOT_READY));
    VulkanVideoFrameCaptureCaps caps{};
    caps.supportedFormats = {42};
    caps.common.maxConcurrentFrames = 77;
    EXPECT_EQ(query(caps), VideoFrameCaptureStatus::SESSION_NOT_READY);
    EXPECT_EQ(caps.supportedFormats, std::vector<int32_t>{42});
    EXPECT_EQ(caps.common.maxConcurrentFrames, 77u);
}
