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

#ifndef FIREBOLT_RIALTO_CLIENT_VIDEO_FRAME_CAPTURE_FACTORY_H_
#define FIREBOLT_RIALTO_CLIENT_VIDEO_FRAME_CAPTURE_FACTORY_H_

#include "IVideoFrameCapture.h"

#include <memory>

namespace firebolt::rialto::client
{
/**
 * @brief Client implementation of IVideoFrameCaptureFactory.
 *
 * Captures are not available yet: registration with the server arrives with the capture registry, so
 * createGLVideoFrameCapture() returns null.
 */
class VideoFrameCaptureFactory : public IVideoFrameCaptureFactory
{
public:
    VideoFrameCaptureFactory() = default;
    ~VideoFrameCaptureFactory() override = default;

    std::shared_ptr<IGLVideoFrameCapture> createGLVideoFrameCapture(EGLDisplay eglDisplay, EGLContext eglContext) override;
};

} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_VIDEO_FRAME_CAPTURE_FACTORY_H_
