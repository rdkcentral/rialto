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

#include "MediaFrameWriterV2.h"
#include "RialtoCommonLogging.h"

namespace
{
/**
 * @brief The version of metadata this object shall write.
 */
constexpr uint32_t kMetadataVersion = 2U;

/**
 * @brief Convert SegmentAlignment to protobuf object
 */
firebolt::rialto::MediaSegmentMetadata_SegmentAlignment
convertSegmentAlignment(const firebolt::rialto::SegmentAlignment &alignment)
{
    switch (alignment)
    {
    case firebolt::rialto::SegmentAlignment::UNDEFINED:
    {
        return firebolt::rialto::MediaSegmentMetadata_SegmentAlignment_ALIGNMENT_UNDEFINED;
    }
    case firebolt::rialto::SegmentAlignment::NAL:
    {
        return firebolt::rialto::MediaSegmentMetadata_SegmentAlignment_ALIGNMENT_NAL;
    }
    case firebolt::rialto::SegmentAlignment::AU:
    {
        return firebolt::rialto::MediaSegmentMetadata_SegmentAlignment_ALIGNMENT_AU;
    }
    }
    return firebolt::rialto::MediaSegmentMetadata_SegmentAlignment_ALIGNMENT_UNDEFINED;
}
} // namespace

namespace firebolt::rialto::common
{
MediaFrameWriterV2::MediaFrameWriterV2(uint8_t *shmBuffer, const std::shared_ptr<MediaPlayerShmInfo> &shmInfo)
    : m_shmBuffer(shmBuffer), m_kMaxBytes(shmInfo->maxMediaBytes), m_bytesWritten(0U),
      m_dataOffset(shmInfo->mediaDataOffset), m_numFrames{0}
{
    RIALTO_COMMON_LOG_INFO("We are using a writer for Metadata V2");

    // Zero memory
    m_byteWriter.fillBytes(m_shmBuffer, shmInfo->metadataOffset, 0, shmInfo->maxMetadataBytes);
    m_byteWriter.fillBytes(m_shmBuffer, m_dataOffset, 0, m_kMaxBytes);

    // Set metadata version
    m_byteWriter.writeUint32(m_shmBuffer, shmInfo->metadataOffset, kMetadataVersion);
}

AddSegmentStatus MediaFrameWriterV2::writeFrame(const std::unique_ptr<IMediaPipeline::MediaSegment> &data)
try
{
    auto metadata{buildMetadata(data)};
    size_t metadataSize{metadata.ByteSizeLong()};
    if (m_bytesWritten + sizeof(metadataSize) + metadataSize + data->getDataLength() > m_kMaxBytes)
    {
        RIALTO_COMMON_LOG_ERROR("Not enough memory available to write MediaSegment");
        return AddSegmentStatus::NO_SPACE;
    }
    m_dataOffset = m_byteWriter.writeUint32(m_shmBuffer, m_dataOffset, static_cast<uint32_t>(metadataSize));
    if (!metadata.SerializeToArray(m_shmBuffer + m_dataOffset, metadataSize))
    {
        RIALTO_COMMON_LOG_ERROR("Failed to write type specific metadata - protobuf serialization failed.");
        m_dataOffset -= 4; // size of metadata size written in previous step
        return AddSegmentStatus::ERROR;
    }
    m_dataOffset += metadataSize;
    m_dataOffset = m_byteWriter.writeBytes(m_shmBuffer, m_dataOffset, data->getData(), data->getDataLength());

    // Track the amount of bytes written
    m_bytesWritten += sizeof(metadataSize) + metadataSize + data->getDataLength();
    ++m_numFrames;

    return AddSegmentStatus::OK;
}
catch (std::exception &e)
{
    RIALTO_COMMON_LOG_ERROR("Failed to write type specific metadata - exception occured");
    return AddSegmentStatus::ERROR;
}

MediaSegmentMetadata MediaFrameWriterV2::buildMetadata(const std::unique_ptr<IMediaPipeline::MediaSegment> &data) const
{
    MediaSegmentMetadata metadata;
    metadata.set_length(data->getDataLength());
    metadata.set_time_position(data->getTimeStamp());
    metadata.set_sample_duration(data->getDuration());
    metadata.set_stream_id(static_cast<uint32_t>(data->getId()));
    if (MediaSourceType::AUDIO == data->getType())
    {
        IMediaPipeline::MediaSegmentAudio &audioSegment = dynamic_cast<IMediaPipeline::MediaSegmentAudio &>(*data);
        metadata.set_sample_rate(static_cast<uint32_t>(audioSegment.getSampleRate()));
        metadata.set_channels_num(static_cast<uint32_t>(audioSegment.getNumberOfChannels()));
    }
    else if (MediaSourceType::VIDEO == data->getType())
    {
        IMediaPipeline::MediaSegmentVideo &videoSegment = dynamic_cast<IMediaPipeline::MediaSegmentVideo &>(*data);
        metadata.set_width(videoSegment.getWidth());
        metadata.set_height(videoSegment.getHeight());
    }
    else
    {
        RIALTO_COMMON_LOG_ERROR("Failed to write type specific metadata - media source type not known");
        throw std::exception();
    }
    if (!data->getExtraData().empty())
    {
        metadata.set_extra_data(std::string(data->getExtraData().begin(), data->getExtraData().end()));
    }
    if (SegmentAlignment::UNDEFINED != data->getSegmentAlignment())
    {
        metadata.set_segment_alignment(convertSegmentAlignment(data->getSegmentAlignment()));
    }
    if (data->getCodecData())
    {
        metadata.set_codec_data(std::string(data->getCodecData()->begin(), data->getCodecData()->end()));
    }
    if (data->isEncrypted())
    {
        metadata.set_media_key_session_id(data->getMediaKeySessionId());
        metadata.set_key_id(std::string(data->getKeyId().begin(), data->getKeyId().end()));
        metadata.set_init_vector(std::string(data->getInitVector().begin(), data->getInitVector().end()));
        metadata.set_init_with_last_15(data->getInitWithLast15());
        for (const auto &subSample : data->getSubSamples())
        {
            auto subSamplePair = metadata.mutable_sub_sample_info()->Add();
            subSamplePair->set_num_clear_bytes(static_cast<uint32_t>(subSample.numClearBytes));
            subSamplePair->set_num_encrypted_bytes(static_cast<uint32_t>(subSample.numEncryptedBytes));
        }
    }
    return metadata;
}
} // namespace firebolt::rialto::common
