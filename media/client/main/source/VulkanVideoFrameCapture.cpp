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

namespace firebolt::rialto::client
{
VulkanVideoFrameCapture::VulkanVideoFrameCapture(const std::shared_ptr<IVideoFrameCaptureIpc> &videoFrameCaptureIpc,
                                                 std::uint32_t captureId,
                                                 const std::shared_ptr<VulkanDeviceContext> &context,
                                                 const std::shared_ptr<IFrameImporter> &importer,
                                                 const CaptureCore::FrameMaker &frameMaker)
    : m_context{context}, m_core{std::make_shared<CaptureCore>(videoFrameCaptureIpc, captureId, importer, frameMaker)},
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
    CaptureFrameAcquisition acquisition{m_core->acquireFrame()};
    return VulkanFrameAcquisition{acquisition.changed, std::dynamic_pointer_cast<IVulkanVideoFrame>(acquisition.frame)};
}
} // namespace firebolt::rialto::client
