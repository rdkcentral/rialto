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

#include "MediaPipelineTestMethods.h"
#include "MediaPipelineProtoRequestMatchers.h"
#include "MediaPipelineProtoUtils.h"
#include "MediaSegments.h"
#include "MetadataProtoMatchers.h"
#include "MetadataProtoUtils.h"
#include "metadata.pb.h"
#include <memory>
#include <string>
#include <vector>

namespace
{
constexpr VideoRequirements kVideoRequirements{3840, 2160};
constexpr int32_t kSessionId{10};
constexpr MediaType kMediaType = MediaType::MSE;
const std::string kMimeType = "mime";
const std::string kUrl = "mse://1";
constexpr int32_t kAudioSourceId = 1;
constexpr int32_t kVideoSourceId = 2;
constexpr firebolt::rialto::SegmentAlignment kAlignment = firebolt::rialto::SegmentAlignment::UNDEFINED;
constexpr firebolt::rialto::StreamFormat kStreamFormat = firebolt::rialto::StreamFormat::RAW;
constexpr int32_t kWidthUhd = 3840;
constexpr int32_t kHeightUhd = 2160;
constexpr uint32_t kNumberOfChannels = 6;
constexpr uint32_t kSampleRate = 48000;
constexpr bool kHasNoDrm = false;
const std::string kCodecSpecificConfigStr = "1243567";
const std::shared_ptr<firebolt::rialto::CodecData> kCodecData{std::make_shared<firebolt::rialto::CodecData>()};
constexpr size_t kFrameCountBeforePreroll = 3;
constexpr size_t kMaxFrameCount = 20;
const std::shared_ptr<MediaPlayerShmInfo> kNullShmInfo;
constexpr int64_t kTimeStamp = 1701352637000;
constexpr int64_t kDuration = 1;
firebolt::rialto::Fraction kFrameRate = {1, 1};
constexpr VideoRequirements kVideoRequirementsSecondary{456, 123};
constexpr int32_t kSessionIdSecondary{11};
constexpr uint32_t kPrimaryPartition{0};
constexpr uint32_t kSecondaryPartition{1};
} // namespace

namespace firebolt::rialto::client::ct
{
MediaPipelineTestMethods::MediaPipelineTestMethods(const std::vector<firebolt::rialto::MediaPlayerShmInfo> &audioShmInfo, const std::vector<firebolt::rialto::MediaPlayerShmInfo> &videoShmInfo)
    : m_mediaPipelineClientMock{std::make_shared<StrictMock<MediaPipelineClientMock>>()},
      m_mediaPipelineModuleMock{std::make_shared<StrictMock<MediaPipelineModuleMock>>()},
      m_mediaPipelineClientSecondaryMock{std::make_shared<StrictMock<MediaPipelineClientMock>>()}, m_kAudioShmInfo{audioShmInfo},
      m_kVideoShmInfo{videoShmInfo}
{
    kCodecData->data = std::vector<std::uint8_t>{'T', 'E', 'S', 'T'};
    kCodecData->type = firebolt::rialto::CodecDataType::BUFFER;

    // MediaPipeline requires 2 AV partitions
    m_locationToWriteAudio.push_back(std::make_shared<MediaPlayerShmInfo>());
    m_locationToWriteAudio.push_back(std::make_shared<MediaPlayerShmInfo>());
    m_locationToWriteVideo.push_back(std::make_shared<MediaPlayerShmInfo>());
    m_locationToWriteVideo.push_back(std::make_shared<MediaPlayerShmInfo>());
    resetWriteLocation(kPrimaryPartition);
    resetWriteLocation(kSecondaryPartition);
}

MediaPipelineTestMethods::~MediaPipelineTestMethods() {}

void MediaPipelineTestMethods::resetWriteLocation(uint32_t partitionId)
{
    m_locationToWriteAudio[partitionId]->maxMetadataBytes = m_kAudioShmInfo[partitionId].maxMetadataBytes;
    m_locationToWriteAudio[partitionId]->metadataOffset = m_kAudioShmInfo[partitionId].metadataOffset;
    m_locationToWriteAudio[partitionId]->maxMediaBytes = m_kAudioShmInfo[partitionId].maxMediaBytes;
    m_locationToWriteAudio[partitionId]->mediaDataOffset = m_kAudioShmInfo[partitionId].mediaDataOffset;
    m_locationToWriteVideo[partitionId]->maxMetadataBytes = m_kVideoShmInfo[partitionId].maxMetadataBytes;
    m_locationToWriteVideo[partitionId]->metadataOffset = m_kVideoShmInfo[partitionId].metadataOffset;
    m_locationToWriteVideo[partitionId]->maxMediaBytes = m_kVideoShmInfo[partitionId].maxMediaBytes;
    m_locationToWriteVideo[partitionId]->mediaDataOffset = m_kVideoShmInfo[partitionId].mediaDataOffset;
}

void MediaPipelineTestMethods::shouldCreateMediaSession()
{
    shouldCreateMediaSessionInternal(kSessionId, kVideoRequirements);
}

void MediaPipelineTestMethods::shouldCreateMediaSessionSecondary()
{
    shouldCreateMediaSessionInternal(kSessionIdSecondary, kVideoRequirementsSecondary);
}

void MediaPipelineTestMethods::shouldCreateMediaSessionInternal(const int32_t sessionId, const VideoRequirements &videoRequirements)
{
    EXPECT_CALL(*m_mediaPipelineModuleMock,
                createSession(_, createSessionRequestMatcher(videoRequirements.maxWidth, videoRequirements.maxHeight),
                              _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaPipelineModuleMock->createSessionResponse(sessionId)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn)))); 
}

void MediaPipelineTestMethods::createMediaPipeline()
{
    createMediaPipelineInternal(m_mediaPipeline, m_mediaPipelineClientMock, kVideoRequirements);
}

void MediaPipelineTestMethods::createMediaPipelineSecondary()
{
    createMediaPipelineInternal(m_mediaPipelineSecondary, m_mediaPipelineClientSecondaryMock, kVideoRequirementsSecondary);
}

void MediaPipelineTestMethods::createMediaPipelineInternal(std::unique_ptr<IMediaPipeline> &mediaPipeline, const std::shared_ptr<StrictMock<MediaPipelineClientMock>> &client, const VideoRequirements &videoRequirements)
{
    m_mediaPipelineFactory = firebolt::rialto::IMediaPipelineFactory::createFactory();
    mediaPipeline = m_mediaPipelineFactory->createMediaPipeline(client, videoRequirements);
    EXPECT_NE(mediaPipeline, nullptr);
}

