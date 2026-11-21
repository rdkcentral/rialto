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
    auto it = m_entries.find(captureId);
    if (it == m_entries.end())
        return false;
    auto &orphans = m_orphans[captureId];
    if (it->second.runtime)
        orphans.push_back(it->second.runtime);
    orphans.insert(orphans.end(), it->second.retiredSessions.begin(), it->second.retiredSessions.end());
    if (orphans.empty())
        m_orphans.erase(captureId);
    m_entries.erase(it);
    return true;
}

std::shared_ptr<CaptureSessionRuntime> VideoFrameCaptureRegistry::sessionForRelease(std::uint32_t captureId,
                                                                                    std::uint32_t poolId) const
{
    auto matches = [poolId](const std::shared_ptr<CaptureSessionRuntime> &runtime)
    { return runtime && runtime->poolId() == poolId; };
    auto entryIt = m_entries.find(captureId);
    if (entryIt != m_entries.end())
    {
        if (matches(entryIt->second.runtime))
            return entryIt->second.runtime;
        for (const auto &kRetired : entryIt->second.retiredSessions)
        {
            if (matches(kRetired))
                return kRetired;
        }
    }
    auto orphanIt = m_orphans.find(captureId);
    if (orphanIt != m_orphans.end())
    {
        for (const auto &kOrphan : orphanIt->second)
        {
            if (matches(kOrphan))
                return kOrphan;
        }
    }
    return nullptr;
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
    if (it->second.runtime)
    {
        // a new commit: the previous session stays reachable for its releases
        it->second.runtime->retire();
        it->second.retiredSessions.push_back(it->second.runtime);
    }
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
