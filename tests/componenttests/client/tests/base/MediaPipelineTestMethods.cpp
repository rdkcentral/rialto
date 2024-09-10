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
#include "CommonConstants.h"
#include "MediaPipelineProtoRequestMatchers.h"
#include "MediaPipelineProtoUtils.h"
#include "MediaPipelineStructureMatchers.h"
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
constexpr MediaType kMediaType{MediaType::MSE};
const std::string kEmptyMimeType{""};
const std::string kVideoH264{"video/h264"};
const std::string kVideoVp9{"video/x-vp9"};
const std::string kAudioMp4{"audio/mp4"};
const std::string kAudioMpeg{"audio/mpeg"};
const std::string kAudioEacs{"audio/x-eac3"};
const std::string kUrl{"mse://1"};
constexpr int32_t kAudioSourceId{1};
constexpr int32_t kVideoSourceId{2};
constexpr firebolt::rialto::SegmentAlignment kAlignment{firebolt::rialto::SegmentAlignment::UNDEFINED};
constexpr firebolt::rialto::StreamFormat kStreamFormatRaw{firebolt::rialto::StreamFormat::RAW};
constexpr firebolt::rialto::StreamFormat kStreamFormatByteStream{firebolt::rialto::StreamFormat::BYTE_STREAM};
constexpr int32_t kWidthUhd{3840};
constexpr int32_t kHeightUhd{2160};
constexpr int32_t kWidth720p{1280};
constexpr int32_t kHeight720p{720};
constexpr int32_t kWidthSecondary{426};
constexpr int32_t kHeightSecondary{240};
constexpr uint32_t kNumberOfChannels{6};
constexpr uint32_t kSampleRate{48000};
constexpr bool kHasNoDrm{false};
const std::string kCodecSpecificConfigStr{"1243567"};
const std::shared_ptr<firebolt::rialto::CodecData> kCodecData{std::make_shared<firebolt::rialto::CodecData>()};
constexpr size_t kFrameCountBeforePreroll{3};
constexpr size_t kMaxFrameCount{20};
const std::shared_ptr<MediaPlayerShmInfo> kNullShmInfo;
constexpr int64_t kTimeStamp{1701352637000};
constexpr int64_t kDuration{105};
constexpr int64_t kDurationSecondary{402};
constexpr firebolt::rialto::Fraction kFrameRateEmpty{0, 0};
constexpr firebolt::rialto::Fraction kFrameRateSecondary{15, 1};
constexpr VideoRequirements kVideoRequirementsSecondary{426, 240};
constexpr int32_t kSessionIdSecondary{11};
constexpr uint32_t kPrimaryPartition{0};
constexpr uint32_t kSecondaryPartition{1};
constexpr uint32_t kNumberOfChannelsMpeg{2};
constexpr uint32_t kSampleRateMpeg{26000};
constexpr uint32_t kNumberOfChannelsEacs{1};
constexpr uint32_t kSampleRateEacs{1000};
constexpr uint32_t kInitWithLast15{6700};
const std::vector<SubSamplePair> kSubSamplePairs{{42, 66}, {6654, 4}, {3, 3654}};
constexpr firebolt::rialto::CipherMode kCipherModeCenc{firebolt::rialto::CipherMode::CENC};
constexpr uint32_t kCrypt{7};
constexpr uint32_t kSkip{8};
constexpr firebolt::rialto::QosInfo kQosInfoAudio{766, 6};
constexpr firebolt::rialto::QosInfo kQosInfoVideo{98, 2};
const std::vector<std::string> kAudioMimeType{"audio/mp4", "audio/aac", "audio/x-eac3", "audio/x-opus"};
const std::vector<std::string> kVideoMimeType{"video/h264", "video/h265", "video/x-av1", "video/x-vp9", "video/mp4"};
const std::vector<std::string> kUnknownMimeType{};
constexpr bool kResetTime{true};
constexpr double kPosition{1234};
constexpr int64_t kDiscontinuityGap{1};
constexpr bool kIsAudioAac{false};
const std::vector<std::string> kSupportedProperties{"immediate-output", "testProp2"};
} // namespace

namespace firebolt::rialto::client::ct
{
MediaPipelineTestMethods::MediaPipelineTestMethods(const std::vector<firebolt::rialto::MediaPlayerShmInfo> &audioShmInfo,
                                                   const std::vector<firebolt::rialto::MediaPlayerShmInfo> &videoShmInfo)
    : m_mediaPipelineClientMock{std::make_shared<StrictMock<MediaPipelineClientMock>>()},
      m_mediaPipelineModuleMock{std::make_shared<StrictMock<MediaPipelineModuleMock>>()},
      m_mediaPipelineClientSecondaryMock{std::make_shared<StrictMock<MediaPipelineClientMock>>()},
      m_mediaPipelineCapabilitiesModuleMock{std::make_shared<StrictMock<MediaPipelineCapabilitiesModuleMock>>()},
      m_kAudioShmInfo{audioShmInfo}, m_kVideoShmInfo{videoShmInfo}
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

void MediaPipelineTestMethods::shouldCreateMediaSession()
{
    shouldCreateMediaSessionInternal(kSessionId, kVideoRequirements);
}

void MediaPipelineTestMethods::shouldCreateMediaSessionFailure()
{
    EXPECT_CALL(*m_mediaPipelineModuleMock,
                createSession(_, createSessionRequestMatcher(kVideoRequirements.maxWidth, kVideoRequirements.maxHeight),
                              _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::failureReturn)));
}

void MediaPipelineTestMethods::shouldCreateMediaSessionSecondary()
{
    shouldCreateMediaSessionInternal(kSessionIdSecondary, kVideoRequirementsSecondary);
}

void MediaPipelineTestMethods::createMediaPipeline()
{
    createMediaPipelineInternal(m_mediaPipeline, m_mediaPipelineClientMock, kVideoRequirements);
}

void MediaPipelineTestMethods::createMediaPipelineFailure()
{
    m_mediaPipelineFactory = firebolt::rialto::IMediaPipelineFactory::createFactory();
    EXPECT_NO_THROW(
        m_mediaPipeline = m_mediaPipelineFactory->createMediaPipeline(m_mediaPipelineClientMock, kVideoRequirements));
    EXPECT_EQ(m_mediaPipeline, nullptr);
}

void MediaPipelineTestMethods::createMediaPipelineSecondary()
{
    createMediaPipelineInternal(m_mediaPipelineSecondary, m_mediaPipelineClientSecondaryMock,
                                kVideoRequirementsSecondary);
}

void MediaPipelineTestMethods::shouldLoad()
{
    shouldLoadInternal(kSessionId, kMediaType, kEmptyMimeType, kUrl);
}

void MediaPipelineTestMethods::shouldLoadSecondary()
{
    shouldLoadInternal(kSessionIdSecondary, kMediaType, kEmptyMimeType, kUrl);
}

void MediaPipelineTestMethods::load()
{
    loadInternal(m_mediaPipeline, kMediaType, kEmptyMimeType, kUrl, true);
}

void MediaPipelineTestMethods::loadSecondary()
{
    loadInternal(m_mediaPipelineSecondary, kMediaType, kEmptyMimeType, kUrl, true);
}

void MediaPipelineTestMethods::shouldNotifyNetworkStateBuffering()
{
    shouldNotifyNetworkStateInternal(m_mediaPipelineClientMock, NetworkState::BUFFERING);
}

void MediaPipelineTestMethods::shouldNotifyNetworkStateBufferingSecondary()
{
    shouldNotifyNetworkStateInternal(m_mediaPipelineClientSecondaryMock, NetworkState::BUFFERING);
}

void MediaPipelineTestMethods::sendNotifyNetworkStateBuffering()
{
    sendNotifyNetworkStateInternal(kSessionId, NetworkState::BUFFERING);
}

void MediaPipelineTestMethods::sendNotifyNetworkStateBufferingSecondary()
{
    sendNotifyNetworkStateInternal(kSessionIdSecondary, NetworkState::BUFFERING);
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
    shouldAttachVideoSourceInternal(kSessionId, kVideoVp9, kHasNoDrm, kWidth720p, kHeight720p, kAlignment, kCodecData,
                                    kStreamFormatRaw);
}

void MediaPipelineTestMethods::shouldAttachVideoSourceSecondary()
{
    shouldAttachVideoSourceInternal(kSessionIdSecondary, kVideoH264, kHasNoDrm, kWidthSecondary, kHeightSecondary,
                                    kAlignment, kCodecData, kStreamFormatByteStream);
}

void MediaPipelineTestMethods::attachSourceVideo()
{
    attachSourceVideoInternal(m_mediaPipeline, kVideoVp9, kHasNoDrm, kWidth720p, kHeight720p, kAlignment, kCodecData,
                              kStreamFormatRaw, true);
}

void MediaPipelineTestMethods::attachSourceVideoSecondary()
{
    attachSourceVideoInternal(m_mediaPipelineSecondary, kVideoH264, kHasNoDrm, kWidthSecondary, kHeightSecondary,
                              kAlignment, kCodecData, kStreamFormatByteStream, true);
}

void MediaPipelineTestMethods::shouldAttachAudioSource()
{
    shouldAttachAudioSourceInternal(kAudioMp4, kHasNoDrm, kAlignment, kNumberOfChannels, kSampleRate,
                                    kCodecSpecificConfigStr, kCodecData, kStreamFormatRaw);
}