void MediaPipelineTestMethods::shouldLoad()
{
    shouldLoadInternal(kSessionId, kMediaType, kMimeType, kUrl);
}

void MediaPipelineTestMethods::shouldLoadSecondary()
{
    shouldLoadInternal(kSessionIdSecondary, kMediaType, kMimeType, kUrl); //TODO: Different?
}

void MediaPipelineTestMethods::shouldLoadInternal(const int32_t sessionId, const MediaType &mediaType, const std::string &mimeType, const std::string &url)
{
    EXPECT_CALL(*m_mediaPipelineModuleMock,
                load(_, loadRequestMatcher(sessionId, convertMediaType(mediaType), mimeType, url), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn)));
}

void MediaPipelineTestMethods::load()
{
    EXPECT_EQ(m_mediaPipeline->load(kMediaType, kMimeType, kUrl), true);
}

void MediaPipelineTestMethods::loadSecondary()
{
    EXPECT_EQ(m_mediaPipelineSecondary->load(kMediaType, kMimeType, kUrl), true);
}

void MediaPipelineTestMethods::shouldNotifyNetworkStateBuffering()
{
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyNetworkState(NetworkState::BUFFERING))
        .WillOnce(Invoke(this, &MediaPipelineTestMethods::notifyEvent));
}

void MediaPipelineTestMethods::sendNotifyNetworkStateBuffering()
{
    getServerStub()->notifyNetworkStateChangeEvent(kSessionId, NetworkState::BUFFERING);
    waitEvent();
}

void MediaPipelineTestMethods::shouldPause()
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, pause(_, pauseRequestMatcher(kSessionId), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn)));
}

void MediaPipelineTestMethods::pause()
{
    EXPECT_EQ(m_mediaPipeline->pause(), true);
}

void MediaPipelineTestMethods::shouldAttachVideoSource()
{
    shouldAttachVideoSourceInternal(kSessionId, kMimeType, kHasNoDrm, kWidthUhd, kHeightUhd, kAlignment, kCodecData, kStreamFormat);
}

void MediaPipelineTestMethods::shouldAttachVideoSourceSecondary()
{
    shouldAttachVideoSourceInternal(kSessionIdSecondary, kMimeType, kHasNoDrm, kWidthUhd, kHeightUhd, kAlignment, kCodecData, kStreamFormat); //TODO: Different?
}

void MediaPipelineTestMethods::shouldAttachVideoSourceInternal(const int32_t sessionId, const std::string &mimeType, bool hasNoDrm, const int32_t width, const int32_t height, const firebolt::rialto::SegmentAlignment &alignment, const std::shared_ptr<firebolt::rialto::CodecData> &codacData, const firebolt::rialto::StreamFormat &streamFormat)
{
    EXPECT_CALL(*m_mediaPipelineModuleMock,
                attachSource(_,
                             attachSourceRequestMatcherVideo(sessionId, mimeType.c_str(), hasNoDrm, width, //TODO: Different?
                                                             height, alignment, codacData,
                                                             convertStreamFormat(streamFormat)),
                             _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaPipelineModuleMock->attachSourceResponse(kVideoSourceId)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn))));
}

void MediaPipelineTestMethods::attachSourceVideo()
{
    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceVideo>(kMimeType.c_str(), kHasNoDrm, kWidthUhd, kHeightUhd,
                                                           kAlignment, kStreamFormat, kCodecData);
    EXPECT_EQ(m_mediaPipeline->attachSource(mediaSource), true);
}

void MediaPipelineTestMethods::shouldAttachAudioSource()
{
    EXPECT_CALL(*m_mediaPipelineModuleMock,
                attachSource(_,
                             attachSourceRequestMatcherAudio(kSessionId, kMimeType.c_str(), kHasNoDrm, kAlignment,
                                                             kNumberOfChannels, kSampleRate, kCodecSpecificConfigStr,
                                                             kCodecData, convertStreamFormat(kStreamFormat)),
                             _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaPipelineModuleMock->attachSourceResponse(kAudioSourceId)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn))));
}

void MediaPipelineTestMethods::attachSourceAudio()
{
    std::vector<uint8_t> codecSpecificConfig;
    codecSpecificConfig.assign(kCodecSpecificConfigStr.begin(), kCodecSpecificConfigStr.end());
    AudioConfig audioConfig{kNumberOfChannels, kSampleRate, codecSpecificConfig};

    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceAudio>(kMimeType.c_str(), kHasNoDrm, audioConfig, kAlignment,
                                                           kStreamFormat, kCodecData);
    EXPECT_EQ(m_mediaPipeline->attachSource(mediaSource), true);
}

void MediaPipelineTestMethods::shouldAllSourcesAttached()
{
    shouldAllSourcesAttachedInternal(kSessionId);
}

void MediaPipelineTestMethods::shouldAllSourcesAttachedSecondary()
{
    shouldAllSourcesAttachedInternal(kSessionIdSecondary);
}

void MediaPipelineTestMethods::shouldAllSourcesAttachedInternal(const int32_t sessionId)
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, allSourcesAttached(_, allSourcesAttachedRequestMatcher(sessionId), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn)));
}

void MediaPipelineTestMethods::allSourcesAttached()
{
    EXPECT_EQ(m_mediaPipeline->allSourcesAttached(), true);
}

void MediaPipelineTestMethods::shouldNotifyPlaybackStateIdle()
{
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyPlaybackState(PlaybackState::IDLE))
        .WillOnce(Invoke(this, &MediaPipelineTestMethods::notifyEvent));
}

void MediaPipelineTestMethods::sendNotifyPlaybackStateIdle()
{
    getServerStub()->notifyPlaybackStateChangeEvent(kSessionId, PlaybackState::IDLE);
    waitEvent();
}

void MediaPipelineTestMethods::shouldNotifyNeedDataAudioBeforePreroll()
{
    EXPECT_CALL(*m_mediaPipelineClientMock,
                notifyNeedMediaData(kAudioSourceId, kFrameCountBeforePreroll, m_needDataRequestId, kNullShmInfo))
        .WillOnce(InvokeWithoutArgs(
            [&]()
            {
                // Set the firstSegment flag
                m_firstSegmentOfNeedData = true;
                notifyEvent();
            }));
}

