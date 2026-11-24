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

#ifndef FIREBOLT_RIALTO_CLIENT_CAPTURE_CAPABILITIES_H_
#define FIREBOLT_RIALTO_CLIENT_CAPTURE_CAPABILITIES_H_

#include "IVideoFrameCapture.h"
#include "IVideoFrameCaptureIpc.h"

namespace firebolt::rialto::client
{
/**
 * @brief The common capabilities from the server's reply, with the size and reserved fields set.
 */
inline VideoFrameCaptureCaps commonCapabilitiesOf(const CaptureCapabilitiesReply &reply)
{
    VideoFrameCaptureCaps common{};
    common.size = sizeof(VideoFrameCaptureCaps);
    common.maxConcurrentFrames = reply.maxConcurrentFrames;
    common.maxResourcesPerFrame = reply.maxResourcesPerFrame;
    common.supportsProtectedContent = reply.supportsProtectedContent;
    common.supportsHdr = reply.supportsHdr;
    common.reserved_0 = 0;
    return common;
}
} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_CAPTURE_CAPABILITIES_H_
