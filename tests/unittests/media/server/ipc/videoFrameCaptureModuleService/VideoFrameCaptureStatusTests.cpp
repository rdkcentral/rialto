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

#include <gtest/gtest.h>

#include <utility>
#include <vector>

using firebolt::rialto::capture::CaptureStatus;
using firebolt::rialto::server::ipc::toWireStatus;

TEST(VideoFrameCaptureStatusTest, ShouldMapEveryHalStatus)
{
    const std::vector<std::pair<CaptureStatus, firebolt::rialto::VideoFrameCaptureStatus>> kTable{
        {CaptureStatus::OK, firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_OK},
        {CaptureStatus::NO_NEW_FRAME, firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_NO_NEW_FRAME},
        {CaptureStatus::UNSUPPORTED, firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_UNSUPPORTED},
        {CaptureStatus::INVALID_ARGUMENT, firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_INVALID_ARGUMENT},
        {CaptureStatus::NO_RESOURCES, firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_NO_RESOURCES},
        {CaptureStatus::NO_FRAME, firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_NO_FRAME},
        {CaptureStatus::FATAL_ERROR, firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_FATAL_ERROR},
        // a bug after admission: fatal
        {CaptureStatus::CAPABILITIES_NOT_AVAILABLE_YET, firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_FATAL_ERROR},
        // an unknown value: fatal
        {static_cast<CaptureStatus>(99), firebolt::rialto::VIDEO_FRAME_CAPTURE_STATUS_FATAL_ERROR},
    };
    for (const auto &[kHal, kWire] : kTable)
        EXPECT_EQ(toWireStatus(kHal), kWire) << "HAL status " << static_cast<unsigned>(kHal);
}