void MediaPipelineTestMethods::sendNotifyNeedDataAudioBeforePreroll()
{
    getServerStub()->notifyNeedMediaDataEvent(kSessionId, kAudioSourceId, kFrameCountBeforePreroll, m_needDataRequestId,
                                              m_locationToWriteAudio[kPrimaryPartition]);
    waitEvent();
}

void MediaPipelineTestMethods::shouldNotifyNeedDataVideoBeforePreroll()
{
    EXPECT_CALL(*m_mediaPipelineClientMock,
                notifyNeedMediaData(kVideoSourceId, kFrameCountBeforePreroll, m_needDataRequestId, kNullShmInfo))
        .WillOnce(InvokeWithoutArgs(
            [&]()
            {
                // Set the firstSegment flag
                m_firstSegmentOfNeedData = true;
                notifyEvent();
            }));
}

void MediaPipelineTestMethods::sendNotifyNeedDataVideoBeforePreroll()
{
    getServerStub()->notifyNeedMediaDataEvent(kSessionId, kVideoSourceId, kFrameCountBeforePreroll, m_needDataRequestId,
                                              m_locationToWriteVideo[kPrimaryPartition]);
    waitEvent();
}

void MediaPipelineTestMethods::haveDataOk()
{
    EXPECT_EQ(m_mediaPipeline->haveData(MediaSourceStatus::OK, m_needDataRequestId), true);
}

uint32_t MediaPipelineTestMethods::getTimestamp(uint32_t segmentId)
{
    return kTimeStamp + 1000 * (segmentId + 1);
}

int32_t MediaPipelineTestMethods::addSegmentMseAudio()
{
    EXPECT_LT(m_audioSegmentCount, sizeof(kAudioSegments) / sizeof(kAudioSegments[0]));

    std::unique_ptr<IMediaPipeline::MediaSegment> mseData =
        std::make_unique<IMediaPipeline::MediaSegmentAudio>(kAudioSourceId, getTimestamp(m_audioSegmentCount),
                                                            kDuration, kSampleRate, kNumberOfChannels);
    mseData->setData(kAudioSegments[m_audioSegmentCount].size(),
                     (const uint8_t *)kAudioSegments[m_audioSegmentCount].c_str());
    EXPECT_EQ(m_mediaPipeline->addSegment(m_needDataRequestId, mseData), AddSegmentStatus::OK);

    // Store where the segment should be written so we can check the data
    int32_t segmentId = m_audioSegmentCount;
    writtenAudioSegments.insert({segmentId, *m_locationToWriteAudio[kPrimaryPartition]});

    incrementWriteLocation(kAudioSegments[segmentId].size(), m_locationToWriteAudio[kPrimaryPartition]);

    m_audioSegmentCount++;
    return segmentId;
}

void MediaPipelineTestMethods::incrementWriteLocation(uint32_t sizeOfSegmentData,
                                                      const std::shared_ptr<MediaPlayerShmInfo> &writeLocation)
{
    // Calibrate the shm info based on segment written
    uint32_t metadataSize = 0;
    if (m_firstSegmentOfNeedData)
    {
        // For first segment we write the metadata version
        // In V2 we dont set anymore data in the metadata buffer
        metadataSize += sizeof(uint32_t);
        m_firstSegmentOfNeedData = false;
    }
    uint32_t metadataBytesWrittenInMedia =
        sizeof(uint32_t) +
        *reinterpret_cast<uint32_t *>(reinterpret_cast<uint8_t *>(getShmAddress()) + writeLocation->mediaDataOffset);
    uint32_t segmentDataSize = sizeOfSegmentData + metadataBytesWrittenInMedia;
    writeLocation->maxMetadataBytes = writeLocation->maxMetadataBytes - metadataSize;
    writeLocation->metadataOffset = writeLocation->metadataOffset + metadataSize;
    writeLocation->maxMediaBytes = writeLocation->maxMediaBytes - segmentDataSize;
    writeLocation->mediaDataOffset = writeLocation->mediaDataOffset + segmentDataSize;
}

void MediaPipelineTestMethods::checkMseAudioSegmentWritten(int32_t segmentId)
{
    auto it = writtenAudioSegments.find(segmentId);
    ASSERT_NE(it, writtenAudioSegments.end());

    uint8_t *dataPosition{reinterpret_cast<uint8_t *>(getShmAddress()) + it->second.mediaDataOffset};
    std::uint32_t *metadataSize{reinterpret_cast<uint32_t *>(dataPosition)};
    dataPosition += sizeof(uint32_t);
    MediaSegmentMetadata metadata;
    ASSERT_TRUE(metadata.ParseFromArray(dataPosition, *metadataSize));

    checkAudioMetadata(metadata, segmentId);
    checkHasNoVideoMetadata(metadata);
    checkHasNoEncryptionMetadata(metadata);
    checkHasNoCodacData(metadata);
    checkHasNoSegmentAlignment(metadata);
    checkHasNoExtraData(metadata);
    checkSegmentData(metadata, dataPosition += *metadataSize, kAudioSegments[segmentId]);
}

int32_t MediaPipelineTestMethods::addSegmentMseVideo()
{
    EXPECT_LT(m_videoSegmentCount, sizeof(kVideoSegments) / sizeof(kVideoSegments[0]));

    std::unique_ptr<IMediaPipeline::MediaSegment> mseData =
        std::make_unique<IMediaPipeline::MediaSegmentVideo>(kVideoSourceId, getTimestamp(m_videoSegmentCount),
                                                            kDuration, kWidthUhd, kHeightUhd, kFrameRate);
    mseData->setData(kVideoSegments[m_videoSegmentCount].size(),
                     (const uint8_t *)kVideoSegments[m_videoSegmentCount].c_str());
    EXPECT_EQ(m_mediaPipeline->addSegment(m_needDataRequestId, mseData), AddSegmentStatus::OK);

    // Store where the segment should be written so we can check the data
    int32_t segmentId = m_videoSegmentCount;
    writtenVideoSegments.insert({segmentId, *m_locationToWriteVideo[kPrimaryPartition]});

    incrementWriteLocation(kVideoSegments[m_videoSegmentCount].size(), m_locationToWriteVideo[kPrimaryPartition]);

    m_videoSegmentCount++;
    return segmentId;
}