void MediaPipelineTestMethods::shouldAttachAudioSourceMpeg()
{
    shouldAttachAudioSourceInternal(kAudioMpeg, kHasNoDrm, kAlignment, kNumberOfChannelsMpeg, kSampleRateMpeg,
                                    kCodecSpecificConfigStr, kCodecData, kStreamFormatRaw);
}

void MediaPipelineTestMethods::shouldAttachAudioSourceMp4()
{
    shouldAttachAudioSource();
}

void MediaPipelineTestMethods::shouldAttachAudioSourceEacs()
{
    shouldAttachAudioSourceInternal(kAudioEacs, kHasNoDrm, kAlignment, kNumberOfChannelsEacs, kSampleRateEacs,
                                    kCodecSpecificConfigStr, kCodecData, kStreamFormatRaw);
}

void MediaPipelineTestMethods::attachSourceAudio()
{
    attachSourceAudioInternal(kAudioMp4, kHasNoDrm, kAlignment, kNumberOfChannels, kSampleRate, kCodecSpecificConfigStr,
                              kCodecData, kStreamFormatRaw, true);
}

void MediaPipelineTestMethods::attachSourceAudioMpeg()
{
    attachSourceAudioInternal(kAudioMpeg, kHasNoDrm, kAlignment, kNumberOfChannelsMpeg, kSampleRateMpeg,
                              kCodecSpecificConfigStr, kCodecData, kStreamFormatRaw, true);
}

void MediaPipelineTestMethods::attachSourceAudioMp4()
{
    attachSourceAudio();
}

void MediaPipelineTestMethods::attachSourceAudioEacs()
{
    attachSourceAudioInternal(kAudioEacs, kHasNoDrm, kAlignment, kNumberOfChannelsEacs, kSampleRateEacs,
                              kCodecSpecificConfigStr, kCodecData, kStreamFormatRaw, true);
}

void MediaPipelineTestMethods::shouldAllSourcesAttached()
{
    shouldAllSourcesAttachedInternal(kSessionId);
}

void MediaPipelineTestMethods::shouldAllSourcesAttachedSecondary()
{
    shouldAllSourcesAttachedInternal(kSessionIdSecondary);
}

void MediaPipelineTestMethods::allSourcesAttached()
{
    allSourcesAttachedInternal(m_mediaPipeline, true);
}

void MediaPipelineTestMethods::allSourcesAttachedSecondary()
{
    allSourcesAttachedInternal(m_mediaPipelineSecondary, true);
}

void MediaPipelineTestMethods::shouldNotifyPlaybackStateIdle()
{
    shouldNotifyPlaybackStateInternal(m_mediaPipelineClientMock, PlaybackState::IDLE);
}

void MediaPipelineTestMethods::shouldNotifyPlaybackStateIdleSecondary()
{
    shouldNotifyPlaybackStateInternal(m_mediaPipelineClientSecondaryMock, PlaybackState::IDLE);
}

void MediaPipelineTestMethods::sendNotifyPlaybackStateIdle()
{
    sendNotifyPlaybackStateInternal(kSessionId, PlaybackState::IDLE);
}

void MediaPipelineTestMethods::sendNotifyPlaybackStateIdleSecondary()
{
    sendNotifyPlaybackStateInternal(kSessionIdSecondary, PlaybackState::IDLE);
}

void MediaPipelineTestMethods::shouldNotifyNeedDataAudioBeforePreroll()
{
    shouldNotifyNeedDataInternal(m_mediaPipelineClientMock, kAudioSourceId, kFrameCountBeforePreroll);
}

void MediaPipelineTestMethods::sendNotifyNeedDataAudioBeforePreroll()
{
    sendNotifyNeedDataInternal(kSessionId, kAudioSourceId, m_locationToWriteAudio[kPrimaryPartition],
                               kFrameCountBeforePreroll);
}

void MediaPipelineTestMethods::shouldNotifyNeedDataVideoBeforePreroll()
{
    shouldNotifyNeedDataInternal(m_mediaPipelineClientMock, kVideoSourceId, kFrameCountBeforePreroll);
}

void MediaPipelineTestMethods::sendNotifyNeedDataVideoBeforePreroll()
{
    sendNotifyNeedDataInternal(kSessionId, kVideoSourceId, m_locationToWriteVideo[kPrimaryPartition],
                               kFrameCountBeforePreroll);
}

void MediaPipelineTestMethods::haveDataOk()
{
    haveDataInternal(m_mediaPipeline, MediaSourceStatus::OK, true);
}

void MediaPipelineTestMethods::haveDataEos()
{
    haveDataInternal(m_mediaPipeline, MediaSourceStatus::EOS, true);
}

void MediaPipelineTestMethods::haveDataNoAvailableSamples()
{
    haveDataInternal(m_mediaPipeline, MediaSourceStatus::NO_AVAILABLE_SAMPLES, true);
}

void MediaPipelineTestMethods::haveDataError()
{
    haveDataInternal(m_mediaPipeline, MediaSourceStatus::ERROR, true);
}

void MediaPipelineTestMethods::haveDataOkSecondary()
{
    haveDataInternal(m_mediaPipelineSecondary, MediaSourceStatus::OK, true);
}

void MediaPipelineTestMethods::haveDataFailure()
{
    haveDataInternal(m_mediaPipeline, MediaSourceStatus::OK, false);
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
    return addSegmentMseVideoInternal(m_mediaPipeline, kDuration, kWidth720p, kHeight720p, kFrameRateEmpty,
                                      kPrimaryPartition, AddSegmentStatus::OK);
}

