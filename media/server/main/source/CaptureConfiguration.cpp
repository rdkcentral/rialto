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

#include "CaptureConfiguration.h"
#include "RialtoServerLogging.h"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <limits>

namespace firebolt::rialto::server
{
namespace
{
constexpr std::chrono::milliseconds kDefaultCaptureReadyTimeout{5000};
constexpr std::uint32_t kDefaultTargetSlots{3};
constexpr std::uint32_t kMinimumAdmittedSlots{2};
} // namespace

std::chrono::milliseconds captureReadyTimeout()
{
    const char *kValue = std::getenv("RIALTO_VIDEO_FRAME_CAPTURE_READY_TIMEOUT_MS");
    if (kValue && *kValue)
    {
        char *end{nullptr};
        const long long kMs = std::strtoll(kValue, &end, 10);
        if (end && *end == '\0' && kMs > 0)
        {
            return std::chrono::milliseconds{kMs};
        }
        RIALTO_SERVER_LOG_WARN("Ignoring RIALTO_VIDEO_FRAME_CAPTURE_READY_TIMEOUT_MS='%s'", kValue);
    }
    return kDefaultCaptureReadyTimeout;
}

std::uint32_t captureTargetSlots()
{
    const char *kValue = std::getenv("RIALTO_VIDEO_FRAME_CAPTURE_TARGET_SLOTS");
    if (!kValue || !*kValue)
    {
        return kDefaultTargetSlots;
    }
    char *end{nullptr};
    errno = 0;
    const unsigned long long kSlots = std::strtoull(kValue, &end, 10);
    if (kValue[0] != '-' && errno == 0 && end && *end == '\0' && kSlots >= kMinimumAdmittedSlots)
    {
        // no upper cap: the clamp to maximumSlots bounds it
        return static_cast<std::uint32_t>(std::min<unsigned long long>(kSlots, std::numeric_limits<std::uint32_t>::max()));
    }
    RIALTO_SERVER_LOG_WARN("Ignoring RIALTO_VIDEO_FRAME_CAPTURE_TARGET_SLOTS='%s'; using %u", kValue,
                           kDefaultTargetSlots);
    return kDefaultTargetSlots;
}

std::uint32_t chooseCaptureSlotCount(std::uint32_t minimumSlots, std::uint32_t maximumSlots, std::uint32_t target)
{
    if (maximumSlots < kMinimumAdmittedSlots)
    {
        return 0;
    }
    const std::uint32_t kLowest{std::max(kMinimumAdmittedSlots, minimumSlots)};
    return std::clamp(target, std::min(kLowest, maximumSlots), maximumSlots);
}
} // namespace firebolt::rialto::server
