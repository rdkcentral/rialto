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

#include "VulkanVideoFrameCapture.h"
#include "CaptureCapabilities.h"
#include "RialtoClientLogging.h"
#include "VulkanFrameImporter.h"
#include "VulkanVideoFrame.h"

#include <algorithm>

namespace firebolt::rialto::client
{
VulkanVideoFrameCapture::VulkanVideoFrameCapture(const std::shared_ptr<IVideoFrameCaptureIpc> &videoFrameCaptureIpc,
                                                 std::uint32_t captureId,
                                                 const std::shared_ptr<VulkanDeviceContext> &context,
                                                 const std::shared_ptr<IFrameImporter> &importer,
                                                 const CaptureCore::FrameMaker &frameMaker)
    : m_context{context}, m_importer{std::dynamic_pointer_cast<VulkanFrameImporter>(importer)},
      m_core{std::make_shared<CaptureCore>(videoFrameCaptureIpc, captureId, importer, frameMaker)},
      m_videoFrameCaptureIpc{videoFrameCaptureIpc}, m_captureId{captureId}
{
}

VulkanVideoFrameCapture::~VulkanVideoFrameCapture()
{
    if (m_videoFrameCaptureIpc && !m_videoFrameCaptureIpc->destroyVideoFrameCapture(m_captureId))
    {
        RIALTO_CLIENT_LOG_DEBUG("Could not remove video frame capture %u from the server", m_captureId);
    }
}

VideoFrameCaptureStatus VulkanVideoFrameCapture::getCapabilities(VulkanVideoFrameCaptureCaps &out,
                                                                 VideoFrameCaptureStatus &status) const
{
    CaptureCapabilitiesReply reply;
    status = m_videoFrameCaptureIpc
                 ? m_videoFrameCaptureIpc->getCapabilities(m_captureId, GraphicsAPIType::VULKAN, reply)
                 : VideoFrameCaptureStatus::FATAL_ERROR;
    if (status != VideoFrameCaptureStatus::OK)
    {
        return status;
    }

    VulkanVideoFrameCaptureCaps caps{};
    caps.size = sizeof(VulkanVideoFrameCaptureCaps);
    caps.common = commonCapabilitiesOf(reply);
    // the formats Rialto can import that this device supports with the reported modifier
    for (std::size_t i = 0; i < reply.drmFourccs.size(); ++i)
    {
        const std::uint64_t kModifier{i < reply.drmModifiers.size() ? reply.drmModifiers[i]
                                      : reply.drmModifiers.empty()  ? 0U
                                                                    : reply.drmModifiers.back()};
        const std::optional<VkFormat> kFormat{vulkanFormatForDrmFormat(reply.drmFourccs[i])};
        if (!kFormat || !m_context ||
            !vulkanFormatSupported(m_context->api(), m_context->physicalDevice(), *kFormat, kModifier))
        {
            RIALTO_CLIENT_LOG_INFO("Video frame capture %u: format 0x%08x with modifier 0x%llx is not supported",
                                   m_captureId, reply.drmFourccs[i], static_cast<unsigned long long>(kModifier));
            continue;
        }
        const auto kValue{static_cast<int32_t>(*kFormat)};
        if (std::find(caps.supportedFormats.begin(), caps.supportedFormats.end(), kValue) == caps.supportedFormats.end())
        {
            caps.supportedFormats.push_back(kValue);
        }
    }
    out = std::move(caps); // only on OK
    return status;
}

VulkanFrameAcquisition VulkanVideoFrameCapture::acquireCurrentFrame()
{
    std::lock_guard<std::mutex> lock{m_acquireMutex};
    CaptureFrameAcquisition acquisition{m_core->acquireFrame()};
    auto frame = std::dynamic_pointer_cast<VulkanVideoFrame>(acquisition.frame);
    if (m_importer)
    {
        // a changed frame is acquired from the producer; an unchanged one already is
        std::shared_ptr<VulkanImportedImage> acquired;
        if (acquisition.changed && frame && frame->import())
            acquired = std::dynamic_pointer_cast<VulkanImportedImage>(frame->import()->image());
        m_importer->submitOwnershipTransfers(acquired);
    }
    return VulkanFrameAcquisition{acquisition.changed, frame};
}
} // namespace firebolt::rialto::client