int32_t MediaPipelineTestMethods::addSegmentMseVideoSecondary()
{
    return addSegmentMseVideoInternal(m_mediaPipelineSecondary, kDurationSecondary, kWidthSecondary, kHeightSecondary,
                                      kFrameRateSecondary, kSecondaryPartition, AddSegmentStatus::OK);
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

void MediaPipelineTestMethods::checkMseVideoSegmentWrittenSecondary(int32_t segmentId)
{
    auto it = writtenVideoSegments.find(segmentId);
    ASSERT_NE(it, writtenVideoSegments.end());

    uint8_t *dataPosition{reinterpret_cast<uint8_t *>(getShmAddress()) + it->second.mediaDataOffset};
    std::uint32_t *metadataSize{reinterpret_cast<uint32_t *>(dataPosition)};
    dataPosition += sizeof(uint32_t);
    MediaSegmentMetadata metadata;
    ASSERT_TRUE(metadata.ParseFromArray(dataPosition, *metadataSize));

    checkVideoMetadataSecondary(metadata, segmentId);
    checkHasNoAudioMetadata(metadata);
    checkHasNoEncryptionMetadata(metadata);
    checkHasNoCodacData(metadata);
    checkHasNoSegmentAlignment(metadata);
    checkHasNoExtraData(metadata);
    checkSegmentData(metadata, dataPosition += *metadataSize, kVideoSegments[segmentId]);
}

int32_t MediaPipelineTestMethods::addSegmentEncryptedAudio(int32_t keyIndex)
{
    EXPECT_LT(m_audioSegmentCount, sizeof(kAudioSegments) / sizeof(kAudioSegments[0]));

    std::unique_ptr<IMediaPipeline::MediaSegment> mseData =
        std::make_unique<IMediaPipeline::MediaSegmentAudio>(kAudioSourceId, getTimestamp(m_audioSegmentCount),
                                                            kDuration, kSampleRate, kNumberOfChannels);
    mseData->setData(kAudioSegments[m_audioSegmentCount].size(),
                     (const uint8_t *)kAudioSegments[m_audioSegmentCount].c_str());
    addEncryptedDataToSegment(mseData, keyIndex);
    EXPECT_EQ(m_mediaPipeline->addSegment(m_needDataRequestId, mseData), AddSegmentStatus::OK);

    // Store where the segment should be written so we can check the data
    int32_t segmentId = m_audioSegmentCount;
    writtenAudioSegments.insert({segmentId, *m_locationToWriteAudio[kPrimaryPartition]});

    incrementWriteLocation(kAudioSegments[segmentId].size(), m_locationToWriteAudio[kPrimaryPartition]);

    m_audioSegmentCount++;
    return segmentId;
}

void MediaPipelineTestMethods::checkEncryptedAudioSegmentWritten(int32_t segmentId, uint32_t keyIndex)
{
    auto it = writtenAudioSegments.find(segmentId);
    ASSERT_NE(it, writtenAudioSegments.end());

    MediaSegmentMetadata metadata;
    uint8_t *segmentPosition = parseMetadata(metadata, it->second.mediaDataOffset);

    checkAudioMetadata(metadata, segmentId);
    checkEncryptionMetadata(metadata, keyIndex);
    checkHasNoVideoMetadata(metadata);
    checkHasNoCodacData(metadata);
    checkHasNoSegmentAlignment(metadata);
    checkHasNoExtraData(metadata);
    checkSegmentData(metadata, segmentPosition, kAudioSegments[segmentId]);
}

int32_t MediaPipelineTestMethods::addSegmentEncryptedVideo(int32_t keyIndex)
{
    EXPECT_LT(m_videoSegmentCount, sizeof(kVideoSegments) / sizeof(kVideoSegments[0]));

    std::unique_ptr<IMediaPipeline::MediaSegment> mseData =
        std::make_unique<IMediaPipeline::MediaSegmentVideo>(kVideoSourceId, getTimestamp(m_videoSegmentCount),
                                                            kDuration, kWidth720p, kHeight720p, kFrameRateEmpty);
    mseData->setData(kVideoSegments[m_videoSegmentCount].size(),
                     (const uint8_t *)kVideoSegments[m_videoSegmentCount].c_str());
    addEncryptedDataToSegment(mseData, keyIndex);
    EXPECT_EQ(m_mediaPipeline->addSegment(m_needDataRequestId, mseData), AddSegmentStatus::OK);

    // Store where the segment should be written so we can check the data
    int32_t segmentId = m_videoSegmentCount;
    writtenVideoSegments.insert({segmentId, *m_locationToWriteVideo[kPrimaryPartition]});

    incrementWriteLocation(kVideoSegments[m_videoSegmentCount].size(), m_locationToWriteVideo[kPrimaryPartition]);

    m_videoSegmentCount++;
    return segmentId;
}

void MediaPipelineTestMethods::addSegmentMseVideoNoSpace()
{
    std::unique_ptr<IMediaPipeline::MediaSegment> mseData =
        std::make_unique<IMediaPipeline::MediaSegmentVideo>(kVideoSourceId, getTimestamp(0), kDuration, kWidth720p,
                                                            kHeight720p, kFrameRateEmpty);

    // Set data as the shared memory buffer as we know this is too large
    mseData->setData(getShmSize() + 1, reinterpret_cast<uint8_t *>(getShmAddress()));
    EXPECT_EQ(m_mediaPipeline->addSegment(m_needDataRequestId, mseData), AddSegmentStatus::NO_SPACE);
}

void MediaPipelineTestMethods::checkEncryptedVideoSegmentWritten(int32_t segmentId, uint32_t keyIndex)
{
    auto it = writtenVideoSegments.find(segmentId);
    ASSERT_NE(it, writtenVideoSegments.end());

    MediaSegmentMetadata metadata;
    uint8_t *segmentPosition = parseMetadata(metadata, it->second.mediaDataOffset);

    checkVideoMetadata(metadata, segmentId);
    checkEncryptionMetadata(metadata, keyIndex);
    checkHasNoAudioMetadata(metadata);
    checkHasNoCodacData(metadata);
    checkHasNoSegmentAlignment(metadata);
    checkHasNoExtraData(metadata);
    checkSegmentData(metadata, segmentPosition, kVideoSegments[segmentId]);
}

void MediaPipelineTestMethods::checkMediaPipelineClient()
{
    EXPECT_EQ(m_mediaPipeline->getClient().lock(), m_mediaPipelineClientMock);
}

void MediaPipelineTestMethods::checkAudioKeyId(int32_t segmentId, uint32_t keyIndex)
{
    auto it = writtenAudioSegments.find(segmentId);
    ASSERT_NE(it, writtenAudioSegments.end());

    checkKeyId(it->second, keyIndex);
}

void MediaPipelineTestMethods::checkVideoKeyId(int32_t segmentId, uint32_t keyIndex)
{
    auto it = writtenVideoSegments.find(segmentId);
    ASSERT_NE(it, writtenVideoSegments.end());

    checkKeyId(it->second, keyIndex);
}

void MediaPipelineTestMethods::shouldNotifyNetworkStateBuffered()
{
    shouldNotifyNetworkStateInternal(m_mediaPipelineClientMock, NetworkState::BUFFERED);
}

void MediaPipelineTestMethods::shouldNotifyNetworkStateBufferedSecondary()
{
    shouldNotifyNetworkStateInternal(m_mediaPipelineClientSecondaryMock, NetworkState::BUFFERED);
}

void MediaPipelineTestMethods::sendNotifyNetworkStateBuffered()
{
    sendNotifyNetworkStateInternal(kSessionId, NetworkState::BUFFERED);
}

void MediaPipelineTestMethods::sendNotifyNetworkStateBufferedSecondary()
{
    sendNotifyNetworkStateInternal(kSessionIdSecondary, NetworkState::BUFFERED);
}

void MediaPipelineTestMethods::shouldNotifyPlaybackStatePaused()
{
    shouldNotifyPlaybackStateInternal(m_mediaPipelineClientMock, PlaybackState::PAUSED);
}

void MediaPipelineTestMethods::shouldNotifyPlaybackStatePausedSecondary()
{
    shouldNotifyPlaybackStateInternal(m_mediaPipelineClientSecondaryMock, PlaybackState::PAUSED);
}

void MediaPipelineTestMethods::sendNotifyPlaybackStatePaused()
{
    sendNotifyPlaybackStateInternal(kSessionId, PlaybackState::PAUSED);
}

void MediaPipelineTestMethods::sendNotifyPlaybackStatePausedSecondary()
{
    sendNotifyPlaybackStateInternal(kSessionIdSecondary, PlaybackState::PAUSED);
}

void MediaPipelineTestMethods::shouldNotifyPlaybackStatePlaying()
{
    shouldNotifyPlaybackStateInternal(m_mediaPipelineClientMock, PlaybackState::PLAYING);
}
void MediaPipelineTestMethods::shouldNotifyPlaybackStatePlayingSecondary()
{
    shouldNotifyPlaybackStateInternal(m_mediaPipelineClientSecondaryMock, PlaybackState::PLAYING);
}

void MediaPipelineTestMethods::sendNotifyPlaybackStatePlaying()
{
    sendNotifyPlaybackStateInternal(kSessionId, PlaybackState::PLAYING);
}

void MediaPipelineTestMethods::sendNotifyPlaybackStatePlayingSecondary()
{
    sendNotifyPlaybackStateInternal(kSessionIdSecondary, PlaybackState::PLAYING);
}

void MediaPipelineTestMethods::shouldPlay()
{
    shouldPlayInternal(kSessionId);
}

void MediaPipelineTestMethods::shouldPlaySecondary()
{
    shouldPlayInternal(kSessionIdSecondary);
}

void MediaPipelineTestMethods::play()
{
    playInternal(m_mediaPipeline, true);
}

void MediaPipelineTestMethods::playSecondary()
{
    playInternal(m_mediaPipelineSecondary, true);
}

void MediaPipelineTestMethods::shouldNotifyNeedDataAudioAfterPreroll()
{
    shouldNotifyNeedDataInternal(m_mediaPipelineClientMock, kAudioSourceId, kMaxFrameCount);
}

void MediaPipelineTestMethods::sendNotifyNeedDataAudioAfterPreroll()
{
    sendNotifyNeedDataInternal(kSessionId, kAudioSourceId, m_locationToWriteAudio[kPrimaryPartition], kMaxFrameCount);
}

void MediaPipelineTestMethods::shouldNotifyNeedDataVideoAfterPreroll()
{
    shouldNotifyNeedDataInternal(m_mediaPipelineClientMock, kVideoSourceId, kMaxFrameCount);
}

void MediaPipelineTestMethods::sendNotifyNeedDataVideoAfterPreroll()
{
    sendNotifyNeedDataInternal(kSessionId, kVideoSourceId, m_locationToWriteVideo[kPrimaryPartition], kMaxFrameCount);
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

void MediaPipelineTestMethods::shouldHaveDataEos(size_t framesWritten)
{
    shouldHaveDataInternal(kSessionId, MediaSourceStatus::EOS, framesWritten, kPrimaryPartition);
}

void MediaPipelineTestMethods::shouldHaveDataNoAvailableSamples()
{
    shouldHaveDataInternal(kSessionId, MediaSourceStatus::NO_AVAILABLE_SAMPLES, 0, kPrimaryPartition);
}

void MediaPipelineTestMethods::shouldHaveDataError()
{
    shouldHaveDataInternal(kSessionId, MediaSourceStatus::ERROR, 0, kPrimaryPartition);
}

void MediaPipelineTestMethods::shouldHaveDataOkSecondary(size_t framesWritten)
{
    shouldHaveDataInternal(kSessionIdSecondary, MediaSourceStatus::OK, framesWritten, kSecondaryPartition);
}

void MediaPipelineTestMethods::shouldHaveDataFailure(size_t framesWritten)
{
    EXPECT_CALL(*m_mediaPipelineModuleMock,
                haveData(_,
                         haveDataRequestMatcher(kSessionId, convertMediaSourceStatus(MediaSourceStatus::OK),
                                                framesWritten, m_needDataRequestId),
                         _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::failureReturn)));
}

void MediaPipelineTestMethods::shouldNotifyPlaybackStateEndOfStream()
{
    shouldNotifyPlaybackStateInternal(m_mediaPipelineClientMock, PlaybackState::END_OF_STREAM);
}

void MediaPipelineTestMethods::sendNotifyPlaybackStateEndOfStream()
{
    sendNotifyPlaybackStateInternal(kSessionId, PlaybackState::END_OF_STREAM);
}

void MediaPipelineTestMethods::shouldRemoveVideoSource()
{
    shouldRemoveSourceInternal(kSessionId, kVideoSourceId);
}

void MediaPipelineTestMethods::shouldRemoveVideoSourceSecondary()
{
    shouldRemoveSourceInternal(kSessionIdSecondary, kVideoSourceId);
}

void MediaPipelineTestMethods::removeSourceVideo()
{
    removeSourceInternal(m_mediaPipeline, kVideoSourceId, true);
}

void MediaPipelineTestMethods::shouldRemoveAudioSource()
{
    shouldRemoveSourceInternal(kSessionId, kAudioSourceId);
}

void MediaPipelineTestMethods::removeSourceAudio()
{
    removeSourceInternal(m_mediaPipeline, kAudioSourceId, true);
}

void MediaPipelineTestMethods::removeSourceVideoSecondary()
{
    removeSourceInternal(m_mediaPipelineSecondary, kVideoSourceId, true);
}

void MediaPipelineTestMethods::shouldStop()
{
    shouldStopInternal(kSessionId);
}

void MediaPipelineTestMethods::shouldStopSecondary()
{
    shouldStopInternal(kSessionIdSecondary);
}

void MediaPipelineTestMethods::stop()
{
    stopInternal(m_mediaPipeline, true);
}

void MediaPipelineTestMethods::stopSecondary()
{
    stopInternal(m_mediaPipelineSecondary, true);
}

void MediaPipelineTestMethods::shouldNotifyPlaybackStateStopped()
{
    shouldNotifyPlaybackStateInternal(m_mediaPipelineClientMock, PlaybackState::STOPPED);
}

void MediaPipelineTestMethods::shouldNotifyPlaybackStateStoppedSecondary()
{
    shouldNotifyPlaybackStateInternal(m_mediaPipelineClientSecondaryMock, PlaybackState::STOPPED);
}

void MediaPipelineTestMethods::sendNotifyPlaybackStateStopped()
{
    sendNotifyPlaybackStateInternal(kSessionId, PlaybackState::STOPPED);
}

void MediaPipelineTestMethods::sendNotifyPlaybackStateStoppedSecondary()
{
    sendNotifyPlaybackStateInternal(kSessionIdSecondary, PlaybackState::STOPPED);
}

void MediaPipelineTestMethods::shouldDestroyMediaSession()
{
    shouldDestroyMediaSessionInternal(kSessionId);
}

void MediaPipelineTestMethods::shouldDestroyMediaSessionSecondary()
{
    shouldDestroyMediaSessionInternal(kSessionIdSecondary);
}

void MediaPipelineTestMethods::destroyMediaPipeline()
{
    m_mediaPipeline.reset();
}

void MediaPipelineTestMethods::destroyMediaPipelineSecondary()
{
    m_mediaPipelineSecondary.reset();
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
    shouldNotifyPlaybackStateInternal(m_mediaPipelineClientMock, PlaybackState::FAILURE);
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
    sendNotifyPlaybackStateInternal(kSessionId, PlaybackState::FAILURE);
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
    shouldNotifyPlaybackStateInternal(m_mediaPipelineClientMock, PlaybackState::SEEKING);
}

void MediaPipelineTestMethods::shouldNotifyPlaybackStateSeekDone()
{
    shouldNotifyPlaybackStateInternal(m_mediaPipelineClientMock, PlaybackState::SEEK_DONE);
}

void MediaPipelineTestMethods::shouldSetPosition(const int64_t expectedPosition)
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, setPosition(_, setPositionRequestMatcher(kSessionId, expectedPosition), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn)));
}

