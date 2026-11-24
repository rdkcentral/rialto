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

#include "GLVideoFrameCapture.h"
#include "CaptureCapabilities.h"
#include "RialtoClientLogging.h"

namespace firebolt::rialto::client
{
GLVideoFrameCapture::GLVideoFrameCapture(const std::shared_ptr<IVideoFrameCaptureIpc> &videoFrameCaptureIpc,
                                         std::uint32_t captureId, EGLDisplay eglDisplay, EGLContext eglContext,
                                         const std::shared_ptr<IFrameImporter> &importer)
    : m_core{std::make_shared<CaptureCore>(videoFrameCaptureIpc, captureId, importer)},
      m_videoFrameCaptureIpc{videoFrameCaptureIpc}, m_captureId{captureId}, m_eglDisplay{eglDisplay},
      m_eglContext{eglContext}
{
}

GLVideoFrameCapture::~GLVideoFrameCapture()
{
    // the server may already be gone; its disconnect cleanup then removed the registration
    if (m_videoFrameCaptureIpc && !m_videoFrameCaptureIpc->destroyVideoFrameCapture(m_captureId))
    {
        RIALTO_CLIENT_LOG_DEBUG("Could not remove video frame capture %u from the server", m_captureId);
    }
}

VideoFrameCaptureStatus GLVideoFrameCapture::getCapabilities(GLVideoFrameCaptureCaps &out,
                                                             VideoFrameCaptureStatus &status) const
{
    CaptureCapabilitiesReply reply;
    status = m_videoFrameCaptureIpc ? m_videoFrameCaptureIpc->getCapabilities(m_captureId, GraphicsAPIType::GL, reply)
                                    : VideoFrameCaptureStatus::FATAL_ERROR;
    if (status == VideoFrameCaptureStatus::OK)
    {
        GLVideoFrameCaptureCaps caps{};
        caps.size = sizeof(GLVideoFrameCaptureCaps);
        caps.common = commonCapabilitiesOf(reply);
        out = caps; // only on OK
    }
    return status;
}

GLFrameAcquisition GLVideoFrameCapture::acquireCurrentFrame()
{
    return m_core->acquireCurrentFrame();
}
} // namespace firebolt::rialto::client
