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
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <tuple>
#include <vector>

namespace firebolt::rialto::server
{
/**
 * @brief What a frame looks like to the importer: format, modifier, backing size, colour and plane layout. Object
 *        IDs are not part of it, since frames in different slots differ only by them.
 */
struct CaptureFrameDescription
{
    std::uint32_t drmFormat{0};
    std::uint64_t drmModifier{0};
    std::uint32_t backingWidth{0};
    std::uint32_t backingHeight{0};
    std::tuple<std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t> colour{};
    std::vector<std::tuple<std::uint32_t, std::uint64_t, std::uint64_t, std::uint32_t>> planes; /**< index, offset,
                                                                                                    length, stride */

    static CaptureFrameDescription of(const capture::CapturedFrame &frame);
    bool operator==(const CaptureFrameDescription &other) const
    {
        return drmFormat == other.drmFormat && drmModifier == other.drmModifier && backingWidth == other.backingWidth &&
               backingHeight == other.backingHeight && colour == other.colour && planes == other.planes;
    }
    bool operator!=(const CaptureFrameDescription &other) const { return !(*this == other); }
};

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

    /**
     * @brief Called under the session lock with an acquired, validated frame, to fill the reply.
     *
     * @param[in] frame   : The frame. Its descriptors pass to the reply, whose sender closes them (owned FDs); the
     *                      runtime forgets them after this call.
     * @param[in] flushed : The invalidation flag to deliver: set if this or an undelivered earlier frame flushed.
     * @param[in] descriptionChanged : The frame description differs from the last one delivered (or none was), so
     *                                 the reply carries the pool metadata.
     */
    using FrameReplyFiller =
        std::function<void(const capture::CapturedFrame &frame, bool flushed, bool descriptionChanged)>;

    /**
     * @brief Acquires the current frame (design D2). Runs on the IPC thread and never waits on media.
     *
     * Under the session lock: a capture-fatal session answers FATAL_ERROR; otherwise the HAL is asked for a frame.
     * A non-OK result is returned as is. An OK frame is validated; an invalid one is released and its descriptors
     * closed, and the session becomes capture-fatal. A valid one is passed to fillReply and kept in its slot.
     *
     * @retval the HAL status, or FATAL_ERROR for a fatal session or an invalid frame.
     */
    capture::CaptureStatus acquire(const FrameReplyFiller &fillReply);

    /**
     * @brief Identifies an acquired frame whose reply is being sent.
     */
    struct DeliveryTicket
    {
        std::uint32_t slotIndex{0};
        std::uint64_t acquisitionToken{0};
        std::uint64_t flushGeneration{0};    /**< The invalidations the reply covered. */
        CaptureFrameDescription description; /**< Becomes the last delivered description on delivery. */
    };

    /**
     * @brief As acquire(), also giving the ticket to pass to onDelivery() for an OK frame.
     */
    capture::CaptureStatus acquire(const FrameReplyFiller &fillReply, DeliveryTicket &ticket);

    /**
     * @brief Resolves an acquired frame once its reply was sent (design D3). Called by the IPC delivery callback,
     *        with no IPC lock held, on any thread.
     *
     * Delivered: the frame's ranges become locked and the invalidation latch is cleared, unless a newer
     * invalidation arrived since the reply was filled. Not delivered: the frame lock is released at once and the
     * latch stays set, so the next delivered frame carries the invalidation.
     */
    void onDelivery(const DeliveryTicket &ticket, bool delivered);

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
    std::uint64_t m_flushGeneration{0}; /**< Counts invalidations seen, so a delivery clears only what it carried. */
    std::optional<CaptureFrameDescription> m_lastSentDescription;
    bool m_fatal{false};
    capture::LockedRanges m_lockedRanges;
    capture::ValidationWarningLog m_warnings;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_CAPTURE_SESSION_RUNTIME_H_