void MediaPipelineTestMethods::checkMseVideoSegmentWritten(int32_t segmentId)
{
    auto it = writtenVideoSegments.find(segmentId);
    ASSERT_NE(it, writtenVideoSegments.end());

    uint8_t *dataPosition{reinterpret_cast<uint8_t *>(getShmAddress()) + it->second.mediaDataOffset};
    std::uint32_t *metadataSize{reinterpret_cast<uint32_t *>(dataPosition)};
    dataPosition += sizeof(uint32_t);
    MediaSegmentMetadata metadata;
    ASSERT_TRUE(metadata.ParseFromArray(dataPosition, *metadataSize));

    checkVideoMetadata(metadata, segmentId);
    checkHasNoAudioMetadata(metadata);
    checkHasNoEncryptionMetadata(metadata);
    checkHasNoCodacData(metadata);
    checkHasNoSegmentAlignment(metadata);
    checkHasNoExtraData(metadata);
    checkSegmentData(metadata, dataPosition += *metadataSize, kVideoSegments[segmentId]);
}

void MediaPipelineTestMethods::checkAudioMetadata(const MediaSegmentMetadata &metadata, uint32_t segmentId)
{
    EXPECT_TRUE(metadata.has_length());
    EXPECT_EQ(metadata.length(), kAudioSegments[segmentId].size());
    EXPECT_TRUE(metadata.has_time_position());
    EXPECT_EQ(metadata.time_position(), getTimestamp(segmentId));
    EXPECT_TRUE(metadata.has_sample_duration());
    EXPECT_EQ(metadata.sample_duration(), kDuration);
    EXPECT_TRUE(metadata.has_stream_id());
    EXPECT_EQ(metadata.stream_id(), kAudioSourceId);
    EXPECT_TRUE(metadata.has_sample_rate());
    EXPECT_EQ(metadata.sample_rate(), kSampleRate);
    EXPECT_TRUE(metadata.channels_num());
    EXPECT_EQ(metadata.channels_num(), kNumberOfChannels);
}

void MediaPipelineTestMethods::checkHasNoAudioMetadata(const MediaSegmentMetadata &metadata)
{
    EXPECT_FALSE(metadata.has_sample_rate());
    EXPECT_FALSE(metadata.channels_num());
}

void MediaPipelineTestMethods::checkVideoMetadata(const MediaSegmentMetadata &metadata, uint32_t segmentId)
{
    EXPECT_TRUE(metadata.has_length());
    EXPECT_EQ(metadata.length(), kVideoSegments[segmentId].size());
    EXPECT_TRUE(metadata.has_time_position());
    EXPECT_EQ(metadata.time_position(), getTimestamp(segmentId));
    EXPECT_TRUE(metadata.has_sample_duration());
    EXPECT_EQ(metadata.sample_duration(), kDuration);
    EXPECT_TRUE(metadata.has_stream_id());
    EXPECT_EQ(metadata.stream_id(), kVideoSourceId);
    EXPECT_TRUE(metadata.has_width());
    EXPECT_EQ(metadata.width(), kWidthUhd);
    EXPECT_TRUE(metadata.has_height());
    EXPECT_EQ(metadata.height(), kHeightUhd);
    EXPECT_TRUE(metadata.has_frame_rate());
    EXPECT_THAT(metadata.frame_rate(), frameRateMatcher(kFrameRate));
}

void MediaPipelineTestMethods::checkHasNoVideoMetadata(const MediaSegmentMetadata &metadata)
{
    EXPECT_FALSE(metadata.has_width());
    EXPECT_FALSE(metadata.has_height());
    EXPECT_FALSE(metadata.has_frame_rate());
}

void MediaPipelineTestMethods::checkHasNoEncryptionMetadata(const MediaSegmentMetadata &metadata)
{
    EXPECT_FALSE(metadata.has_media_key_session_id());
    EXPECT_FALSE(metadata.has_key_id());
    EXPECT_FALSE(metadata.has_init_vector());
    EXPECT_FALSE(metadata.has_init_with_last_15());
    EXPECT_EQ(metadata.sub_sample_info().size(), 0);
    EXPECT_FALSE(metadata.has_cipher_mode());
    EXPECT_FALSE(metadata.has_crypt());
    EXPECT_FALSE(metadata.has_skip());
}

void MediaPipelineTestMethods::checkHasNoCodacData(const MediaSegmentMetadata &metadata)
{
    EXPECT_FALSE(metadata.has_codec_data());
}

void MediaPipelineTestMethods::checkHasNoSegmentAlignment(const MediaSegmentMetadata &metadata)
{
    EXPECT_FALSE(metadata.has_segment_alignment());
}

void MediaPipelineTestMethods::checkHasNoExtraData(const MediaSegmentMetadata &metadata)
{
    EXPECT_FALSE(metadata.has_extra_data());
}

void MediaPipelineTestMethods::checkSegmentData(const MediaSegmentMetadata &metadata, uint8_t *dataPtr,
                                                const std::string &expectedSegmentData)
{
    EXPECT_TRUE(metadata.has_length());
    std::string data = std::string(reinterpret_cast<char *>(dataPtr), metadata.length());
    EXPECT_EQ(data, expectedSegmentData);
}

void MediaPipelineTestMethods::shouldNotifyNetworkStateBuffered()
{
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyNetworkState(NetworkState::BUFFERED))
        .WillOnce(Invoke(this, &MediaPipelineTestMethods::notifyEvent));
}

void MediaPipelineTestMethods::sendNotifyNetworkStateBuffered()
{
    getServerStub()->notifyNetworkStateChangeEvent(kSessionId, NetworkState::BUFFERED);
    waitEvent();
}

void MediaPipelineTestMethods::shouldNotifyPlaybackStatePaused()
{
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyPlaybackState(PlaybackState::PAUSED))
        .WillOnce(Invoke(this, &MediaPipelineTestMethods::notifyEvent));
}

void MediaPipelineTestMethods::sendNotifyPlaybackStatePaused()
{
    getServerStub()->notifyPlaybackStateChangeEvent(kSessionId, PlaybackState::PAUSED);
    waitEvent();
}

void MediaPipelineTestMethods::shouldNotifyPlaybackStatePlaying()
{
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyPlaybackState(PlaybackState::PLAYING))
        .WillOnce(Invoke(this, &MediaPipelineTestMethods::notifyEvent));
}

void MediaPipelineTestMethods::sendNotifyPlaybackStatePlaying()
{
    getServerStub()->notifyPlaybackStateChangeEvent(kSessionId, PlaybackState::PLAYING);
    waitEvent();
}

void MediaPipelineTestMethods::shouldPlay()
{
    shouldPlayInternal(kSessionId);
}

void MediaPipelineTestMethods::shouldPlaySecondary()
{
    shouldPlayInternal(kSessionIdSecondary);
}

