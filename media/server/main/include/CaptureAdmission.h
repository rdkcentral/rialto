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

#ifndef FIREBOLT_RIALTO_SERVER_CAPTURE_ADMISSION_H_
#define FIREBOLT_RIALTO_SERVER_CAPTURE_ADMISSION_H_

#include "ICaptureAdmission.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <thread>

namespace firebolt::rialto::server
{
/**
 * @brief Pool admission (proposal 3.3 steps 8 and 9, SoC capture spec 3.1, 3.2 and 4.4).
 *
 * On the server main thread, after the capture sink reached configured READY: queries the HAL capabilities,
 * retrying "not available yet" within the READY timeout; validates them; checks the pipeline's maximum video size
 * against maximumContentSize; chooses the slot count; creates the pool once and validates it; and gives the session
 * a pool ID. On failure it returns false and leaves the rollback and the session to the caller.
 */
class CaptureAdmission : public ICaptureAdmission
{
public:
    using Clock = std::function<std::chrono::steady_clock::time_point()>;
    using Sleep = std::function<void(std::chrono::milliseconds)>;

    /** The pause between "not available yet" answers. */
    static constexpr std::chrono::milliseconds kRetryInterval{10};

    /**
     * @param[in] clock : The time source for the retry budget; tests inject their own.
     * @param[in] sleep : Waits between retries; tests inject their own.
     */
    explicit CaptureAdmission(
        const Clock &clock = std::chrono::steady_clock::now,
        const Sleep &sleep = [](std::chrono::milliseconds duration) { std::this_thread::sleep_for(duration); });
    ~CaptureAdmission() override = default;

    bool admit(CaptureSessionHandle &handle) override;

private:
    Clock m_clock;
    Sleep m_sleep;
};

/**
 * @brief A new pool ID: non-zero and unique among the server's sessions (a process-wide counter that skips 0).
 */
std::uint32_t nextCapturePoolId();
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_CAPTURE_ADMISSION_H_
