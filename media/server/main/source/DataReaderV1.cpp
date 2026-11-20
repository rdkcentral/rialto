/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Sky UK
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

#include "DataReaderV1.h"
#include "RialtoServerLogging.h"
#include "ShmCommon.h"
#include "TypeConverters.h"
#include <cinttypes>
#include <cstring>

namespace firebolt::rialto::server
{
DataReaderV1::DataReaderV1(const MediaSourceType &mediaSourceType, std::uint8_t *buffer, std::uint32_t metadataOffset,
                           std::uint32_t numFrames, bool isBufferFull, std::uint32_t regionStart,
                           std::uint64_t regionEnd)
    : m_mediaSourceType{mediaSourceType}, m_buffer{buffer}, m_metadataOffset{metadataOffset}, m_numFrames{numFrames},
      m_isBufferFull{isBufferFull}, m_regionStart{regionStart}, m_regionEnd{regionEnd}
{
    RIALTO_SERVER_LOG_DEBUG("Detected Metadata in Version 1. Media source type: %s",
                            common::convertMediaSourceType(m_mediaSourceType));

    // numFrames comes from the client's IPC call - clamp it to the number of
    // metadata records that fit in the region before it can drive an OOB walk.
    const std::uint64_t kAvail{m_regionEnd > m_metadataOffset ? m_regionEnd - m_metadataOffset : 0};
    const std::uint64_t kMaxFrames{kAvail / common::METADATA_V1_SIZE_PER_FRAME_BYTES};
    if (m_numFrames > kMaxFrames)
    {
        RIALTO_SERVER_LOG_ERROR("numFrames %u exceeds region capacity %" PRIu64 " - clamping", m_numFrames, kMaxFrames);
        m_numFrames = static_cast<std::uint32_t>(kMaxFrames);
    }
    static_assert(sizeof(DataReaderV1::MetadataV1) == common::METADATA_V1_SIZE_PER_FRAME_BYTES,
                  "Size of firebolt::rialto::server::DataReaderV1::MetadataV1 is not equal to "
                  "firebolt::rialto::common::METADATA_V1_SIZE_PER_FRAME_BYTES");
}

IMediaPipeline::MediaSegmentVector DataReaderV1::readData() const
{
    IMediaPipeline::MediaSegmentVector mediaSegments;
    auto metadatas = readMetadata();
    for (const auto &metadata : metadatas)
    {
        if (m_mediaSourceType == MediaSourceType::AUDIO)
        {
            if (auto segment = createSegment<IMediaPipeline::MediaSegmentAudio>(metadata))
                mediaSegments.emplace_back(std::move(segment));
            else
                RIALTO_SERVER_LOG_ERROR("Skipping audio segment with out-of-range data offset/length");
        }
        else if (m_mediaSourceType == MediaSourceType::VIDEO)
        {
            if (auto segment = createSegment<IMediaPipeline::MediaSegmentVideo>(metadata))
                mediaSegments.emplace_back(std::move(segment));
            else
                RIALTO_SERVER_LOG_ERROR("Skipping video segment with out-of-range data offset/length");
        }
    }
    return mediaSegments;
}

bool DataReaderV1::isBufferFull() const
{
    return m_isBufferFull;
}

std::vector<DataReaderV1::MetadataV1> DataReaderV1::readMetadata() const
{
    std::vector<DataReaderV1::MetadataV1> result;
    std::uint8_t *regionMetadataOffset = m_buffer + m_metadataOffset;
    for (std::uint32_t frame = 0; frame < m_numFrames; ++frame)
    {
        std::uint8_t *frameMetadataOffset = regionMetadataOffset + (frame * common::METADATA_V1_SIZE_PER_FRAME_BYTES);
        // memcpy rather than a reinterpret_cast: the metadata region is not guaranteed aligned.
        DataReaderV1::MetadataV1 metadata;
        std::memcpy(&metadata, frameMetadataOffset, sizeof(metadata));
        result.emplace_back(metadata);
    }
    return result;
}
} // namespace firebolt::rialto::server