void MediaPipelineTestMethods::shouldPlayInternal(const int32_t sessionId)
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, play(_, playRequestMatcher(sessionId), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn)));
}

void MediaPipelineTestMethods::play()
{
    EXPECT_EQ(m_mediaPipeline->play(), true);
}

void MediaPipelineTestMethods::shouldNotifyNeedDataAudioAfterPreroll()
{
    EXPECT_CALL(*m_mediaPipelineClientMock,
                notifyNeedMediaData(kAudioSourceId, kMaxFrameCount, m_needDataRequestId, kNullShmInfo))
        .WillOnce(InvokeWithoutArgs(
            [&]()
            {
                // Set the firstSegment flag
                m_firstSegmentOfNeedData = true;
                notifyEvent();
            }));
}

void MediaPipelineTestMethods::sendNotifyNeedDataAudioAfterPreroll()
{
    getServerStub()->notifyNeedMediaDataEvent(kSessionId, kAudioSourceId, kMaxFrameCount, m_needDataRequestId,
                                              m_locationToWriteAudio[kPrimaryPartition]);
    waitEvent();
}

void MediaPipelineTestMethods::shouldNotifyNeedDataVideoAfterPreroll()
{
    EXPECT_CALL(*m_mediaPipelineClientMock,
                notifyNeedMediaData(kVideoSourceId, kMaxFrameCount, m_needDataRequestId, kNullShmInfo))
        .WillOnce(InvokeWithoutArgs(
            [&]()
            {
                // Set the firstSegment flag
                m_firstSegmentOfNeedData = true;
                notifyEvent();
            }));
}

void MediaPipelineTestMethods::sendNotifyNeedDataVideoAfterPreroll()
{
    getServerStub()->notifyNeedMediaDataEvent(kSessionId, kVideoSourceId, kMaxFrameCount, m_needDataRequestId,
                                              m_locationToWriteVideo[kPrimaryPartition]);
    waitEvent();
}

void MediaPipelineTestMethods::shouldHaveDataAfterPreroll()
{
    shouldHaveDataInternal(kSessionId, MediaSourceStatus::OK, kMaxFrameCount, kPrimaryPartition);
}

void MediaPipelineTestMethods::shouldHaveDataBeforePreroll()
{
    shouldHaveDataInternal(kSessionId, MediaSourceStatus::OK, kFrameCountBeforePreroll, kPrimaryPartition);
}

void MediaPipelineTestMethods::shouldHaveDataOk(size_t framesWritten)
{
    shouldHaveDataInternal(kSessionId, MediaSourceStatus::OK, framesWritten, kPrimaryPartition);
}

void MediaPipelineTestMethods::shouldHaveDataOkSecondary(size_t framesWritten)
{
    shouldHaveDataInternal(kSessionIdSecondary, MediaSourceStatus::OK, framesWritten, kSecondaryPartition);
}

void MediaPipelineTestMethods::shouldHaveDataInternal(const int32_t sessionId, const MediaSourceStatus status, const size_t framesWritten, const uint32_t partition)
{
    EXPECT_CALL(*m_mediaPipelineModuleMock,
                haveData(_,
                         haveDataRequestMatcher(sessionId, convertMediaSourceStatus(status),
                                                framesWritten, m_needDataRequestId),
                         _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(
            [&](::google::protobuf::RpcController *controller, ::google::protobuf::Closure *done)
            {
                // Increment needData request Id
                m_needDataRequestId++;
                m_mediaPipelineModuleMock->defaultReturn(controller, done);
                resetWriteLocation(partition);
            })));
}

void MediaPipelineTestMethods::shouldHaveDataEos(size_t framesWritten)
{
    shouldHaveDataInternal(kSessionId, MediaSourceStatus::EOS, framesWritten, kPrimaryPartition);
}

void MediaPipelineTestMethods::haveDataEos()
{
    EXPECT_EQ(m_mediaPipeline->haveData(MediaSourceStatus::EOS, m_needDataRequestId), true);
}

void MediaPipelineTestMethods::shouldNotifyPlaybackStateEndOfStream()
{
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyPlaybackState(PlaybackState::END_OF_STREAM))
        .WillOnce(Invoke(this, &MediaPipelineTestMethods::notifyEvent));
}

void MediaPipelineTestMethods::sendNotifyPlaybackStateEndOfStream()
{
    getServerStub()->notifyPlaybackStateChangeEvent(kSessionId, PlaybackState::END_OF_STREAM);
    waitEvent();
}

void MediaPipelineTestMethods::shouldRemoveVideoSource()
{
    shouldRemoveVideoSourceInternal(kSessionId);
}

void MediaPipelineTestMethods::shouldRemoveVideoSourceSecondary()
{
    shouldRemoveVideoSourceInternal(kSessionIdSecondary);
}

void MediaPipelineTestMethods::shouldRemoveVideoSourceInternal(const int32_t sessionId)
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, removeSource(_, removeSourceRequestMatcher(sessionId, kVideoSourceId), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn)));
}

void MediaPipelineTestMethods::removeSourceVideo()
{
    EXPECT_EQ(m_mediaPipeline->removeSource(kVideoSourceId), true);
}

void MediaPipelineTestMethods::shouldRemoveAudioSource()
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, removeSource(_, removeSourceRequestMatcher(kSessionId, kAudioSourceId), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn)));
}

void MediaPipelineTestMethods::removeSourceAudio()
{
    EXPECT_EQ(m_mediaPipeline->removeSource(kAudioSourceId), true);
}

void MediaPipelineTestMethods::shouldStop()
{
    shouldStopInternal(kSessionId);
}

void MediaPipelineTestMethods::shouldStopSecondary()
{
    shouldStopInternal(kSessionIdSecondary);
}

void MediaPipelineTestMethods::shouldStopInternal(const int32_t sessionId)
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, stop(_, stopRequestMatcher(sessionId), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn)));
}

void MediaPipelineTestMethods::stop()
{
    EXPECT_EQ(m_mediaPipeline->stop(), true);
}

void MediaPipelineTestMethods::shouldNotifyPlaybackStateStopped()
{
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyPlaybackState(PlaybackState::STOPPED))
        .WillOnce(Invoke(this, &MediaPipelineTestMethods::notifyEvent));
}

void MediaPipelineTestMethods::sendNotifyPlaybackStateStopped()
{
    getServerStub()->notifyPlaybackStateChangeEvent(kSessionId, PlaybackState::STOPPED);
    waitEvent();
}

