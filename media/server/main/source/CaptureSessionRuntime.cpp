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

#include "CaptureSessionRuntime.h"
#include "RialtoServerLogging.h"

#include <utility>

namespace firebolt::rialto::server
{
CaptureSessionRuntime::CaptureSessionRuntime(wrappers::CaptureSessionPtr &&session, std::uint32_t poolId,
                                             const capture::CapturePool &pool)
    : m_session{std::move(session)}, m_guard{*m_session}, m_poolId{poolId}, m_pool{pool},
      m_slots(pool.slotLayouts.size())
{
}

CaptureSessionRuntime::~CaptureSessionRuntime()
{
    // locks still held here are released by session teardown (R10); the HAL session is deleted with m_session
    for (const auto &kRecord : m_slots)
    {
        if (kRecord)
        {
            RIALTO_SERVER_LOG_WARN("Capture slot %u of pool %u still locked at destruction", kRecord->slotIndex,
                                   m_poolId);
        }
    }
}

bool CaptureSessionRuntime::isSlotLocked(std::uint32_t slotIndex) const
{
    std::lock_guard<std::mutex> lock{m_mutex};
    return slotIndex < m_slots.size() && m_slots[slotIndex] != nullptr;
}

bool CaptureSessionRuntime::isFatal() const
{
    std::lock_guard<std::mutex> lock{m_mutex};
    return m_fatal;
}

bool CaptureSessionRuntime::isFlushPending() const
{
    std::lock_guard<std::mutex> lock{m_mutex};
    return m_pendingFlush;
}
} // namespace firebolt::rialto::server
