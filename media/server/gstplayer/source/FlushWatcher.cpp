/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 Sky UK
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

#include "FlushWatcher.h"

namespace firebolt::rialto::server
{
void FlushWatcher::setFlushing(const MediaSourceType &type)
{
    std::unique_lock lock{m_mutex};
    if (MediaSourceType::UNKNOWN != type)
    {
        m_flushingSources.insert(type);
    }
}

void FlushWatcher::setFlushed(const MediaSourceType &type)
{
    std::unique_lock lock{m_mutex};
    m_flushingSources.erase(type);
}

bool FlushWatcher::isFlushOngoing() const
{
    std::unique_lock lock{m_mutex};
    return !m_flushingSources.empty();
}
} // namespace firebolt::rialto::server