void MediaPipelineTestMethods::setPosition(const int64_t position)
{
    EXPECT_EQ(m_mediaPipeline->setPosition(position), true);
}

void MediaPipelineTestMethods::sendNotifyPlaybackStateSeeking()
{
    sendNotifyPlaybackStateInternal(kSessionId, PlaybackState::SEEKING);
}

void MediaPipelineTestMethods::sendNotifyPlaybackStateSeekDone()
{
    sendNotifyPlaybackStateInternal(kSessionId, PlaybackState::SEEK_DONE);
}

void MediaPipelineTestMethods::shouldNotifyNeedDataAudio(const size_t framesToWrite)
{
    shouldNotifyNeedDataInternal(m_mediaPipelineClientMock, kAudioSourceId, framesToWrite);
}

void MediaPipelineTestMethods::shouldNotifyNeedDataVideo(const size_t framesToWrite)
{
    shouldNotifyNeedDataInternal(m_mediaPipelineClientMock, kVideoSourceId, framesToWrite);
}

void MediaPipelineTestMethods::shouldNotifyNeedDataVideoSecondary(const size_t framesToWrite)
{
    shouldNotifyNeedDataInternal(m_mediaPipelineClientSecondaryMock, kVideoSourceId, framesToWrite);
}

void MediaPipelineTestMethods::sendNotifyNeedDataVideo(uint32_t framesToWrite)
{
    sendNotifyNeedDataInternal(kSessionId, kVideoSourceId, m_locationToWriteVideo[kPrimaryPartition], framesToWrite);
}

void MediaPipelineTestMethods::sendNotifyNeedDataAudio(uint32_t framesToWrite)
{
    sendNotifyNeedDataInternal(kSessionId, kAudioSourceId, m_locationToWriteAudio[kPrimaryPartition], framesToWrite);
}

void MediaPipelineTestMethods::sendNotifyNeedDataVideoSecondary(uint32_t framesToWrite)
{
    sendNotifyNeedDataInternal(kSessionIdSecondary, kVideoSourceId, m_locationToWriteVideo[kSecondaryPartition],
                               framesToWrite);
}

void MediaPipelineTestMethods::writeAudioFrames()
{
    uint32_t framesToWrite = 3;
    MediaPipelineTestMethods::shouldNotifyNeedDataAudio(framesToWrite);
    MediaPipelineTestMethods::sendNotifyNeedDataAudio(framesToWrite);
    uint32_t segmentId = MediaPipelineTestMethods::addSegmentMseAudio();
    MediaPipelineTestMethods::checkMseAudioSegmentWritten(segmentId);
    segmentId = MediaPipelineTestMethods::addSegmentMseAudio();
    MediaPipelineTestMethods::checkMseAudioSegmentWritten(segmentId);
    segmentId = MediaPipelineTestMethods::addSegmentMseAudio();
    MediaPipelineTestMethods::checkMseAudioSegmentWritten(segmentId);
    MediaPipelineTestMethods::shouldHaveDataOk(framesToWrite);
    MediaPipelineTestMethods::haveDataOk();
}

void MediaPipelineTestMethods::writeVideoFrames()
{
    uint32_t framesToWrite = 3;
    MediaPipelineTestMethods::shouldNotifyNeedDataVideo(framesToWrite);
    MediaPipelineTestMethods::sendNotifyNeedDataVideo(framesToWrite);
    uint32_t segmentId = MediaPipelineTestMethods::addSegmentMseVideo();
    MediaPipelineTestMethods::checkMseVideoSegmentWritten(segmentId);
    segmentId = MediaPipelineTestMethods::addSegmentMseVideo();
    MediaPipelineTestMethods::checkMseVideoSegmentWritten(segmentId);
    segmentId = MediaPipelineTestMethods::addSegmentMseVideo();
    MediaPipelineTestMethods::checkMseVideoSegmentWritten(segmentId);
    MediaPipelineTestMethods::shouldHaveDataOk(framesToWrite);
    MediaPipelineTestMethods::haveDataOk();
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

void MediaPipelineTestMethods::writeVideoFramesSecondary()
{
    uint32_t framesToWrite = 3;
    MediaPipelineTestMethods::shouldNotifyNeedDataVideoSecondary(framesToWrite);
    MediaPipelineTestMethods::sendNotifyNeedDataVideoSecondary(framesToWrite);
    int32_t segmentId = MediaPipelineTestMethods::addSegmentMseVideoSecondary();
    MediaPipelineTestMethods::checkMseVideoSegmentWrittenSecondary(segmentId);
    segmentId = MediaPipelineTestMethods::addSegmentMseVideoSecondary();
    MediaPipelineTestMethods::checkMseVideoSegmentWrittenSecondary(segmentId);
    segmentId = MediaPipelineTestMethods::addSegmentMseVideoSecondary();
    MediaPipelineTestMethods::checkMseVideoSegmentWrittenSecondary(segmentId);
    MediaPipelineTestMethods::shouldHaveDataOkSecondary(framesToWrite);
    MediaPipelineTestMethods::haveDataOkSecondary();
}

void MediaPipelineTestMethods::shouldSetVolume(const double expectedVolume)
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, setVolume(_, setVolumeRequestMatcher(kSessionId, expectedVolume), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn)));
}

void MediaPipelineTestMethods::shouldGetVolume(const double volume)
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, getVolume(_, getVolumeRequestMatcher(kSessionId), _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaPipelineModuleMock->getVolumeResponse(volume)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn))));
}

void MediaPipelineTestMethods::setVolume(const double volume)
{
    EXPECT_EQ(m_mediaPipeline->setVolume(volume), true);
}

void MediaPipelineTestMethods::getVolume(const double expectedVolume)
{
    double returnVolume;
    EXPECT_EQ(m_mediaPipeline->getVolume(returnVolume), true);
    EXPECT_EQ(returnVolume, expectedVolume);
}

void MediaPipelineTestMethods::shouldSetMute(const bool expectedMute)
{
    EXPECT_CALL(*m_mediaPipelineModuleMock,
                setMute(_, setMuteRequestMatcher(kSessionId, kAudioSourceId, expectedMute), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn)));
}

