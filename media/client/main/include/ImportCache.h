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

#ifndef FIREBOLT_RIALTO_CLIENT_IMPORT_CACHE_H_
#define FIREBOLT_RIALTO_CLIENT_IMPORT_CACHE_H_

#include "IFrameImporter.h"
#include "IVideoFrameCaptureIpc.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace firebolt::rialto::client
{
/**
 * @brief Identifies a reusable import (SoC capture spec 6.2): the invalidation period, the ordered plane-to-object
 *        mapping and the complete import description including colour. The graphics context is the capture's own.
 */
struct ImportCacheKey
{
    std::uint64_t period{0};
    std::vector<CapturePlane> planes;
    std::uint32_t drmFormat{0};
    std::uint64_t drmModifier{0};
    std::uint32_t backingWidth{0};
    std::uint32_t backingHeight{0};
    CaptureColour colour;

    static ImportCacheKey from(const CaptureFrameDescription &description, std::uint64_t period);
    bool operator==(const ImportCacheKey &other) const;
};

/**
 * @brief One import kept for reuse. Frames using it hold a reference; when the cache and every frame have dropped
 *        it, its image is handed to the importer to be retired.
 */
class ImportCacheEntry
{
public:
    ImportCacheEntry(const ImportCacheKey &key, const std::shared_ptr<IImportedImage> &image,
                     const std::shared_ptr<IFrameImporter> &importer);
    ~ImportCacheEntry();

    ImportCacheEntry(const ImportCacheEntry &) = delete;
    ImportCacheEntry &operator=(const ImportCacheEntry &) = delete;

    const ImportCacheKey &key() const { return m_key; }
    const std::shared_ptr<IImportedImage> &image() const { return m_image; }

    std::uint64_t lastUse{0}; /**< For least-recently-used eviction; changed only by the cache. */

private:
    const ImportCacheKey m_key;
    std::shared_ptr<IImportedImage> m_image;
    std::shared_ptr<IFrameImporter> m_importer;
};

/**
 * @brief The reusable imports of one capture, at most the admitted slot count of them (SoC capture spec 6.2).
 *
 * Not thread safe: the capture serializes its use. Entries may be destroyed on any thread when their last frame goes.
 */
class ImportCache
{
public:
    /** Sets the bound; 0 means not known yet, so nothing is evicted. */
    void setBound(std::uint32_t bound) { m_bound = bound; }

    /** @retval the matching entry, or null. */
    std::shared_ptr<ImportCacheEntry> find(const ImportCacheKey &key);

    /** Adds an entry, then evicts unused entries, least recently used first, while over the bound. */
    void insert(const std::shared_ptr<ImportCacheEntry> &entry);

    std::size_t size() const { return m_entries.size(); }

private:
    std::uint32_t m_bound{0};
    std::uint64_t m_clock{0};
    std::vector<std::shared_ptr<ImportCacheEntry>> m_entries;
};
} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_IMPORT_CACHE_H_
