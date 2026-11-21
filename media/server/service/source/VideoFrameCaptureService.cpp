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

#include "VideoFrameCaptureService.h"
#include "RialtoServerLogging.h"

namespace firebolt::rialto::server::service
{
VideoFrameCaptureServiceStatus VideoFrameCaptureService::createCapture(VideoFrameCaptureClientId clientId,
                                                                       std::optional<GraphicsAPIType> api,
                                                                       std::uint32_t &captureId)
{
    std::lock_guard<std::mutex> lock{m_mutex};
    captureId = m_registries[clientId].add(api.value_or(GraphicsAPIType::GL));
    RIALTO_SERVER_LOG_INFO("Registered video frame capture %u for client %llu", captureId,
                           static_cast<unsigned long long>(clientId));
    return VideoFrameCaptureServiceStatus::OK;
}

bool VideoFrameCaptureService::destroyCapture(VideoFrameCaptureClientId clientId, std::uint32_t captureId)
{
    std::lock_guard<std::mutex> lock{m_mutex};
    auto it = m_registries.find(clientId);
    if (it == m_registries.end() || !it->second.remove(captureId))
    {
        RIALTO_SERVER_LOG_WARN("Client %llu has no video frame capture %u", static_cast<unsigned long long>(clientId),
                               captureId);
        return false;
    }
    return true;
}

bool VideoFrameCaptureService::hasCapture(VideoFrameCaptureClientId clientId, std::uint32_t captureId) const
{
    std::lock_guard<std::mutex> lock{m_mutex};
    auto it = m_registries.find(clientId);
    return it != m_registries.end() && it->second.find(captureId) != nullptr;
}

void VideoFrameCaptureService::removeClient(VideoFrameCaptureClientId clientId)
{
    std::lock_guard<std::mutex> lock{m_mutex};
    m_registries.erase(clientId);
}
} // namespace firebolt::rialto::server::service