void MediaPipelineTestMethods::shouldGetMute(const bool mute)
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, getMute(_, getMuteRequestMatcher(kSessionId, kAudioSourceId), _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaPipelineModuleMock->getMuteResponse(mute)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn))));
}

void MediaPipelineTestMethods::setMute(const bool mute)
{
    EXPECT_EQ(m_mediaPipeline->setMute(kAudioSourceId, mute), true);
}

void MediaPipelineTestMethods::getMute(const bool expectedMute)
{
    bool returnMute;
    EXPECT_EQ(m_mediaPipeline->getMute(kAudioSourceId, returnMute), true);
    EXPECT_EQ(returnMute, expectedMute);
}

void MediaPipelineTestMethods::shouldSetLowLatency(const bool expectedLowLatency)
{
    EXPECT_CALL(*m_mediaPipelineModuleMock,
                setLowLatency(_, setLowLatencyRequestMatcher(kSessionId, expectedLowLatency), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn)));
}

void MediaPipelineTestMethods::setLowLatency(const bool lowLatency)
{
    EXPECT_EQ(m_mediaPipeline->setLowLatency(lowLatency), true);
}

void MediaPipelineTestMethods::shouldSetSync(const bool expectedSync)
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, setSync(_, setSyncRequestMatcher(kSessionId, expectedSync), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn)));
}

void MediaPipelineTestMethods::setSync(const bool sync)
{
    EXPECT_EQ(m_mediaPipeline->setSync(sync), true);
}

void MediaPipelineTestMethods::shouldGetSync(const bool sync)
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, getSync(_, getSyncRequestMatcher(kSessionId), _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaPipelineModuleMock->getSyncResponse(sync)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn))));
}

void MediaPipelineTestMethods::getSync(const bool expectedSync)
{
    bool returnSync;
    EXPECT_EQ(m_mediaPipeline->getSync(returnSync), true);
    EXPECT_EQ(returnSync, expectedSync);
}

void MediaPipelineTestMethods::shouldSetSyncOff(const bool expectedSyncOff)
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, setSyncOff(_, setSyncOffRequestMatcher(kSessionId, expectedSyncOff), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn)));
}

void MediaPipelineTestMethods::setSyncOff(const bool syncOff)
{
    EXPECT_EQ(m_mediaPipeline->setSyncOff(syncOff), true);
}

void MediaPipelineTestMethods::shouldSetStreamSyncMode(const int32_t expectedStreamSyncMode)
{
    EXPECT_CALL(*m_mediaPipelineModuleMock,
                setStreamSyncMode(_, setStreamSyncModeRequestMatcher(kSessionId, expectedStreamSyncMode), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn)));
}

void MediaPipelineTestMethods::setStreamSyncMode(const int32_t streamSyncMode)
{
    EXPECT_EQ(m_mediaPipeline->setStreamSyncMode(streamSyncMode), true);
}

void MediaPipelineTestMethods::shouldGetStreamSyncMode(const int32_t streamSyncMode)
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, getStreamSyncMode(_, getStreamSyncModeRequestMatcher(kSessionId), _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaPipelineModuleMock->getStreamSyncModeResponse(streamSyncMode)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn))));
}

void MediaPipelineTestMethods::getStreamSyncMode(const int32_t expectedStreamSyncMode)
{
    int32_t returnStreamSyncMode;
    EXPECT_EQ(m_mediaPipeline->getStreamSyncMode(returnStreamSyncMode), true);
    EXPECT_EQ(returnStreamSyncMode, expectedStreamSyncMode);
}

void MediaPipelineTestMethods::shouldSetVideoWindow(const uint32_t expectedX, const uint32_t expectedY,
                                                    const uint32_t expectedWidth, const uint32_t expectedHeight)
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, setVideoWindow(_,
                                                           setVideoWindowRequestMatcher(kSessionId, expectedX, expectedY,
                                                                                        expectedWidth, expectedHeight),
                                                           _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn)));
}

void MediaPipelineTestMethods::setSetVideoWindow(const uint32_t x, const uint32_t y, const uint32_t width,
                                                 const uint32_t height)
{
    EXPECT_EQ(m_mediaPipeline->setVideoWindow(x, y, width, height), true);
}

void MediaPipelineTestMethods::shouldRenderFrame()
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, renderFrame(_, renderFrameRequestMatcher(kSessionId), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn)));
}

void MediaPipelineTestMethods::renderFrame()
{
    EXPECT_EQ(m_mediaPipeline->renderFrame(), true);
}

void MediaPipelineTestMethods::shouldRenderFrameFailure()
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, renderFrame(_, renderFrameRequestMatcher(kSessionId), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::failureReturn)));
}

void MediaPipelineTestMethods::renderFrameFailure()
{
    EXPECT_EQ(m_mediaPipeline->renderFrame(), false);
}

void MediaPipelineTestMethods::shouldNotifyPosition(const uint32_t expectedPosition)
{
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyPosition(expectedPosition))
        .WillOnce(Invoke(this, &MediaPipelineTestMethods::notifyEvent));
}

void MediaPipelineTestMethods::sendNotifyPositionChanged(const int64_t position)
{
    getServerStub()->notifyPositionChangeEvent(kSessionId, position);
    waitEvent();
}

void MediaPipelineTestMethods::shouldNotifyQosAudio()
{
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyQos(kAudioSourceId, qosInfoMatcher(kQosInfoAudio)))
        .WillOnce(Invoke(this, &MediaPipelineTestMethods::notifyEvent));
}

void MediaPipelineTestMethods::shouldNotifyQosVideo()
{
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyQos(kVideoSourceId, qosInfoMatcher(kQosInfoVideo)))
        .WillOnce(Invoke(this, &MediaPipelineTestMethods::notifyEvent));
}

void MediaPipelineTestMethods::sendNotifyQosAudio()
{
    getServerStub()->notifyQosEvent(kSessionId, kAudioSourceId, kQosInfoAudio);
    waitEvent();
}

void MediaPipelineTestMethods::sendNotifyQosVideo()
{
    getServerStub()->notifyQosEvent(kSessionId, kVideoSourceId, kQosInfoVideo);
    waitEvent();
}

void MediaPipelineTestMethods::shouldNotifyBufferUnderflowAudio()
{
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyBufferUnderflow(kAudioSourceId))
        .WillOnce(Invoke(this, &MediaPipelineTestMethods::notifyEvent));
}

void MediaPipelineTestMethods::shouldNotifyBufferUnderflowVideo()
{
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyBufferUnderflow(kVideoSourceId))
        .WillOnce(Invoke(this, &MediaPipelineTestMethods::notifyEvent));
}

void MediaPipelineTestMethods::sendNotifyBufferUnderflowAudio()
{
    getServerStub()->notifyBufferUnderflowEvent(kSessionId, kAudioSourceId);
    waitEvent();
}

void MediaPipelineTestMethods::sendNotifyBufferUnderflowVideo()
{
    getServerStub()->notifyBufferUnderflowEvent(kSessionId, kVideoSourceId);
    waitEvent();
}

void MediaPipelineTestMethods::shouldNotifyPlaybackErrorAudio()
{
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyPlaybackError(kAudioSourceId, PlaybackError::DECRYPTION))
        .WillOnce(Invoke(this, &MediaPipelineTestMethods::notifyEvent));
}

void MediaPipelineTestMethods::shouldNotifyPlaybackErrorVideo()
{
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyPlaybackError(kVideoSourceId, PlaybackError::DECRYPTION))
        .WillOnce(Invoke(this, &MediaPipelineTestMethods::notifyEvent));
}

void MediaPipelineTestMethods::sendNotifyPlaybackErrorAudio()
{
    getServerStub()->notifyPlaybackErrorEvent(kSessionId, kAudioSourceId, PlaybackError::DECRYPTION);
    waitEvent();
}

void MediaPipelineTestMethods::sendNotifyPlaybackErrorVideo()
{
    getServerStub()->notifyPlaybackErrorEvent(kSessionId, kVideoSourceId, PlaybackError::DECRYPTION);
    waitEvent();
}

void MediaPipelineTestMethods::shouldNotifySourceFlushed()
{
    EXPECT_CALL(*m_mediaPipelineClientMock, notifySourceFlushed(kAudioSourceId))
        .WillOnce(Invoke(this, &MediaPipelineTestMethods::notifyEvent));
}

void MediaPipelineTestMethods::sendNotifySourceFlushed()
{
    getServerStub()->notifySourceFlushed(kSessionId, kAudioSourceId);
    waitEvent();
}

void MediaPipelineTestMethods::shouldGetPosition(const int64_t position)
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, getPosition(_, getPositionRequestMatcher(kSessionId), _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaPipelineModuleMock->getPositionResponse(position)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn))));
}

void MediaPipelineTestMethods::getPosition(const int64_t expectedPosition)
{
    int64_t returnPosition;
    EXPECT_EQ(m_mediaPipeline->getPosition(returnPosition), true);
    EXPECT_EQ(returnPosition, expectedPosition);
}

void MediaPipelineTestMethods::shouldSetImmediateOutput(bool immediateOutput)
{
    EXPECT_CALL(*m_mediaPipelineModuleMock,
                setImmediateOutput(_, setImmediateOutputRequestMatcher(kSessionId, kVideoSourceId), _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaPipelineModuleMock->setImmediateOutputResponse()),
                        WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn))));
}

