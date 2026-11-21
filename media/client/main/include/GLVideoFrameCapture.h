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

#include "IVideoFrameCapture.h"
#include "IVideoFrameCaptureIpc.h"

#include <cstdint>
#include <memory>

namespace firebolt::rialto::client
{
/**
 * @brief A GL capture registered with the server. Destroying it removes the registration.
 *
 * Frame delivery is not implemented yet: acquireCurrentFrame() returns no frame.
 */
class GLVideoFrameCapture : public IGLVideoFrameCapture
{
public:
    GLVideoFrameCapture(const std::shared_ptr<IVideoFrameCaptureIpc> &videoFrameCaptureIpc, std::uint32_t captureId,
                        EGLDisplay eglDisplay, EGLContext eglContext);
    ~GLVideoFrameCapture() override;

    GraphicsAPIType apiType() const override { return GraphicsAPIType::GL; }
    GLFrameAcquisition acquireCurrentFrame() override;

    /** The server's ID for this capture. */
    std::uint32_t captureId() const { return m_captureId; }

private:
    std::shared_ptr<IVideoFrameCaptureIpc> m_videoFrameCaptureIpc;
    const std::uint32_t m_captureId;
    EGLDisplay m_eglDisplay;
    EGLContext m_eglContext;
};
} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_GL_VIDEO_FRAME_CAPTURE_H_