void MediaPipelineTestMethods::shouldDestroyMediaSession()
{
    shouldDestroyMediaSessionInternal(kSessionId);
}

void MediaPipelineTestMethods::shouldDestroyMediaSessionSecondary()
{
    shouldDestroyMediaSessionInternal(kSessionIdSecondary);
}

void MediaPipelineTestMethods::shouldDestroyMediaSessionInternal(const int32_t sessionId)
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, destroySession(_, destroySessionRequestMatcher(sessionId), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn)));
}

void MediaPipelineTestMethods::destroyMediaPipeline()
{
    m_mediaPipeline.reset();
}

void MediaPipelineTestMethods::startAudioVideoMediaSessionWaitForPreroll()
{
    // Create a new media session
    MediaPipelineTestMethods::shouldCreateMediaSession();
    MediaPipelineTestMethods::createMediaPipeline();

    // Load content
    MediaPipelineTestMethods::shouldLoad();
    MediaPipelineTestMethods::load();
    MediaPipelineTestMethods::shouldNotifyNetworkStateBuffering();
    MediaPipelineTestMethods::sendNotifyNetworkStateBuffering();

    // Attach all sources
    MediaPipelineTestMethods::shouldAttachVideoSource();
    MediaPipelineTestMethods::attachSourceVideo();
    MediaPipelineTestMethods::shouldAttachAudioSource();
    MediaPipelineTestMethods::attachSourceAudio();
    MediaPipelineTestMethods::shouldAllSourcesAttached();
    MediaPipelineTestMethods::allSourcesAttached();

    // Notify Idle
    MediaPipelineTestMethods::shouldNotifyPlaybackStateIdle();
    MediaPipelineTestMethods::sendNotifyPlaybackStateIdle();

    // Pause
    MediaPipelineTestMethods::shouldPause();
    MediaPipelineTestMethods::pause();
}

void MediaPipelineTestMethods::startAudioVideoMediaSessionPrerollPaused()
{
    startAudioVideoMediaSessionWaitForPreroll();

    // Write 1 audio frame
    MediaPipelineTestMethods::shouldNotifyNeedDataAudioBeforePreroll();
    MediaPipelineTestMethods::sendNotifyNeedDataAudioBeforePreroll();
    uint32_t segmentId = MediaPipelineTestMethods::addSegmentMseAudio();
    MediaPipelineTestMethods::checkMseAudioSegmentWritten(segmentId);
    MediaPipelineTestMethods::shouldHaveDataOk(1);
    MediaPipelineTestMethods::haveDataOk();

    // Write 1 video frame
    MediaPipelineTestMethods::shouldNotifyNeedDataVideoBeforePreroll();
    MediaPipelineTestMethods::sendNotifyNeedDataVideoBeforePreroll();
    segmentId = MediaPipelineTestMethods::addSegmentMseVideo();
    MediaPipelineTestMethods::checkMseVideoSegmentWritten(segmentId);
    MediaPipelineTestMethods::shouldHaveDataOk(1);
    MediaPipelineTestMethods::haveDataOk();

    // Preroll paused
    MediaPipelineTestMethods::shouldNotifyNetworkStateBuffered();
    MediaPipelineTestMethods::sendNotifyNetworkStateBuffered();
    MediaPipelineTestMethods::shouldNotifyPlaybackStatePaused();
    MediaPipelineTestMethods::sendNotifyPlaybackStatePaused();
}

void MediaPipelineTestMethods::endAudioVideoMediaSession()
{
    // Remove sources
    MediaPipelineTestMethods::shouldRemoveVideoSource();
    MediaPipelineTestMethods::removeSourceVideo();
    MediaPipelineTestMethods::shouldRemoveAudioSource();
    MediaPipelineTestMethods::removeSourceAudio();

    // Stop
    MediaPipelineTestMethods::shouldStop();
    MediaPipelineTestMethods::stop();
    MediaPipelineTestMethods::shouldNotifyPlaybackStateStopped();
    MediaPipelineTestMethods::sendNotifyPlaybackStateStopped();

    // Destroy media session
    MediaPipelineTestMethods::shouldDestroyMediaSession();
    MediaPipelineTestMethods::destroyMediaPipeline();
}

void MediaPipelineTestMethods::shouldPlayWithFailure()
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, play(_, _, _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::failureReturn)));
}

void MediaPipelineTestMethods::shouldPauseWithFailure()
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, pause(_, _, _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::failureReturn)));
}

void MediaPipelineTestMethods::shouldStopWithFailure()
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, stop(_, _, _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::failureReturn)));
}

void MediaPipelineTestMethods::shouldNotifyPlaybackStateFailure()
{
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyPlaybackState(PlaybackState::FAILURE))
        .WillOnce(Invoke(this, &MediaPipelineTestMethods::notifyEvent));
}

void MediaPipelineTestMethods::playFailure()
{
    EXPECT_EQ(m_mediaPipeline->play(), false);
}

void MediaPipelineTestMethods::pauseFailure()
{
    EXPECT_EQ(m_mediaPipeline->pause(), false);
}

void MediaPipelineTestMethods::stopFailure()
{
    EXPECT_EQ(m_mediaPipeline->stop(), false);
}

void MediaPipelineTestMethods::sendNotifyPlaybackStateFailure()
{
    getServerStub()->notifyPlaybackStateChangeEvent(kSessionId, PlaybackState::FAILURE);
    waitEvent();
}

void MediaPipelineTestMethods::shouldSetPlaybackRate2x()
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, setPlaybackRate(_, setPlaybackRateRequestMatcher(kSessionId, 2.0), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn)));
}

void MediaPipelineTestMethods::shouldSetPlaybackRateNegative2x()
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, setPlaybackRate(_, setPlaybackRateRequestMatcher(kSessionId, -2.0), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn)));
}

void MediaPipelineTestMethods::shouldSetPlaybackRateFailure()
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, setPlaybackRate(_, _, _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::failureReturn)));
}

void MediaPipelineTestMethods::setPlaybackRate2x()
{
    EXPECT_EQ(m_mediaPipeline->setPlaybackRate(2.0), true);
}

void MediaPipelineTestMethods::setPlaybackRateNegative2x()
{
    EXPECT_EQ(m_mediaPipeline->setPlaybackRate(-2.0), true);
}

void MediaPipelineTestMethods::setPlaybackRateFailure()
{
    EXPECT_EQ(m_mediaPipeline->setPlaybackRate(0.0), false);
}