void MediaPipelineTestMethods::setImmediateOutput(bool immediateOutput)
{
    EXPECT_TRUE(m_mediaPipeline->setImmediateOutput(kVideoSourceId, immediateOutput));
}

void MediaPipelineTestMethods::shouldGetImmediateOutput(bool immediateOutput)
{
    EXPECT_CALL(*m_mediaPipelineModuleMock,
                getImmediateOutput(_, getImmediateOutputRequestMatcher(kSessionId, kVideoSourceId), _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaPipelineModuleMock->getImmediateOutputResponse(immediateOutput)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn))));
}

void MediaPipelineTestMethods::getImmediateOutput(bool immediateOutput)
{
    EXPECT_TRUE(m_mediaPipeline->getImmediateOutput(kVideoSourceId, immediateOutput));
}

void MediaPipelineTestMethods::shouldGetStats(uint64_t renderedFrames, uint64_t droppedFrames)
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, getStats(_, getStatsRequestMatcher(kSessionId, kVideoSourceId), _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaPipelineModuleMock->getStatsResponse(renderedFrames, droppedFrames)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn))));
}

void MediaPipelineTestMethods::getStats(uint64_t expectedFrames, uint64_t expectedDropped)
{
    uint64_t returnFrames;
    uint64_t returnDropped;
    EXPECT_TRUE(m_mediaPipeline->getStats(kVideoSourceId, returnFrames, returnDropped));
    EXPECT_EQ(returnFrames, expectedFrames);
    EXPECT_EQ(returnDropped, expectedDropped);
}

void MediaPipelineTestMethods::shouldFlush()
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, flush(_, flushRequestMatcher(kSessionId, kAudioSourceId, kResetTime), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn)));
}

void MediaPipelineTestMethods::flush()
{
    EXPECT_TRUE(m_mediaPipeline->flush(kAudioSourceId, kResetTime));
}

void MediaPipelineTestMethods::shouldFailToFlush()
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, flush(_, flushRequestMatcher(kSessionId, kAudioSourceId, kResetTime), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::failureReturn)));
}

void MediaPipelineTestMethods::flushFailure()
{
    EXPECT_FALSE(m_mediaPipeline->flush(kAudioSourceId, kResetTime));
}

void MediaPipelineTestMethods::shouldSetSourcePosition()
{
    EXPECT_CALL(*m_mediaPipelineModuleMock,
                setSourcePosition(_, setSourcePositionRequestMatcher(kSessionId, kAudioSourceId, kPosition, kResetTime),
                                  _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn)));
}

void MediaPipelineTestMethods::setSourcePosition()
{
    EXPECT_TRUE(m_mediaPipeline->setSourcePosition(kAudioSourceId, kPosition, kResetTime));
}

void MediaPipelineTestMethods::shouldFailToSetSourcePosition()
{
    EXPECT_CALL(*m_mediaPipelineModuleMock,
                setSourcePosition(_, setSourcePositionRequestMatcher(kSessionId, kAudioSourceId, kPosition, kResetTime),
                                  _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::failureReturn)));
}

void MediaPipelineTestMethods::setSourcePositionFailure()
{
    EXPECT_FALSE(m_mediaPipeline->setSourcePosition(kAudioSourceId, kPosition, kResetTime));
}

void MediaPipelineTestMethods::shouldProcessAudioGap()
{
    EXPECT_CALL(*m_mediaPipelineModuleMock,
                processAudioGap(_,
                                processAudioGapRequestMatcher(kSessionId, kPosition, kDuration, kDiscontinuityGap,
                                                              kIsAudioAac),
                                _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn)));
}

void MediaPipelineTestMethods::processAudioGap()
{
    EXPECT_TRUE(m_mediaPipeline->processAudioGap(kPosition, kDuration, kDiscontinuityGap, kIsAudioAac));
}

void MediaPipelineTestMethods::shouldFailToProcessAudioGap()
{
    EXPECT_CALL(*m_mediaPipelineModuleMock,
                processAudioGap(_,
                                processAudioGapRequestMatcher(kSessionId, kPosition, kDuration, kDiscontinuityGap,
                                                              kIsAudioAac),
                                _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::failureReturn)));
}

void MediaPipelineTestMethods::processAudioGapFailure()
{
    EXPECT_FALSE(m_mediaPipeline->processAudioGap(kPosition, kDuration, kDiscontinuityGap, kIsAudioAac));
}

/*************************** Private methods ********************************/

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

uint32_t MediaPipelineTestMethods::getTimestamp(uint32_t segmentId)
{
    return kTimeStamp + 1000 * (segmentId + 1);
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
    EXPECT_EQ(metadata.width(), kWidth720p);
    EXPECT_TRUE(metadata.has_height());
    EXPECT_EQ(metadata.height(), kHeight720p);
    EXPECT_TRUE(metadata.has_frame_rate());
    EXPECT_THAT(metadata.frame_rate(), frameRateMatcher(kFrameRateEmpty));
}

void MediaPipelineTestMethods::checkVideoMetadataSecondary(const MediaSegmentMetadata &metadata, uint32_t segmentId)
{
    EXPECT_TRUE(metadata.has_length());
    EXPECT_EQ(metadata.length(), kVideoSegments[segmentId].size());
    EXPECT_TRUE(metadata.has_time_position());
    EXPECT_EQ(metadata.time_position(), getTimestamp(segmentId));
    EXPECT_TRUE(metadata.has_sample_duration());
    EXPECT_EQ(metadata.sample_duration(), kDurationSecondary);
    EXPECT_TRUE(metadata.has_stream_id());
    EXPECT_EQ(metadata.stream_id(), kVideoSourceId);
    EXPECT_TRUE(metadata.has_width());
    EXPECT_EQ(metadata.width(), kWidthSecondary);
    EXPECT_TRUE(metadata.has_height());
    EXPECT_EQ(metadata.height(), kHeightSecondary);
    EXPECT_TRUE(metadata.has_frame_rate());
    EXPECT_THAT(metadata.frame_rate(), frameRateMatcher(kFrameRateSecondary));
}

void MediaPipelineTestMethods::checkEncryptionMetadata(const MediaSegmentMetadata &metadata, uint32_t keyIndex)
{
    EXPECT_TRUE(metadata.has_media_key_session_id());
    EXPECT_EQ(metadata.media_key_session_id(), kKeySessionId);
    EXPECT_TRUE(metadata.has_key_id());
    EXPECT_EQ(metadata.key_id(), std::string(kKeyStatuses[keyIndex].first.begin(), kKeyStatuses[keyIndex].first.end()));
    EXPECT_TRUE(metadata.has_init_vector());
    EXPECT_EQ(metadata.init_vector(), std::string(kInitData.begin(), kInitData.end()));
    EXPECT_TRUE(metadata.has_init_with_last_15());
    EXPECT_EQ(metadata.init_with_last_15(), kInitWithLast15);
    EXPECT_NE(metadata.sub_sample_info().size(), 0);
    for (int i = 0; i < metadata.sub_sample_info().size(); i++)
    {
        EXPECT_EQ(metadata.sub_sample_info()[i].num_clear_bytes(), kSubSamplePairs[i].numClearBytes);
        EXPECT_EQ(metadata.sub_sample_info()[i].num_encrypted_bytes(), kSubSamplePairs[i].numEncryptedBytes);
    }
    EXPECT_TRUE(metadata.has_cipher_mode());
    EXPECT_EQ(metadata.cipher_mode(), convertCipherMode(kCipherModeCenc));
    EXPECT_TRUE(metadata.has_crypt());
    EXPECT_EQ(metadata.crypt(), kCrypt);
    EXPECT_TRUE(metadata.has_skip());
    EXPECT_EQ(metadata.skip(), kSkip);
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

void MediaPipelineTestMethods::checkKeyId(const MediaPlayerShmInfo &shmInfo, uint32_t keyIndex)
{
    MediaSegmentMetadata metadata;
    parseMetadata(metadata, shmInfo.mediaDataOffset);

    EXPECT_TRUE(metadata.has_key_id());
    EXPECT_EQ(metadata.key_id(), std::string(kKeyStatuses[keyIndex].first.begin(), kKeyStatuses[keyIndex].first.end()));
}

void MediaPipelineTestMethods::addEncryptedDataToSegment(std::unique_ptr<IMediaPipeline::MediaSegment> &mseData,
                                                         int32_t keyIndex)
{
    mseData->setEncrypted(true);
    mseData->setMediaKeySessionId(kKeySessionId);
    if (keyIndex >= 0)
    {
        mseData->setKeyId(kKeyStatuses[keyIndex].first);
    }
    mseData->setInitVector(kInitData);
    for (const auto &info : kSubSamplePairs)
    {
        mseData->addSubSample(info.numClearBytes, info.numEncryptedBytes);
    }
    mseData->setInitWithLast15(kInitWithLast15);
    mseData->setCipherMode(kCipherModeCenc);
    mseData->setEncryptionPattern(kCrypt, kSkip);
}

void MediaPipelineTestMethods::shouldCreateMediaSessionInternal(const int32_t sessionId,
                                                                const VideoRequirements &videoRequirements)
{
    EXPECT_CALL(*m_mediaPipelineModuleMock,
                createSession(_, createSessionRequestMatcher(videoRequirements.maxWidth, videoRequirements.maxHeight),
                              _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaPipelineModuleMock->createSessionResponse(sessionId)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn))));
}

