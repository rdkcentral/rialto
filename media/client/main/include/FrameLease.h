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

#ifndef FIREBOLT_RIALTO_CLIENT_FRAME_LEASE_H_
#define FIREBOLT_RIALTO_CLIENT_FRAME_LEASE_H_

#include "IFrameImporter.h"
#include "IVideoFrameCaptureIpc.h"

#include <cstdint>
#include <memory>
#include <mutex>

namespace firebolt::rialto::client
{
/**
 * @brief State a capture shares with its frames, so a frame can release its slot even after the capture is gone.
 */
class CaptureLeaseState
{
public:
    CaptureLeaseState(const std::shared_ptr<IVideoFrameCaptureIpc> &ipc, std::uint32_t captureId);

    /** Releases a frame lock. Calls are serialized per capture; may be called from any thread. */
    void release(std::uint32_t poolId, std::uint32_t slotIndex, std::uint64_t acquisitionToken);

private:
    std::shared_ptr<IVideoFrameCaptureIpc> m_ipc;
    const std::uint32_t m_captureId;
    std::mutex m_releaseMutex;
};

/**
 * @brief The lock on one frame's capture slot. Destroying it releases the slot exactly once.
 */
class FrameLease
{
public:
    FrameLease(const std::shared_ptr<CaptureLeaseState> &state, std::uint32_t poolId, std::uint32_t slotIndex,
               std::uint64_t acquisitionToken);
    ~FrameLease();

    FrameLease(const FrameLease &) = delete;
    FrameLease &operator=(const FrameLease &) = delete;

    /**
     * @brief Offers the release to \a importer when the lease goes (IFrameImporter::deferRelease); if it declines,
     *        the release is sent at once.
     */
    void deferTo(const std::shared_ptr<IFrameImporter> &importer, const std::shared_ptr<IImportedImage> &image);

private:
    std::shared_ptr<CaptureLeaseState> m_state;
    const std::uint32_t m_poolId;
    const std::uint32_t m_slotIndex;
    const std::uint64_t m_acquisitionToken;
    std::shared_ptr<IFrameImporter> m_importer;
    std::shared_ptr<IImportedImage> m_image;
};
} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_FRAME_LEASE_H_
