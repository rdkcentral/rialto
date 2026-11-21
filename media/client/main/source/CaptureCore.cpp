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

#include "CaptureCore.h"
#include "RialtoClientLogging.h"
#include "VideoFrame.h"

#include <unistd.h>

namespace firebolt::rialto::client
{
CaptureCore::CaptureCore(const std::shared_ptr<IVideoFrameCaptureIpc> &ipc, std::uint32_t captureId,
                         const std::shared_ptr<IFrameImporter> &importer)
    : m_ipc{ipc}, m_captureId{captureId}, m_importer{importer},
      m_leaseState{std::make_shared<CaptureLeaseState>(ipc, captureId)}
{
}

CaptureCore::~CaptureCore() = default;

namespace
{
void closeFds(const std::vector<CaptureObjectFd> &fds)
{
    for (const CaptureObjectFd &kFd : fds)
    {
        if (kFd.fd >= 0)
            close(kFd.fd);
    }
}
} // namespace

GLFrameAcquisition CaptureCore::acquireCurrentFrame()
{
    std::lock_guard<std::mutex> lock{m_acquireMutex};
    // the application's context is current here: delete imports retired since the last acquisition, then, once
    // this acquisition is done, the ones it invalidated or evicted
    if (m_importer)
    {
        m_importer->drain();
    }
    GLFrameAcquisition acquisition{acquireLocked()};
    if (m_importer)
    {
        m_importer->drain();
    }
    return acquisition;
}

GLFrameAcquisition CaptureCore::acquireLocked()
{
    CaptureAcquireResult result = m_ipc->acquireCurrentFrame(m_captureId);

    switch (result.status)
    {
    case CaptureAcquireStatus::OK:
    {
        std::shared_ptr<IGLVideoFrame> frame = importNewFrame(result);
        if (!frame)
        {
            return GLFrameAcquisition{false, m_currentFrame};
        }
        m_currentFrame = frame;
        return GLFrameAcquisition{true, m_currentFrame};
    }
    case CaptureAcquireStatus::NO_NEW_FRAME:
    case CaptureAcquireStatus::NO_RESOURCES:
    case CaptureAcquireStatus::IPC_FAILURE:
        return GLFrameAcquisition{false, m_currentFrame};
    case CaptureAcquireStatus::NO_FRAME:
    case CaptureAcquireStatus::UNSUPPORTED:
    case CaptureAcquireStatus::INVALID_ARGUMENT:
    case CaptureAcquireStatus::FATAL_ERROR:
        break;
    }
    m_currentFrame.reset();
    return GLFrameAcquisition{false, nullptr};
}

std::size_t CaptureCore::cacheSize()
{
    std::lock_guard<std::mutex> lock{m_acquireMutex};
    return m_cache.size();
}

std::shared_ptr<IGLVideoFrame> CaptureCore::importNewFrame(CaptureAcquireResult &result)
{
    auto lease = std::make_unique<FrameLease>(m_leaseState, result.description.poolId, result.description.slotIndex,
                                              result.description.acquisitionToken);

    // the cache holds at most the admitted slot count of reusable imports
    if (result.pool)
    {
        m_cache.setBound(result.pool->slotCount);
    }
    else if (!m_poolRequested)
    {
        m_poolRequested = true;
        CapturePoolDescription pool;
        if (m_ipc->getPool(m_captureId, pool))
            m_cache.setBound(pool.slotCount);
    }

    // invalidation comes before the lookup and is never undone, even if the import below fails
    if (result.description.dmaBufObjectsFlushed)
    {
        ++m_period;
        m_cache.invalidate();
    }

    const ImportCacheKey kKey = ImportCacheKey::from(result.description, m_period);
    std::shared_ptr<ImportCacheEntry> entry = m_cache.find(kKey);
    if (entry)
    {
        closeFds(result.fds); // no import consumes them
    }
    else
    {
        bool fdsConsumed{false};
        std::shared_ptr<IImportedImage> image =
            m_importer ? m_importer->importFrame(result.description, result.fds, fdsConsumed) : nullptr;
        if (!fdsConsumed)
        {
            closeFds(result.fds);
        }
        if (!image)
        {
            RIALTO_CLIENT_LOG_ERROR("Failed to import video frame in slot %u", result.description.slotIndex);
            return nullptr; // the lease is dropped here, so the slot is released
        }
        entry = std::make_shared<ImportCacheEntry>(kKey, image, m_importer);
        m_cache.insert(entry);
    }
    return std::make_shared<VideoFrame>(result.description, entry, std::move(lease));
}
} // namespace firebolt::rialto::client