void MediaPipelineTestMethods::shouldLoadInternal(const int32_t sessionId, const MediaType &mediaType,
                                                  const std::string &mimeType, const std::string &url)
{
    EXPECT_CALL(*m_mediaPipelineModuleMock,
                load(_, loadRequestMatcher(sessionId, convertMediaType(mediaType), mimeType, url), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn)));
}

void MediaPipelineTestMethods::shouldRemoveSourceInternal(const int32_t sessionId, const int32_t sourceId)
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, removeSource(_, removeSourceRequestMatcher(sessionId, sourceId), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn)));
}

void MediaPipelineTestMethods::shouldStopInternal(const int32_t sessionId)
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, stop(_, stopRequestMatcher(sessionId), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn)));
}

void MediaPipelineTestMethods::shouldPlayInternal(const int32_t sessionId)
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, play(_, playRequestMatcher(sessionId), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn)));
}

void MediaPipelineTestMethods::shouldDestroyMediaSessionInternal(const int32_t sessionId)
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, destroySession(_, destroySessionRequestMatcher(sessionId), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn)));
}

void MediaPipelineTestMethods::shouldAttachVideoSourceInternal(
    const int32_t sessionId, const std::string &mimeType, bool hasNoDrm, const int32_t width, const int32_t height,
    const firebolt::rialto::SegmentAlignment &alignment, const std::shared_ptr<firebolt::rialto::CodecData> &codacData,
    const firebolt::rialto::StreamFormat &streamFormat)
{
    EXPECT_CALL(*m_mediaPipelineModuleMock,
                attachSource(_,
                             attachSourceRequestMatcherVideo(sessionId, mimeType.c_str(), hasNoDrm, width, height,
                                                             alignment, codacData, convertStreamFormat(streamFormat)),
                             _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaPipelineModuleMock->attachSourceResponse(kVideoSourceId)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn))));
}

void MediaPipelineTestMethods::shouldAttachAudioSourceInternal(
    const std::string &mimeType, bool hasNoDrm, const firebolt::rialto::SegmentAlignment &alignment,
    const uint32_t noOfChannels, const uint32_t sampleRate, const std::string &codecSpecificConfigStr,
    const std::shared_ptr<firebolt::rialto::CodecData> &codacData, const firebolt::rialto::StreamFormat &streamFormat)
{
    EXPECT_CALL(*m_mediaPipelineModuleMock,
                attachSource(_,
                             attachSourceRequestMatcherAudio(kSessionId, mimeType, hasNoDrm, alignment, noOfChannels,
                                                             sampleRate, codecSpecificConfigStr, codacData,
                                                             convertStreamFormat(streamFormat)),
                             _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaPipelineModuleMock->attachSourceResponse(kAudioSourceId)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn))));
}

void MediaPipelineTestMethods::shouldAllSourcesAttachedInternal(const int32_t sessionId)
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, allSourcesAttached(_, allSourcesAttachedRequestMatcher(sessionId), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn)));
}

void MediaPipelineTestMethods::shouldHaveDataInternal(const int32_t sessionId, const MediaSourceStatus status,
                                                      const size_t framesWritten, const uint32_t partition)
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, haveData(_,
                                                     haveDataRequestMatcher(sessionId, convertMediaSourceStatus(status),
                                                                            framesWritten, m_needDataRequestId),
                                                     _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(
            [&, partition](::google::protobuf::RpcController *controller, ::google::protobuf::Closure *done)
            {
                // Increment needData request Id
                m_needDataRequestId++;
                m_mediaPipelineModuleMock->defaultReturn(controller, done);
                resetWriteLocation(partition);
            })));
}

void MediaPipelineTestMethods::shouldNotifyPlaybackStateInternal(
    const std::shared_ptr<StrictMock<MediaPipelineClientMock>> &clientMock, const PlaybackState &state)
{
    EXPECT_CALL(*clientMock, notifyPlaybackState(state)).WillOnce(Invoke(this, &MediaPipelineTestMethods::notifyEvent));
}

void MediaPipelineTestMethods::shouldNotifyNetworkStateInternal(
    const std::shared_ptr<StrictMock<MediaPipelineClientMock>> &clientMock, const NetworkState &state)
{
    EXPECT_CALL(*clientMock, notifyNetworkState(state)).WillOnce(Invoke(this, &MediaPipelineTestMethods::notifyEvent));
}

void MediaPipelineTestMethods::shouldNotifyNeedDataInternal(
    const std::shared_ptr<StrictMock<MediaPipelineClientMock>> &clientMock, const int32_t sourceId,
    const size_t framesToWrite)
{
    EXPECT_CALL(*clientMock, notifyNeedMediaData(sourceId, framesToWrite, m_needDataRequestId, kNullShmInfo))
        .WillOnce(InvokeWithoutArgs(
            [&]()
            {
                // Set the firstSegment flag
                m_firstSegmentOfNeedData = true;
                notifyEvent();
            }));
}

void MediaPipelineTestMethods::createMediaPipelineInternal(std::unique_ptr<IMediaPipeline> &mediaPipeline,
                                                           const std::shared_ptr<StrictMock<MediaPipelineClientMock>> &client,
                                                           const VideoRequirements &videoRequirements)
{
    m_mediaPipelineFactory = firebolt::rialto::IMediaPipelineFactory::createFactory();
    mediaPipeline = m_mediaPipelineFactory->createMediaPipeline(client, videoRequirements);
    EXPECT_NE(mediaPipeline, nullptr);
}

void MediaPipelineTestMethods::loadInternal(const std::unique_ptr<IMediaPipeline> &mediaPipeline,
                                            const MediaType &mediaType, const std::string &mimeType,
                                            const std::string &url, const bool status)
{
    EXPECT_EQ(mediaPipeline->load(mediaType, mimeType, url), status);
}

void MediaPipelineTestMethods::removeSourceInternal(const std::unique_ptr<IMediaPipeline> &mediaPipeline,
                                                    const int32_t sourceId, const bool status)
{
    EXPECT_EQ(mediaPipeline->removeSource(sourceId), status);
}

void MediaPipelineTestMethods::stopInternal(const std::unique_ptr<IMediaPipeline> &mediaPipeline, const bool status)
{
    EXPECT_EQ(mediaPipeline->stop(), status);
}

void MediaPipelineTestMethods::attachSourceVideoInternal(const std::unique_ptr<IMediaPipeline> &mediaPipeline,
                                                         const std::string &mimeType, bool hasNoDrm,
                                                         const int32_t width, const int32_t height,
                                                         const firebolt::rialto::SegmentAlignment &alignment,
                                                         const std::shared_ptr<firebolt::rialto::CodecData> &codacData,
                                                         const firebolt::rialto::StreamFormat &streamFormat,
                                                         const bool status)
{
    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceVideo>(mimeType.c_str(), hasNoDrm, width, height, alignment,
                                                           streamFormat, codacData);
    EXPECT_EQ(mediaPipeline->attachSource(mediaSource), status);
}

void MediaPipelineTestMethods::attachSourceAudioInternal(const std::string &mimeType, bool hasNoDrm,
                                                         const firebolt::rialto::SegmentAlignment &alignment,
                                                         const uint32_t noOfChannels, const uint32_t sampleRate,
                                                         const std::string &codecSpecificConfigStr,
                                                         const std::shared_ptr<firebolt::rialto::CodecData> &codacData,
                                                         const firebolt::rialto::StreamFormat &streamFormat,
                                                         const bool status)
{
    std::vector<uint8_t> codecSpecificConfig;
    codecSpecificConfig.assign(codecSpecificConfigStr.begin(), codecSpecificConfigStr.end());
    AudioConfig audioConfig{noOfChannels, sampleRate, codecSpecificConfig};

    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceAudio>(mimeType.c_str(), hasNoDrm, audioConfig, alignment,
                                                           streamFormat, codacData);
    EXPECT_EQ(m_mediaPipeline->attachSource(mediaSource), true);
}

void MediaPipelineTestMethods::allSourcesAttachedInternal(const std::unique_ptr<IMediaPipeline> &mediaPipeline,
                                                          const bool status)
{
    EXPECT_EQ(mediaPipeline->allSourcesAttached(), status);
}

int32_t MediaPipelineTestMethods::addSegmentMseVideoInternal(const std::unique_ptr<IMediaPipeline> &mediaPipeline,
                                                             const int64_t duration, const int32_t width,
                                                             const int32_t height, const Fraction &frameRate,
                                                             const uint32_t partitionId, const AddSegmentStatus &status)
{
    EXPECT_LT(m_videoSegmentCount, sizeof(kVideoSegments) / sizeof(kVideoSegments[0]));

    std::unique_ptr<IMediaPipeline::MediaSegment> mseData =
        std::make_unique<IMediaPipeline::MediaSegmentVideo>(kVideoSourceId, getTimestamp(m_videoSegmentCount), duration,
                                                            width, height, frameRate);
    mseData->setData(kVideoSegments[m_videoSegmentCount].size(),
                     (const uint8_t *)kVideoSegments[m_videoSegmentCount].c_str());
    EXPECT_EQ(mediaPipeline->addSegment(m_needDataRequestId, mseData), status);

    // Store where the segment should be written so we can check the data
    int32_t segmentId = m_videoSegmentCount;
    writtenVideoSegments.insert({segmentId, *m_locationToWriteVideo[partitionId]});

    incrementWriteLocation(kVideoSegments[m_videoSegmentCount].size(), m_locationToWriteVideo[partitionId]);

    m_videoSegmentCount++;
    return segmentId;
}

