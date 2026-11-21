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

#include <unistd.h>

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

namespace
{
void closeDescriptors(capture::CapturedFrame &frame)
{
    for (auto &exported : frame.exportedDmaBufs)
    {
        if (exported.fd >= 0)
        {
            close(exported.fd);
            exported.fd = -1;
        }
    }
}
} // namespace

CaptureFrameDescription CaptureFrameDescription::of(const capture::CapturedFrame &frame)
{
    CaptureFrameDescription description;
    description.drmFormat = frame.capabilities.drmFormat;
    description.drmModifier = frame.capabilities.drmModifier;
    description.backingWidth = frame.backingSize.width;
    description.backingHeight = frame.backingSize.height;
    description.colour = {static_cast<std::uint32_t>(frame.colorDescription.matrix),
                          static_cast<std::uint32_t>(frame.colorDescription.range),
                          static_cast<std::uint32_t>(frame.colorDescription.horizontalSiting),
                          static_cast<std::uint32_t>(frame.colorDescription.verticalSiting)};
    for (const auto &kPlane : frame.planes)
    {
        description.planes.emplace_back(kPlane.planeIndex, kPlane.offsetBytes, kPlane.lengthBytes, kPlane.strideBytes);
    }
    return description;
}

capture::CaptureStatus CaptureSessionRuntime::acquire(const FrameReplyFiller &fillReply)
{
    DeliveryTicket ticket;
    return acquire(fillReply, ticket);
}

capture::CaptureStatus CaptureSessionRuntime::acquire(const FrameReplyFiller &fillReply, DeliveryTicket &ticket)
{
    std::lock_guard<std::mutex> lock{m_mutex};
    if (m_retired)
    {
        return capture::CaptureStatus::NO_FRAME; // abandoned: no acquisition on a retired session
    }
    if (m_fatal)
    {
        return capture::CaptureStatus::FATAL_ERROR;
    }

    auto record = std::make_unique<capture::CapturedFrame>(); // a fresh, empty record for every call
    const capture::CaptureStatus kStatus{m_guard.acquireCurrentFrame(*record)};
    if (kStatus != capture::CaptureStatus::OK)
    {
        return kStatus;
    }

    // an invalidation is kept until a frame carrying it is delivered (SoC spec 7.3)
    if (record->dmaBufObjectsFlushed)
    {
        m_pendingFlush = true;
        ++m_flushGeneration;
    }

    const capture::ValidationResult kValidation{capture::validateFrame(*record, slotCount(), &m_lockedRanges)};
    m_warnings.report(kValidation);
    // validation checks the slot index is in the pool; a slot already locked is just as invalid
    const bool kSlotTaken{kValidation.ok && m_slots[record->slotIndex] != nullptr};
    if (!kValidation.ok || kSlotTaken)
    {
        RIALTO_SERVER_LOG_ERROR("Invalid captured frame (%s): video frame capture of pool %u is now unavailable",
                                kSlotTaken ? "slot already locked" : kValidation.rule.c_str(), m_poolId);
        m_guard.releaseFrame(*record); // the lock this acquisition created is ended exactly once
        closeDescriptors(*record);
        m_fatal = true;
        return capture::CaptureStatus::FATAL_ERROR;
    }

    CaptureFrameDescription description{CaptureFrameDescription::of(*record)};
    const bool kDescriptionChanged{!m_lastSentDescription || *m_lastSentDescription != description};
    fillReply(*record, m_pendingFlush, kDescriptionChanged);
    for (auto &exported : record->exportedDmaBufs)
    {
        exported.fd = -1; // the reply owns them now
    }
    ticket = DeliveryTicket{record->slotIndex, record->acquisitionToken, m_flushGeneration, std::move(description)};
    const std::uint32_t kSlot{record->slotIndex};
    m_slots[kSlot] = std::move(record);
    return capture::CaptureStatus::OK;
}

void CaptureSessionRuntime::onDelivery(const DeliveryTicket &ticket, bool delivered)
{
    std::lock_guard<std::mutex> lock{m_mutex};
    if (ticket.slotIndex >= m_slots.size() || !m_slots[ticket.slotIndex] ||
        m_slots[ticket.slotIndex]->acquisitionToken != ticket.acquisitionToken)
    {
        return; // already resolved
    }
    capture::CapturedFrame &record{*m_slots[ticket.slotIndex]};
    if (delivered)
    {
        // the client holds the frame now: its bytes are locked until it releases it
        m_lockedRanges.add(record);
        m_lastSentDescription = ticket.description; // the client knows this layout now
        if (ticket.flushGeneration == m_flushGeneration)
        {
            m_pendingFlush = false; // consumed only by a delivery that carried every invalidation seen
        }
        return;
    }
    // the client never got the frame: end its lock now (SoC spec 7.3); the invalidation stays pending
    RIALTO_SERVER_LOG_WARN("Video frame in slot %u of pool %u was not delivered; releasing it", ticket.slotIndex,
                           m_poolId);
    m_guard.releaseFrame(record);
    m_slots[ticket.slotIndex].reset();
}

capture::CaptureStatus CaptureSessionRuntime::release(std::uint32_t slotIndex, std::uint64_t acquisitionToken)
{
    std::lock_guard<std::mutex> lock{m_mutex};
    if (slotIndex >= m_slots.size() || !m_slots[slotIndex] || m_slots[slotIndex]->acquisitionToken != acquisitionToken)
    {
        RIALTO_SERVER_LOG_WARN("Release of slot %u with token %llu of pool %u matches no frame lock", slotIndex,
                               static_cast<unsigned long long>(acquisitionToken), m_poolId);
        return capture::CaptureStatus::INVALID_ARGUMENT;
    }
    const capture::CaptureStatus kStatus{m_guard.releaseFrame(*m_slots[slotIndex])};
    if (kStatus != capture::CaptureStatus::OK)
    {
        RIALTO_SERVER_LOG_ERROR("The capture HAL failed to release slot %u of pool %u", slotIndex, m_poolId);
        return kStatus; // the lock may still be held: keep the record
    }
    m_lockedRanges.remove(slotIndex);
    m_slots[slotIndex].reset();
    return capture::CaptureStatus::OK;
}

void CaptureSessionRuntime::retire()
{
    std::lock_guard<std::mutex> lock{m_mutex};
    if (!m_retired)
    {
        RIALTO_SERVER_LOG_INFO("Video frame capture session of pool %u retired", m_poolId);
        m_retired = true;
    }
}

bool CaptureSessionRuntime::isRetired() const
{
    std::lock_guard<std::mutex> lock{m_mutex};
    return m_retired;
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
