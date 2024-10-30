/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Sky UK
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

#ifndef FIREBOLT_RIALTO_SERVER_CT_CONSTANTS_H_
#define FIREBOLT_RIALTO_SERVER_CT_CONSTANTS_H_

#include <cstdint>
#include <string>

#include "MediaCommon.h"
#include "RialtoLogging.h"

namespace firebolt::rialto::server::ct
{
constexpr unsigned kDefaultPermissions{0666};
const std::string kSocketName{"/tmp/rialto-0"};
const std::string kOwnerName{"root"};
constexpr int kWidth{1920};
constexpr int kHeight{1080};
constexpr int kX{10};
constexpr int kY{15};
constexpr int kNumOfChannels{2};
constexpr int kSampleRate{48000};
constexpr uint64_t kClippingStart{1024};
constexpr uint64_t kClippingEnd{2048};
constexpr firebolt::rialto::Fraction kFrameRate{15, 1};
constexpr int kAllLogs{RIALTO_DEBUG_LEVEL_FATAL | RIALTO_DEBUG_LEVEL_ERROR | RIALTO_DEBUG_LEVEL_WARNING |
                       RIALTO_DEBUG_LEVEL_MILESTONE | RIALTO_DEBUG_LEVEL_INFO | RIALTO_DEBUG_LEVEL_DEBUG};
constexpr double kPlaybackRate{0.5};
constexpr std::int64_t kCurrentPosition{1234};
constexpr double kVolume{0.5};
constexpr uint32_t kNoVolumeDuration{0};
constexpr uint32_t kVolumeDuration{10};
constexpr firebolt::rialto::EaseType kEaseType{firebolt::rialto::EaseType::EASE_LINEAR};
constexpr QosInfo kQosInfo{234, 567};
constexpr std::uint64_t kPosition{1234};
constexpr double kRate{1.0};
constexpr uint64_t kRenderedFrames{54321};
constexpr uint64_t kDroppedFrames{76};
constexpr uint64_t kStopPosition{234234};
} // namespace firebolt::rialto::server::ct

#endif // FIREBOLT_RIALTO_SERVER_CT_CONSTANTS_H_
