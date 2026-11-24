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

#ifndef FIREBOLT_RIALTO_CLIENT_GL_VIDEO_FRAME_CAPTURE_H_
#define FIREBOLT_RIALTO_CLIENT_GL_VIDEO_FRAME_CAPTURE_H_

#include "CaptureCore.h"
#include "IAttachableVideoFrameCapture.h"
#include "IFrameImporter.h"
#include "IVideoFrameCapture.h"
#include "IVideoFrameCaptureIpc.h"

#include <atomic>
#include <cstdint>
#include <memory>

namespace firebolt::rialto::client
{
/**
 * @brief A GL capture registered with the server. Destroying it removes the registration.
 *
 * Acquisition, the frame lease and the import cache are in CaptureCore; the importer does the GL import.
 */
class GLVideoFrameCapture : public IGLVideoFrameCapture, public IAttachableVideoFrameCapture
{
public:
    GLVideoFrameCapture(const std::shared_ptr<IVideoFrameCaptureIpc> &videoFrameCaptureIpc, std::uint32_t captureId,
                        EGLDisplay eglDisplay, EGLContext eglContext,
                        const std::shared_ptr<IFrameImporter> &importer = nullptr);
    ~GLVideoFrameCapture() override;

    GraphicsAPIType apiType() const override { return GraphicsAPIType::GL; }
    VideoFrameCaptureStatus getCapabilities(GLVideoFrameCaptureCaps &out, VideoFrameCaptureStatus &status) const override;
    GLFrameAcquisition acquireCurrentFrame() override;

    std::uint32_t captureId() const override { return m_captureId; }
    bool markAttached() override { return !m_attached.exchange(true); }
    void clearAttached() override { m_attached = false; }

private:
    std::shared_ptr<CaptureCore> m_core;
    std::shared_ptr<IVideoFrameCaptureIpc> m_videoFrameCaptureIpc;
    const std::uint32_t m_captureId;
    EGLDisplay m_eglDisplay;
    EGLContext m_eglContext;
    std::atomic<bool> m_attached{false}; /**< Attached to a pipeline; the pipeline holds the capture meanwhile. */
};
} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_GL_VIDEO_FRAME_CAPTURE_H_
