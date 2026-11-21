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

#include "FrameLease.h"
#include "RialtoClientLogging.h"

namespace firebolt::rialto::client
{
CaptureLeaseState::CaptureLeaseState(const std::shared_ptr<IVideoFrameCaptureIpc> &ipc, std::uint32_t captureId)
    : m_ipc{ipc}, m_captureId{captureId}
{
}

void CaptureLeaseState::release(std::uint32_t poolId, std::uint32_t slotIndex, std::uint64_t acquisitionToken)
{
    std::lock_guard<std::mutex> lock{m_releaseMutex};
    if (!m_ipc || !m_ipc->releaseFrame(m_captureId, poolId, slotIndex, acquisitionToken))
    {
        // the server releases remaining locks itself when the connection goes
        RIALTO_CLIENT_LOG_WARN("Could not release video frame slot %u of capture %u", slotIndex, m_captureId);
    }
}

FrameLease::FrameLease(const std::shared_ptr<CaptureLeaseState> &state, std::uint32_t poolId, std::uint32_t slotIndex,
                       std::uint64_t acquisitionToken)
    : m_state{state}, m_poolId{poolId}, m_slotIndex{slotIndex}, m_acquisitionToken{acquisitionToken}
{
}

FrameLease::~FrameLease()
{
    if (!m_state)
    {
        return;
    }
    std::function<void()> sendRelease{[state = m_state, poolId = m_poolId, slotIndex = m_slotIndex,
                                       token = m_acquisitionToken]() { state->release(poolId, slotIndex, token); }};
    if (m_importer && m_importer->deferRelease(m_image, sendRelease))
    {
        return; // the importer sends it once the image is back with the producer
    }
    sendRelease();
}

void FrameLease::deferTo(const std::shared_ptr<IFrameImporter> &importer, const std::shared_ptr<IImportedImage> &image)
{
    m_importer = importer;
    m_image = image;
}
} // namespace firebolt::rialto::client
