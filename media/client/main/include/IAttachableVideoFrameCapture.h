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

#ifndef FIREBOLT_RIALTO_CLIENT_I_ATTACHABLE_VIDEO_FRAME_CAPTURE_H_
#define FIREBOLT_RIALTO_CLIENT_I_ATTACHABLE_VIDEO_FRAME_CAPTURE_H_

#include <cstdint>

namespace firebolt::rialto::client
{
/**
 * @brief Implemented by every capture Rialto's factory creates, so a pipeline can tell them from foreign
 *        IVideoFrameCapture implementations and keep a capture on one pipeline at a time (design D2).
 */
class IAttachableVideoFrameCapture
{
public:
    virtual ~IAttachableVideoFrameCapture() = default;

    /** The server's ID for this capture. */
    virtual std::uint32_t captureId() const = 0;

    /** Marks the capture attached. @retval false if it already was attached to a pipeline. */
    virtual bool markAttached() = 0;

    /** Clears the attached mark. */
    virtual void clearAttached() = 0;
};
} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_I_ATTACHABLE_VIDEO_FRAME_CAPTURE_H_
