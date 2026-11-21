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

#include "VideoFrameCaptureFactory.h"
#include "RialtoClientLogging.h"

#include <exception>

namespace firebolt::rialto
{
std::shared_ptr<IVideoFrameCaptureFactory> IVideoFrameCaptureFactory::createFactory()
{
    std::shared_ptr<IVideoFrameCaptureFactory> factory;

    try
    {
        factory = std::make_shared<client::VideoFrameCaptureFactory>();
    }
    catch (const std::exception &e)
    {
        RIALTO_CLIENT_LOG_ERROR("Failed to create the video frame capture factory, reason: %s", e.what());
    }

    return factory;
}
} // namespace firebolt::rialto

namespace firebolt::rialto::client
{
std::shared_ptr<IGLVideoFrameCapture> VideoFrameCaptureFactory::createGLVideoFrameCapture(EGLDisplay eglDisplay,
                                                                                          EGLContext eglContext)
{
    (void)eglDisplay;
    (void)eglContext;
    RIALTO_CLIENT_LOG_WARN("Video frame capture is not supported");
    return nullptr;
}
} // namespace firebolt::rialto::client
