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
#include "RialtoClientLogging.h"
#include "VulkanVideoFrame.h"

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