void MediaPipelineTestMethods::shouldNotifyPlaybackStateSeeking()
{
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyPlaybackState(PlaybackState::SEEKING))
        .WillOnce(Invoke(this, &MediaPipelineTestMethods::notifyEvent));
}

void MediaPipelineTestMethods::shouldNotifyPlaybackStateFlushed()
{
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyPlaybackState(PlaybackState::FLUSHED))
        .WillOnce(Invoke(this, &MediaPipelineTestMethods::notifyEvent));
}

void MediaPipelineTestMethods::shouldSetPositionTo10()
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, setPosition(_, setPositionRequestMatcher(kSessionId, 10), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn)));
}

void MediaPipelineTestMethods::setPosition10()
{
    EXPECT_EQ(m_mediaPipeline->setPosition(10), true);
}

void MediaPipelineTestMethods::sendNotifyPlaybackStateSeeking()
{
    getServerStub()->notifyPlaybackStateChangeEvent(kSessionId, PlaybackState::SEEKING);
    waitEvent();
}

void MediaPipelineTestMethods::sendNotifyPlaybackStateFlushed()
{
    getServerStub()->notifyPlaybackStateChangeEvent(kSessionId, PlaybackState::FLUSHED);
    waitEvent();
}

void MediaPipelineTestMethods::writeAudioFrames()
{
    MediaPipelineTestMethods::shouldNotifyNeedDataAudioBeforePreroll();
    MediaPipelineTestMethods::sendNotifyNeedDataAudioBeforePreroll();
    uint32_t segmentId = MediaPipelineTestMethods::addSegmentMseAudio();
    MediaPipelineTestMethods::checkMseAudioSegmentWritten(segmentId);
    segmentId = MediaPipelineTestMethods::addSegmentMseAudio();
    MediaPipelineTestMethods::checkMseAudioSegmentWritten(segmentId);
    segmentId = MediaPipelineTestMethods::addSegmentMseAudio();
    MediaPipelineTestMethods::checkMseAudioSegmentWritten(segmentId);
    MediaPipelineTestMethods::shouldHaveDataBeforePreroll();
    MediaPipelineTestMethods::haveDataOk();
}

void MediaPipelineTestMethods::writeVideoFrames()
{
    MediaPipelineTestMethods::shouldNotifyNeedDataVideoBeforePreroll();
    MediaPipelineTestMethods::sendNotifyNeedDataVideoBeforePreroll();
    uint32_t segmentId = MediaPipelineTestMethods::addSegmentMseVideo();
    MediaPipelineTestMethods::checkMseVideoSegmentWritten(segmentId);
    segmentId = MediaPipelineTestMethods::addSegmentMseVideo();
    MediaPipelineTestMethods::checkMseVideoSegmentWritten(segmentId);
    segmentId = MediaPipelineTestMethods::addSegmentMseVideo();
    MediaPipelineTestMethods::checkMseVideoSegmentWritten(segmentId);
    MediaPipelineTestMethods::shouldHaveDataBeforePreroll();
    MediaPipelineTestMethods::haveDataOk();
}

void MediaPipelineTestMethods::shouldSetPositionTo0()
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, setPosition(_, setPositionRequestMatcher(kSessionId, 0), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn)));
}

void MediaPipelineTestMethods::setPosition0()
{
    EXPECT_EQ(m_mediaPipeline->setPosition(0), true);
}

void MediaPipelineTestMethods::writeAudioEos()
{
    MediaPipelineTestMethods::shouldNotifyNeedDataAudioAfterPreroll();
    MediaPipelineTestMethods::sendNotifyNeedDataAudioAfterPreroll();
    MediaPipelineTestMethods::shouldHaveDataEos(0);
    MediaPipelineTestMethods::haveDataEos();
}

void MediaPipelineTestMethods::writeVideoEos()
{
    MediaPipelineTestMethods::shouldNotifyNeedDataVideoAfterPreroll();
    MediaPipelineTestMethods::sendNotifyNeedDataVideoAfterPreroll();
    MediaPipelineTestMethods::shouldHaveDataEos(0);
    MediaPipelineTestMethods::haveDataEos();
}

void MediaPipelineTestMethods::setPositionFailure()
{
    EXPECT_EQ(m_mediaPipeline->setPosition(0), false);
}

void MediaPipelineTestMethods::addSegmentFailure()
{
    EXPECT_LT(m_audioSegmentCount, sizeof(kAudioSegments) / sizeof(kAudioSegments[0]));

    std::unique_ptr<IMediaPipeline::MediaSegment> mseData =
        std::make_unique<IMediaPipeline::MediaSegmentAudio>(kAudioSourceId, getTimestamp(m_audioSegmentCount),
                                                            kDuration, kSampleRate, kNumberOfChannels);
    mseData->setData(kAudioSegments[m_audioSegmentCount].size(),
                     (const uint8_t *)kAudioSegments[m_audioSegmentCount].c_str());
    EXPECT_EQ(m_mediaPipeline->addSegment(m_needDataRequestId, mseData), AddSegmentStatus::ERROR);
}

void MediaPipelineTestMethods::removeSourceVideoSecondary()
{
    EXPECT_EQ(m_mediaPipelineSecondary->removeSource(kVideoSourceId), true);
}

void MediaPipelineTestMethods::stopSecondary()
{
    EXPECT_EQ(m_mediaPipelineSecondary->stop(), true);
}

void MediaPipelineTestMethods::shouldNotifyPlaybackStateStoppedSecondary()
{
    EXPECT_CALL(*m_mediaPipelineClientSecondaryMock, notifyPlaybackState(PlaybackState::STOPPED))
        .WillOnce(Invoke(this, &MediaPipelineTestMethods::notifyEvent));
}

void MediaPipelineTestMethods::sendNotifyPlaybackStateStoppedSecondary()
{
    getServerStub()->notifyPlaybackStateChangeEvent(kSessionIdSecondary, PlaybackState::STOPPED);
    waitEvent();
}

void MediaPipelineTestMethods::destroyMediaPipelineSecondary()
{
    m_mediaPipelineSecondary.reset();
}

void MediaPipelineTestMethods::shouldNotifyNetworkStateBufferingSecondary()
{
    EXPECT_CALL(*m_mediaPipelineClientSecondaryMock, notifyNetworkState(NetworkState::BUFFERING))
        .WillOnce(Invoke(this, &MediaPipelineTestMethods::notifyEvent));
}

void MediaPipelineTestMethods::sendNotifyNetworkStateBufferingSecondary()
{
    getServerStub()->notifyNetworkStateChangeEvent(kSessionIdSecondary, NetworkState::BUFFERING);
    waitEvent();
}

