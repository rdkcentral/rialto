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

#include "ImportCache.h"

#include <algorithm>

namespace firebolt::rialto::client
{
namespace
{
bool samePlane(const CapturePlane &first, const CapturePlane &second)
{
    return first.planeIndex == second.planeIndex && first.objectId == second.objectId &&
           first.offsetBytes == second.offsetBytes && first.lengthBytes == second.lengthBytes &&
           first.strideBytes == second.strideBytes;
}
} // namespace

ImportCacheKey ImportCacheKey::from(const CaptureFrameDescription &description, std::uint64_t period)
{
    return ImportCacheKey{period,
                          description.planes,
                          description.drmFormat,
                          description.drmModifier,
                          description.backingWidth,
                          description.backingHeight,
                          description.colour};
}

bool ImportCacheKey::operator==(const ImportCacheKey &other) const
{
    return period == other.period && drmFormat == other.drmFormat && drmModifier == other.drmModifier &&
           backingWidth == other.backingWidth && backingHeight == other.backingHeight && colour == other.colour &&
           std::equal(planes.begin(), planes.end(), other.planes.begin(), other.planes.end(), samePlane);
}

ImportCacheEntry::ImportCacheEntry(const ImportCacheKey &key, const std::shared_ptr<IImportedImage> &image,
                                   const std::shared_ptr<IFrameImporter> &importer)
    : m_key{key}, m_image{image}, m_importer{importer}
{
}

ImportCacheEntry::~ImportCacheEntry()
{
    if (m_importer && m_image)
    {
        m_importer->retire(m_image);
    }
}

std::shared_ptr<ImportCacheEntry> ImportCache::find(const ImportCacheKey &key)
{
    for (const auto &kEntry : m_entries)
    {
        if (kEntry->key() == key)
        {
            kEntry->lastUse = ++m_clock;
            return kEntry;
        }
    }
    return nullptr;
}

void ImportCache::insert(const std::shared_ptr<ImportCacheEntry> &entry)
{
    entry->lastUse = ++m_clock;
    m_entries.push_back(entry);

    while (m_bound != 0 && m_entries.size() > m_bound)
    {
        // only the cache holds an unused entry; frames hold the others. Counts only rise under the capture's
        // acquisition lock, so an entry seen as used here may at worst be evicted on a later acquisition.
        auto oldestUnused = m_entries.end();
        for (auto it = m_entries.begin(); it != m_entries.end(); ++it)
        {
            if (it->use_count() == 1 && (oldestUnused == m_entries.end() || (*it)->lastUse < (*oldestUnused)->lastUse))
                oldestUnused = it;
        }
        if (oldestUnused == m_entries.end())
            break; // every entry is in use by a frame
        m_entries.erase(oldestUnused);
    }
}
} // namespace firebolt::rialto::client
