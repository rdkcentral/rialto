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

#ifndef FIREBOLT_RIALTO_SERVER_CAPTURE_CONFIGURATION_H_
#define FIREBOLT_RIALTO_SERVER_CAPTURE_CONFIGURATION_H_

#include <chrono>
#include <cstdint>

namespace firebolt::rialto::server
{
/**
 * @brief The capture READY timeout: RIALTO_VIDEO_FRAME_CAPTURE_READY_TIMEOUT_MS if it is a positive number of
 *        milliseconds, else 5000. Read at each call so it can be changed for tests.
 *
 * It bounds the capture sink's READY wait and, separately, pool admission's capabilities retries.
 */
std::chrono::milliseconds captureReadyTimeout();

/**
 * @brief The target slot count T: RIALTO_VIDEO_FRAME_CAPTURE_TARGET_SLOTS if it is a whole number of at least 2,
 *        else 3. A set but unparsable value, or one below 2, logs a warning. Read at each call, so tests can change it.
 */
std::uint32_t captureTargetSlots();

/**
 * @brief Chooses the admitted slot count N = clamp(T, max(2, minimumSlots), maximumSlots).
 *
 * @retval The slot count, or 0 if maximumSlots is below 2 (the session cannot be admitted).
 */
std::uint32_t chooseCaptureSlotCount(std::uint32_t minimumSlots, std::uint32_t maximumSlots, std::uint32_t target);
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_CAPTURE_CONFIGURATION_H_
