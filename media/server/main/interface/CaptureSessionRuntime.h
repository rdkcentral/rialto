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

#ifndef FIREBOLT_RIALTO_SERVER_CAPTURE_SESSION_RUNTIME_H_
#define FIREBOLT_RIALTO_SERVER_CAPTURE_SESSION_RUNTIME_H_

#include "CaptureRecordValidator.h"
#include "CaptureSessionGuard.h"
#include "ICaptureHalLibrary.h"
#include "ICaptureSession.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

namespace firebolt::rialto::server
{
/**
 * @brief Everything that happens to a committed capture session's frames (R9 design D1): the HAL session and the
 *        lock serializing its calls, one record per pool slot, the pending invalidation, the last description sent,
 *        the capture-fatal flag and the locked byte ranges.
 *
 * Created by the pipeline at commit, from the admitted session and pool, and given to the capture registry entry,
 * where the acquire and release calls find it.
 */
class CaptureSessionRuntime
{
public:
    /**
     * @param[in] session : The admitted HAL session.
     * @param[in] poolId  : The admitted pool's ID.
     * @param[in] pool    : The admitted pool; one record slot is kept per slot layout.
     */
    CaptureSessionRuntime(wrappers::CaptureSessionPtr &&session, std::uint32_t poolId, const capture::CapturePool &pool);
    ~CaptureSessionRuntime();

    CaptureSessionRuntime(const CaptureSessionRuntime &) = delete;
    CaptureSessionRuntime &operator=(const CaptureSessionRuntime &) = delete;

    std::uint32_t poolId() const { return m_poolId; }
    std::uint32_t slotCount() const { return static_cast<std::uint32_t>(m_slots.size()); }
    const capture::CapturePool &pool() const { return m_pool; }

    /** Whether the slot holds a frame lock owed a release. */
    bool isSlotLocked(std::uint32_t slotIndex) const;

    /** Whether the session became capture-fatal. */
    bool isFatal() const;

    /** Whether an invalidation is waiting to be delivered. */
    bool isFlushPending() const;

private:
    mutable std::mutex m_mutex; /**< Serializes every HAL call and the state below. */
    wrappers::CaptureSessionPtr m_session;
    capture::CaptureSessionGuard m_guard;
    const std::uint32_t m_poolId;
    const capture::CapturePool m_pool;
    std::vector<std::unique_ptr<capture::CapturedFrame>> m_slots; /**< Records are not movable: one per slot. */
    bool m_pendingFlush{false};
    bool m_fatal{false};
    capture::LockedRanges m_lockedRanges;
    capture::ValidationWarningLog m_warnings;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_CAPTURE_SESSION_RUNTIME_H_
