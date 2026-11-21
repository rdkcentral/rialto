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

#include <map>
#include <optional>

namespace firebolt::rialto::server::service
{
VideoFrameCaptureService::VideoFrameCaptureService(
    const std::shared_ptr<wrappers::ICaptureHalLibraryFactory> &captureHalLibraryFactory)
    : m_captureHalLibraryFactory{captureHalLibraryFactory}
{
}

bool VideoFrameCaptureService::isCaptureAvailableLocked()
{
    if (!m_captureHalLibrary && m_captureHalLibraryFactory)
    {
        // held for the server's lifetime once obtained, so the vendor library is loaded at most once
        m_captureHalLibrary = m_captureHalLibraryFactory->getCaptureHalLibrary();
    }
    return m_captureHalLibrary && m_captureHalLibrary->isAvailable();
}

VideoFrameCaptureServiceStatus VideoFrameCaptureService::createCapture(VideoFrameCaptureClientId clientId,
                                                                       std::optional<GraphicsAPIType> api,
                                                                       std::uint32_t &captureId)
{
    std::lock_guard<std::mutex> lock{m_mutex};
    if (!api)
    {
        RIALTO_SERVER_LOG_ERROR("Cannot register a video frame capture with an unknown graphics API");
        return VideoFrameCaptureServiceStatus::INVALID_ARGUMENT;
    }
    if (!isCaptureAvailableLocked())
    {
        RIALTO_SERVER_LOG_WARN("Video frame capture is unavailable: no capture HAL library");
        return VideoFrameCaptureServiceStatus::UNSUPPORTED;
    }
    captureId = m_registries[clientId].add(*api);
    RIALTO_SERVER_LOG_INFO("Registered video frame capture %u for client %llu", captureId,
                           static_cast<unsigned long long>(clientId));
    return VideoFrameCaptureServiceStatus::OK;
}

bool VideoFrameCaptureService::destroyCapture(VideoFrameCaptureClientId clientId, std::uint32_t captureId)
{
    std::optional<int> boundSession;
    BoundCaptureRemovedHandler handler;
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        auto it = m_registries.find(clientId);
        const VideoFrameCaptureEntry *kEntry{it == m_registries.end() ? nullptr : it->second.find(captureId)};
        if (!kEntry)
        {
            RIALTO_SERVER_LOG_WARN("Client %llu has no video frame capture %u",
                                   static_cast<unsigned long long>(clientId), captureId);
            return false;
        }
        boundSession = kEntry->attachedSessionId;
        it->second.remove(captureId);
        handler = m_boundCaptureRemovedHandler;
    }
    if (boundSession && handler)
    {
        handler(*boundSession, captureId); // destroying a bound capture detaches it
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
    std::map<std::uint32_t, int> bound;
    BoundCaptureRemovedHandler handler;
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        auto it = m_registries.find(clientId);
        if (it == m_registries.end())
            return;
        bound = it->second.boundCaptures();
        m_registries.erase(it);
        handler = m_boundCaptureRemovedHandler;
    }
    for (const auto &[kCaptureId, kSessionId] : bound)
    {
        if (handler)
            handler(kSessionId, kCaptureId);
    }
}

bool VideoFrameCaptureService::bindCapture(VideoFrameCaptureClientId clientId, std::uint32_t captureId, int sessionId)
{
    std::lock_guard<std::mutex> lock{m_mutex};
    auto it = m_registries.find(clientId);
    if (it == m_registries.end() || !it->second.bind(captureId, sessionId))
    {
        RIALTO_SERVER_LOG_WARN("Cannot bind video frame capture %u of client %llu to session %d", captureId,
                               static_cast<unsigned long long>(clientId), sessionId);
        return false;
    }
    return true;
}

void VideoFrameCaptureService::unbindCapture(VideoFrameCaptureClientId clientId, std::uint32_t captureId)
{
    std::lock_guard<std::mutex> lock{m_mutex};
    auto it = m_registries.find(clientId);
    if (it != m_registries.end())
    {
        it->second.unbind(captureId);
    }
}

void VideoFrameCaptureService::setBoundCaptureRemovedHandler(const BoundCaptureRemovedHandler &handler)
{
    std::lock_guard<std::mutex> lock{m_mutex};
    m_boundCaptureRemovedHandler = handler;
}

void VideoFrameCaptureService::setCaptureRuntime(VideoFrameCaptureClientId clientId, std::uint32_t captureId,
                                                 const std::shared_ptr<CaptureSessionRuntime> &runtime)
{
    std::lock_guard<std::mutex> lock{m_mutex};
    auto it = m_registries.find(clientId);
    if (it == m_registries.end() || !it->second.setRuntime(captureId, runtime))
    {
        RIALTO_SERVER_LOG_WARN("Cannot give video frame capture %u of client %llu its session runtime", captureId,
                               static_cast<unsigned long long>(clientId));
    }
}

std::shared_ptr<CaptureSessionRuntime> VideoFrameCaptureService::captureRuntime(VideoFrameCaptureClientId clientId,
                                                                                std::uint32_t captureId) const
{
    std::lock_guard<std::mutex> lock{m_mutex};
    auto it = m_registries.find(clientId);
    const VideoFrameCaptureEntry *kEntry{it == m_registries.end() ? nullptr : it->second.find(captureId)};
    return kEntry ? kEntry->runtime : nullptr;
}
} // namespace firebolt::rialto::server::service
