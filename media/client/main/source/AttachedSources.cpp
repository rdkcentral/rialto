/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Sky UK
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

#include "AttachedSources.h"

namespace firebolt::rialto::client
{
void AttachedSources::add(std::uint32_t id, const MediaSourceType &mediaSourceType)
{
    std::unique_lock<std::mutex> lock{m_mutex};
    m_attachedSources.insert(std::make_pair(id, mediaSourceType));
}

void AttachedSources::remove(std::uint32_t id)
{
    std::unique_lock<std::mutex> lock{m_mutex};
    m_attachedSources.erase(id);
}

MediaSourceType AttachedSources::get(std::uint32_t id) const
{
    std::unique_lock<std::mutex> lock{m_mutex};
    auto iter = m_attachedSources.find(id);
    if (m_attachedSources.end() == iter)
    {
        return MediaSourceType::UNKNOWN;
    }
    return iter->second;
}
} // namespace firebolt::rialto::client
