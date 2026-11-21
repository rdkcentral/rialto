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

#include "VideoFrameCaptureStatus.h"
#include "RialtoServerLogging.h"

namespace firebolt::rialto::server::ipc
{
::firebolt::rialto::VideoFrameCaptureStatus toWireStatus(capture::CaptureStatus status)
{
    switch (status)
    {
    case capture::CaptureStatus::OK:
        return ::firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_OK;
    case capture::CaptureStatus::NO_NEW_FRAME:
        return ::firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_NO_NEW_FRAME;
    case capture::CaptureStatus::UNSUPPORTED:
        return ::firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_UNSUPPORTED;
    case capture::CaptureStatus::INVALID_ARGUMENT:
        RIALTO_SERVER_LOG_ERROR("The capture HAL reported INVALID_ARGUMENT after admission: a Rialto or vendor bug");
        return ::firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_INVALID_ARGUMENT;
    case capture::CaptureStatus::NO_RESOURCES:
        return ::firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_NO_RESOURCES;
    case capture::CaptureStatus::NO_FRAME:
        return ::firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_NO_FRAME;
    case capture::CaptureStatus::FATAL_ERROR:
        return ::firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_FATAL_ERROR;
    case capture::CaptureStatus::CAPABILITIES_NOT_AVAILABLE_YET:
        RIALTO_SERVER_LOG_ERROR("The capture HAL reported CAPABILITIES_NOT_AVAILABLE_YET after admission: a Rialto or "
                                "vendor bug, treated as fatal");
        return ::firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_FATAL_ERROR;
    }
    RIALTO_SERVER_LOG_ERROR("Unknown capture HAL status %u", static_cast<unsigned>(status));
    return ::firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_FATAL_ERROR;
}
} // namespace firebolt::rialto::server::ipc