void MediaPipelineTestMethods::haveDataInternal(const std::unique_ptr<IMediaPipeline> &mediaPipeline,
                                                const MediaSourceStatus &mediaStatus, const bool status)
{
    EXPECT_EQ(mediaPipeline->haveData(mediaStatus, m_needDataRequestId), status);
}

void MediaPipelineTestMethods::playInternal(const std::unique_ptr<IMediaPipeline> &mediaPipeline, const bool status)
{
    EXPECT_EQ(mediaPipeline->play(), status);
}

void MediaPipelineTestMethods::sendNotifyPlaybackStateInternal(const int32_t sessionId, const PlaybackState &state)
{
    getServerStub()->notifyPlaybackStateChangeEvent(sessionId, state);
    waitEvent();
}

void MediaPipelineTestMethods::sendNotifyNetworkStateInternal(const int32_t sessionId, const NetworkState &state)
{
    getServerStub()->notifyNetworkStateChangeEvent(sessionId, state);
    waitEvent();
}

void MediaPipelineTestMethods::sendNotifyNeedDataInternal(const int32_t sessionId, const int32_t sourceId,
                                                          const std::shared_ptr<MediaPlayerShmInfo> &location,
                                                          uint32_t framesToWrite)
{
    getServerStub()->notifyNeedMediaDataEvent(sessionId, sourceId, framesToWrite, m_needDataRequestId, location);
    waitEvent();
}

uint8_t *MediaPipelineTestMethods::parseMetadata(MediaSegmentMetadata &metadataStruct, const uint32_t metadataOffset)
{
    uint8_t *dataPosition{reinterpret_cast<uint8_t *>(getShmAddress()) + metadataOffset};
    std::uint32_t *metadataSize{reinterpret_cast<uint32_t *>(dataPosition)};
    dataPosition += sizeof(uint32_t);
    EXPECT_TRUE(metadataStruct.ParseFromArray(dataPosition, *metadataSize));
    return dataPosition + *metadataSize;
}

void MediaPipelineTestMethods::createMediaPipelineCapabilitiesObject()
{
    m_mediaPipelineCapabilitiesFactory = firebolt::rialto::IMediaPipelineCapabilitiesFactory::createFactory();
    m_mediaPipelineCapabilities = m_mediaPipelineCapabilitiesFactory->createMediaPipelineCapabilities();
    EXPECT_NE(m_mediaPipelineCapabilities, nullptr);
}

void MediaPipelineTestMethods::destroyMediaPipelineCapabilitiesObject()
{
    m_mediaPipelineCapabilities.reset();
}

void MediaPipelineTestMethods::shouldGetSupportedAudioMimeTypes()
{
    EXPECT_CALL(*m_mediaPipelineCapabilitiesModuleMock,
                getSupportedMimeTypes(_, getSupportedMimeTypesRequestMatcher(firebolt::rialto::MediaSourceType::AUDIO),
                                      _, _))
        .WillOnce(
            DoAll(SetArgPointee<2>(m_mediaPipelineCapabilitiesModuleMock->getSupportedMimeTypesResponse(kAudioMimeType)),
                  WithArgs<0, 3>(Invoke(&(*m_mediaPipelineCapabilitiesModuleMock),
                                        &MediaPipelineCapabilitiesModuleMock::defaultReturn))));
}

void MediaPipelineTestMethods::getSupportedAudioMimeTypes()
{
    MediaSourceType sourceType = firebolt::rialto::MediaSourceType::AUDIO;
    EXPECT_EQ(m_mediaPipelineCapabilities->getSupportedMimeTypes(sourceType), kAudioMimeType);
}

void MediaPipelineTestMethods::shouldGetSupportedVideoMimeTypes()
{
    EXPECT_CALL(*m_mediaPipelineCapabilitiesModuleMock,
                getSupportedMimeTypes(_, getSupportedMimeTypesRequestMatcher(firebolt::rialto::MediaSourceType::VIDEO),
                                      _, _))
        .WillOnce(
            DoAll(SetArgPointee<2>(m_mediaPipelineCapabilitiesModuleMock->getSupportedMimeTypesResponse(kVideoMimeType)),
                  WithArgs<0, 3>(Invoke(&(*m_mediaPipelineCapabilitiesModuleMock),
                                        &MediaPipelineCapabilitiesModuleMock::defaultReturn))));
}

void MediaPipelineTestMethods::getSupportedVideoMimeTypes()
{
    MediaSourceType sourceType = firebolt::rialto::MediaSourceType::VIDEO;
    EXPECT_EQ(m_mediaPipelineCapabilities->getSupportedMimeTypes(sourceType), kVideoMimeType);
}

void MediaPipelineTestMethods::shouldGetSupportedUnknownMimeTypes()
{
    EXPECT_CALL(*m_mediaPipelineCapabilitiesModuleMock,
                getSupportedMimeTypes(_, getSupportedMimeTypesRequestMatcher(firebolt::rialto::MediaSourceType::UNKNOWN),
                                      _, _))
        .WillOnce(DoAll(SetArgPointee<2>(
                            m_mediaPipelineCapabilitiesModuleMock->getSupportedMimeTypesResponse(kUnknownMimeType)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaPipelineCapabilitiesModuleMock),
                                              &MediaPipelineCapabilitiesModuleMock::defaultReturn))));
}

void MediaPipelineTestMethods::getUnknownMimeTypes()
{
    MediaSourceType sourceType = firebolt::rialto::MediaSourceType::UNKNOWN;
    EXPECT_EQ(m_mediaPipelineCapabilities->getSupportedMimeTypes(sourceType), kUnknownMimeType);
}

void MediaPipelineTestMethods::shouldCheckIsMimeTypeSupported()
{
    EXPECT_CALL(*m_mediaPipelineCapabilitiesModuleMock,
                isMimeTypeSupported(_, isMimeTypeSupportedRequestMatcher(kVideoMimeType[0]), _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaPipelineCapabilitiesModuleMock->isMimeTypeSupportedResponse(true)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaPipelineCapabilitiesModuleMock),
                                              &MediaPipelineCapabilitiesModuleMock::defaultReturn))));
}

void MediaPipelineTestMethods::isMimeTypeSupported()
{
    EXPECT_EQ(m_mediaPipelineCapabilities->isMimeTypeSupported(kVideoMimeType[0]), true);
}

void MediaPipelineTestMethods::shouldCheckIsMimeTypeNotSupported()
{
    EXPECT_CALL(*m_mediaPipelineCapabilitiesModuleMock,
                isMimeTypeSupported(_, isMimeTypeSupportedRequestMatcher(kVideoMimeType[0]), _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaPipelineCapabilitiesModuleMock->isMimeTypeSupportedResponse(false)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaPipelineCapabilitiesModuleMock),
                                              &MediaPipelineCapabilitiesModuleMock::defaultReturn))));
}

void MediaPipelineTestMethods::isMimeTypeNotSupported()
{
    EXPECT_EQ(m_mediaPipelineCapabilities->isMimeTypeSupported(kVideoMimeType[0]), false);
}

void MediaPipelineTestMethods::shouldGetSupportedProperties()
{
    EXPECT_CALL(*m_mediaPipelineCapabilitiesModuleMock,
                getSupportedProperties(_,
                                       getSupportedPropertiesRequestMatcher(firebolt::rialto::MediaSourceType::VIDEO,
                                                                            kSupportedProperties),
                                       _, _))
        .WillOnce(DoAll(SetArgPointee<2>(
                            m_mediaPipelineCapabilitiesModuleMock->getSupportedPropertiesResponse(kSupportedProperties)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaPipelineCapabilitiesModuleMock),
                                              &MediaPipelineCapabilitiesModuleMock::defaultReturn))));
}

void MediaPipelineTestMethods::getSupportedProperties()
{
    MediaSourceType sourceType = firebolt::rialto::MediaSourceType::VIDEO;
    EXPECT_EQ(m_mediaPipelineCapabilities->getSupportedProperties(sourceType, kSupportedProperties),
              kSupportedProperties);
}

void MediaPipelineTestMethods::shouldGetSupportedPropertiesFailure()
{
    EXPECT_CALL(*m_mediaPipelineCapabilitiesModuleMock,
                getSupportedProperties(_,
                                       getSupportedPropertiesRequestMatcher(firebolt::rialto::MediaSourceType::VIDEO,
                                                                            kSupportedProperties),
                                       _, _))
        .WillOnce(DoAll(SetArgPointee<2>(
                            m_mediaPipelineCapabilitiesModuleMock->getSupportedPropertiesResponse(kSupportedProperties)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaPipelineCapabilitiesModuleMock),
                                              &MediaPipelineCapabilitiesModuleMock::failureReturn))));
}

void MediaPipelineTestMethods::getSupportedPropertiesFailure()
{
    MediaSourceType sourceType = firebolt::rialto::MediaSourceType::VIDEO;
    EXPECT_TRUE(m_mediaPipelineCapabilities->getSupportedProperties(sourceType, kSupportedProperties).empty());
}
} // namespace firebolt::rialto::client::ct
