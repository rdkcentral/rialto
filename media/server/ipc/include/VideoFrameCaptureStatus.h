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

#ifndef FIREBOLT_RIALTO_SERVER_IPC_VIDEO_FRAME_CAPTURE_STATUS_H_
#define FIREBOLT_RIALTO_SERVER_IPC_VIDEO_FRAME_CAPTURE_STATUS_H_

#include "ICaptureSession.h"
#include "videoframecapturemodule.pb.h"

namespace firebolt::rialto::server::ipc
{
/**
 * @brief Maps a HAL result to the wire status (R9 design D6).
 *
 * After admission, CAPABILITIES_NOT_AVAILABLE_YET is a Rialto or vendor bug: it is logged and answered
 * FATAL_ERROR. INVALID_ARGUMENT is also logged and passed on. Every other status maps by name.
 */
::firebolt::rialto::VideoFrameCaptureStatus toWireStatus(capture::CaptureStatus status);
} // namespace firebolt::rialto::server::ipc

#endif // FIREBOLT_RIALTO_SERVER_IPC_VIDEO_FRAME_CAPTURE_STATUS_H_