void MediaPipelineTestMethods::attachSourceVideoSecondary()
{
    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceVideo>(kMimeType.c_str(), kHasNoDrm, kWidthUhd, kHeightUhd,
                                                           kAlignment, kStreamFormat, kCodecData); //TODO: Different?
    EXPECT_EQ(m_mediaPipelineSecondary->attachSource(mediaSource), true);
}

void MediaPipelineTestMethods::allSourcesAttachedSecondary()
{
    EXPECT_EQ(m_mediaPipelineSecondary->allSourcesAttached(), true);
}

void MediaPipelineTestMethods::shouldNotifyPlaybackStateIdleSecondary()
{
    EXPECT_CALL(*m_mediaPipelineClientSecondaryMock, notifyPlaybackState(PlaybackState::IDLE))
        .WillOnce(Invoke(this, &MediaPipelineTestMethods::notifyEvent));
}

void MediaPipelineTestMethods::sendNotifyPlaybackStateIdleSecondary()
{
    getServerStub()->notifyPlaybackStateChangeEvent(kSessionIdSecondary, PlaybackState::IDLE);
    waitEvent();
}

void MediaPipelineTestMethods::writeVideoFramesSecondary()
{
    uint32_t framesToWrite = 3;
    MediaPipelineTestMethods::shouldNotifyNeedDataVideoSecondary(framesToWrite);
    MediaPipelineTestMethods::sendNotifyNeedDataVideoSecondary(framesToWrite);
    int32_t segmentId = MediaPipelineTestMethods::addSegmentMseVideoSecondary();
    MediaPipelineTestMethods::checkMseVideoSegmentWritten(segmentId);
    segmentId = MediaPipelineTestMethods::addSegmentMseVideoSecondary();
    MediaPipelineTestMethods::checkMseVideoSegmentWritten(segmentId);
    segmentId = MediaPipelineTestMethods::addSegmentMseVideoSecondary();
    MediaPipelineTestMethods::checkMseVideoSegmentWritten(segmentId);
    MediaPipelineTestMethods::shouldHaveDataOkSecondary(framesToWrite);
    MediaPipelineTestMethods::haveDataOkSecondary();
}

void MediaPipelineTestMethods::shouldNotifyNeedDataVideoSecondary(uint32_t framesToWrite)
{
    EXPECT_CALL(*m_mediaPipelineClientSecondaryMock,
                notifyNeedMediaData(kVideoSourceId, framesToWrite, m_needDataRequestId, kNullShmInfo))
        .WillOnce(InvokeWithoutArgs(
            [&]()
            {
                // Set the firstSegment flag
                m_firstSegmentOfNeedData = true;
                notifyEvent();
            }));
}

void MediaPipelineTestMethods::sendNotifyNeedDataVideoSecondary(uint32_t framesToWrite)
{
    getServerStub()->notifyNeedMediaDataEvent(kSessionIdSecondary, kVideoSourceId, framesToWrite, m_needDataRequestId,
                                              m_locationToWriteVideo[kSecondaryPartition]);
    waitEvent();
}

int32_t MediaPipelineTestMethods::addSegmentMseVideoSecondary()
{
    EXPECT_LT(m_videoSegmentCount, sizeof(kVideoSegments) / sizeof(kVideoSegments[0]));

    std::unique_ptr<IMediaPipeline::MediaSegment> mseData =
        std::make_unique<IMediaPipeline::MediaSegmentVideo>(kVideoSourceId, getTimestamp(m_videoSegmentCount),
                                                            kDuration, kWidthUhd, kHeightUhd, kFrameRate); //TODO: Different?
    mseData->setData(kVideoSegments[m_videoSegmentCount].size(),
                     (const uint8_t *)kVideoSegments[m_videoSegmentCount].c_str());
    EXPECT_EQ(m_mediaPipelineSecondary->addSegment(m_needDataRequestId, mseData), AddSegmentStatus::OK);

    // Store where the segment should be written so we can check the data
    int32_t segmentId = m_videoSegmentCount;
    writtenVideoSegments.insert({segmentId, *m_locationToWriteVideo[kSecondaryPartition]});

    incrementWriteLocation(kVideoSegments[m_videoSegmentCount].size(), m_locationToWriteVideo[kSecondaryPartition]);

    m_videoSegmentCount++;
    return segmentId;
}

void MediaPipelineTestMethods::haveDataOkSecondary()
{
    EXPECT_EQ(m_mediaPipelineSecondary->haveData(MediaSourceStatus::OK, m_needDataRequestId), true);
}

void MediaPipelineTestMethods::shouldNotifyNetworkStateBufferedSecondary()
{
    EXPECT_CALL(*m_mediaPipelineClientSecondaryMock, notifyNetworkState(NetworkState::BUFFERED))
        .WillOnce(Invoke(this, &MediaPipelineTestMethods::notifyEvent));
}

void MediaPipelineTestMethods::sendNotifyNetworkStateBufferedSecondary()
{
    getServerStub()->notifyNetworkStateChangeEvent(kSessionIdSecondary, NetworkState::BUFFERED);
    waitEvent();
}

void MediaPipelineTestMethods::shouldNotifyPlaybackStatePausedSecondary()
{
    EXPECT_CALL(*m_mediaPipelineClientSecondaryMock, notifyPlaybackState(PlaybackState::PAUSED))
        .WillOnce(Invoke(this, &MediaPipelineTestMethods::notifyEvent));
}

void MediaPipelineTestMethods::sendNotifyPlaybackStatePausedSecondary()
{
    getServerStub()->notifyPlaybackStateChangeEvent(kSessionIdSecondary, PlaybackState::PAUSED);
    waitEvent();
}

void MediaPipelineTestMethods::playSecondary()
{
    EXPECT_EQ(m_mediaPipelineSecondary->play(), true);

}

void MediaPipelineTestMethods::shouldNotifyPlaybackStatePlayingSecondary()
{
    EXPECT_CALL(*m_mediaPipelineClientSecondaryMock, notifyPlaybackState(PlaybackState::PLAYING))
        .WillOnce(Invoke(this, &MediaPipelineTestMethods::notifyEvent));
}

void MediaPipelineTestMethods::sendNotifyPlaybackStatePlayingSecondary()
{
    getServerStub()->notifyPlaybackStateChangeEvent(kSessionIdSecondary, PlaybackState::PLAYING);
    waitEvent();
}

} // namespace firebolt::rialto::client::ct
