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

#include "VideoFrameCaptureRegistry.h"

namespace firebolt::rialto::server::service
{
std::uint32_t VideoFrameCaptureRegistry::add(GraphicsAPIType api)
{
    const std::uint32_t kId = m_nextId++;
    m_entries.emplace(kId, VideoFrameCaptureEntry{api});
    return kId;
}

bool VideoFrameCaptureRegistry::remove(std::uint32_t captureId)
{
    return m_entries.erase(captureId) != 0;
}

bool VideoFrameCaptureRegistry::bind(std::uint32_t captureId, int sessionId)
{
    auto it = m_entries.find(captureId);
    if (it == m_entries.end() || it->second.attachedSessionId)
        return false;
    it->second.attachedSessionId = sessionId;
    return true;
}

bool VideoFrameCaptureRegistry::setRuntime(std::uint32_t captureId, const std::shared_ptr<CaptureSessionRuntime> &runtime)
{
    auto it = m_entries.find(captureId);
    if (it == m_entries.end())
        return false;
    it->second.runtime = runtime;
    return true;
}

bool VideoFrameCaptureRegistry::unbind(std::uint32_t captureId)
{
    auto it = m_entries.find(captureId);
    if (it == m_entries.end() || !it->second.attachedSessionId)
        return false;
    it->second.attachedSessionId.reset();
    return true;
}

std::map<std::uint32_t, int> VideoFrameCaptureRegistry::boundCaptures() const
{
    std::map<std::uint32_t, int> bound;
    for (const auto &[kId, kEntry] : m_entries)
    {
        if (kEntry.attachedSessionId)
            bound.emplace(kId, *kEntry.attachedSessionId);
    }
    return bound;
}

const VideoFrameCaptureEntry *VideoFrameCaptureRegistry::find(std::uint32_t captureId) const
{
    auto it = m_entries.find(captureId);
    return it == m_entries.end() ? nullptr : &it->second;
}
} // namespace firebolt::rialto::server::service
