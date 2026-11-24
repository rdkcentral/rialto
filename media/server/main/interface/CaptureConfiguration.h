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

namespace firebolt::rialto::server
{
/**
 * @brief The capture READY timeout: RIALTO_VIDEO_FRAME_CAPTURE_READY_TIMEOUT_MS if it is a positive number of
 *        milliseconds, else 5000. Read at each call so it can be changed for tests.
 *
 * It bounds the capture sink's READY wait and, separately, pool admission's capabilities retries.
 */
std::chrono::milliseconds captureReadyTimeout();
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_CAPTURE_CONFIGURATION_H_
