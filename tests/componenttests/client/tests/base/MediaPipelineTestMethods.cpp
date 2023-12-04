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
#include "MetadataProtoUtils.h"
#include "metadata.pb.h"
#include <memory>

namespace
{
    constexpr VideoRequirements kVideoRequirements{123, 456};
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
    firebolt::rialto::Fraction kFrameRate = {1,1};

    //MediaSegments
    const std::string kAudioSegments[27] = {"plhyftrsf", "svgywwx", "0kine64hd~;envss", "nxwfy5fvbgg^sfh7&", "svrvy63fbw1sxg", "dfc", "fhji94gjbf", "g0494vf", "fso4[;dw]", "pp[;,6mkf]", "o", "gre94", "g04[]", "ogjow", "fgrejvfd", "gjkt92ijs", "xnute", "0g", "9jsonf", "g0e9r0[fsss]", "44444444", "gf0iksikgs", "ge943pv", "ge-q[[]]", "803batvgff", "vfe83", "94FG"};
    const std::string kVideoSegments[27] = {"lloefewhrwohruwhfr9eee8833hbcjka", "#mcuruiw83udbhc", "cnvbvfbuhrew8383hbvbckzoaopalwlelhvrwvc", "nvid9", "viru80202-idendhjx cefebfihgrwwwwcdcwd", "ncie83uuwonvbbvaoaodoekjvbv  vyreifur779y", "yr6w4hgsbhjk674kuur", "g5u7khdhg;popuyteyeyjhdjy", "fhttwtjbts5568khdtjy", "hrjkl8p09[jtrwhstry5twyyuhhgdhtyww455y]", "htrkuyr", "yu764thgkio9p9trw545jjhf", "ju;p'#hsgr55hthghh", "hrtjnaty54", "huykil0[-thtshngds5435yjjiulkitgdhhh]", "hdml;pup'654sfshgshyu65tgni5566754ui76i", "fdshyj7647whyjdjl;;pyuhdgtyhyjey", "trjjyte", "hdth4y67ggshqqq125tjytel7t", "ku00-uyuj766677jkutjytydyj", "yyt", "shtrnkkil ik;oiy,etq 525425", "hyrejhye y6u2255y6858geqszz", "ngkyirlkrst45ghmn,", "jkl;p'#tshr4faggra", "greabfdbnmur7y64wnguyoyrar55shshjw", "G9042HG"};
} // namespace

MediaPipelineTestMethods::MediaPipelineTestMethods(const MediaPlayerShmInfo &audioShmInfo, const MediaPlayerShmInfo &videoShmInfo)
    : m_mediaPipelineClientMock{std::make_shared<StrictMock<MediaPipelineClientMock>>()},
      m_mediaPipelineModuleMock{std::make_shared<StrictMock<MediaPipelineModuleMock>>()},
      m_kAudioShmInfo{audioShmInfo}, m_kVideoShmInfo{videoShmInfo}
{
    kCodecData->data = std::vector<std::uint8_t>{'T', 'E', 'S', 'T'};
    kCodecData->type = firebolt::rialto::CodecDataType::BUFFER;

    resetWriteLocation(audioShmInfo, videoShmInfo);
}

MediaPipelineTestMethods::~MediaPipelineTestMethods()
{
}

void MediaPipelineTestMethods::resetWriteLocation(const MediaPlayerShmInfo &audioShmInfo, const MediaPlayerShmInfo &videoShmInfo)
{
    m_locationToWriteAudio->maxMetadataBytes = audioShmInfo.maxMetadataBytes;
    m_locationToWriteAudio->metadataOffset = audioShmInfo.metadataOffset;
    m_locationToWriteAudio->maxMediaBytes = audioShmInfo.maxMediaBytes;
    m_locationToWriteAudio->mediaDataOffset = audioShmInfo.mediaDataOffset;
    m_locationToWriteVideo->maxMetadataBytes = videoShmInfo.maxMetadataBytes;
    m_locationToWriteVideo->metadataOffset = videoShmInfo.metadataOffset;
    m_locationToWriteVideo->maxMediaBytes = videoShmInfo.maxMediaBytes;
    m_locationToWriteVideo->mediaDataOffset = videoShmInfo.mediaDataOffset;
}

void MediaPipelineTestMethods::shouldCreateMediaSession()
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, createSession(_, createSessionRequestMatcher(kVideoRequirements.maxWidth, kVideoRequirements.maxHeight), _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaPipelineModuleMock->createSessionResponse(kSessionId)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn))));
}

void MediaPipelineTestMethods::createMediaPipeline()
{
    m_mediaPipelineFactory = firebolt::rialto::IMediaPipelineFactory::createFactory();
    m_mediaPipeline = m_mediaPipelineFactory->createMediaPipeline(m_mediaPipelineClientMock, kVideoRequirements);
    EXPECT_NE(m_mediaPipeline, nullptr);
}

void MediaPipelineTestMethods::shouldLoad()
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, load(_, loadRequestMatcher(kSessionId, convertMediaType(kMediaType), kMimeType, kUrl), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn)));
}

void MediaPipelineTestMethods::load()
{
    EXPECT_EQ(m_mediaPipeline->load(kMediaType, kMimeType, kUrl), true);
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
    EXPECT_CALL(*m_mediaPipelineModuleMock, attachSource(_, attachSourceRequestMatcherVideo(kSessionId,
                                                           kMimeType.c_str(), kHasNoDrm, kWidthUhd, kHeightUhd, kAlignment, kCodecData, convertStreamFormat(kStreamFormat)), _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaPipelineModuleMock->attachSourceResponse(kVideoSourceId)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn))));
}

void MediaPipelineTestMethods::attachSourceVideo()
{
    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceVideo>(kMimeType.c_str(), kHasNoDrm, kWidthUhd, kHeightUhd, kAlignment, kStreamFormat, kCodecData);
    EXPECT_EQ(m_mediaPipeline->attachSource(mediaSource), true);
}

void MediaPipelineTestMethods::shouldAttachAudioSource()
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, attachSource(_, attachSourceRequestMatcherAudio(kSessionId,
                                                           kMimeType.c_str(), kHasNoDrm, kAlignment, kNumberOfChannels, kSampleRate, kCodecSpecificConfigStr, kCodecData, convertStreamFormat(kStreamFormat)), _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaPipelineModuleMock->attachSourceResponse(kAudioSourceId)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn))));
}

void MediaPipelineTestMethods::attachSourceAudio()
{
    std::vector<uint8_t> codecSpecificConfig;
    codecSpecificConfig.assign(kCodecSpecificConfigStr.begin(), kCodecSpecificConfigStr.end());
    AudioConfig audioConfig{kNumberOfChannels, kSampleRate, codecSpecificConfig};

    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceAudio>(kMimeType.c_str(), kHasNoDrm, audioConfig, kAlignment, kStreamFormat, kCodecData);
    EXPECT_EQ(m_mediaPipeline->attachSource(mediaSource), true);
}

void MediaPipelineTestMethods::shouldAllSourcesAttached()
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, allSourcesAttached(_, allSourcesAttachedRequestMatcher(kSessionId), _, _))
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
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyNeedMediaData(kAudioSourceId, kFrameCountBeforePreroll, m_needDataRequestId, kNullShmInfo))
        .WillOnce(InvokeWithoutArgs([&]()
            {
                // Set the firstSegment flag
                m_firstSegmentOfNeedData = true;
                notifyEvent();
            }));
}

void MediaPipelineTestMethods::sendNotifyNeedDataAudioBeforePreroll()
{
    getServerStub()->notifyNeedMediaDataEvent(kSessionId, kAudioSourceId, kFrameCountBeforePreroll, m_needDataRequestId, m_locationToWriteAudio);
    waitEvent();
}

void MediaPipelineTestMethods::shouldNotifyNeedDataVideoBeforePreroll()
{
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyNeedMediaData(kVideoSourceId, kFrameCountBeforePreroll, m_needDataRequestId, kNullShmInfo))
        .WillOnce(InvokeWithoutArgs([&]()
            {
                // Set the firstSegment flag
                m_firstSegmentOfNeedData = true;
                notifyEvent();
            }));
}

void MediaPipelineTestMethods::sendNotifyNeedDataVideoBeforePreroll()
{
    getServerStub()->notifyNeedMediaDataEvent(kSessionId, kVideoSourceId, kFrameCountBeforePreroll, m_needDataRequestId, m_locationToWriteVideo);
    waitEvent();
}

void MediaPipelineTestMethods::shouldHaveDataBeforePreroll()
{
    shouldHaveDataOk(kFrameCountBeforePreroll);
}

void MediaPipelineTestMethods::haveDataOk()
{
    EXPECT_EQ(m_mediaPipeline->haveData(MediaSourceStatus::OK, m_needDataRequestId), true);
}

int32_t MediaPipelineTestMethods::addSegmentMseAudio()
{
    int64_t timestamp = kTimeStamp + 1000 * (m_audioSegmentCount + 1);
    std::unique_ptr<IMediaPipeline::MediaSegment> mseData =
        std::make_unique<IMediaPipeline::MediaSegmentAudio>(kAudioSourceId, timestamp, kDuration, kSampleRate, kNumberOfChannels);
    mseData->setData(kAudioSegments[m_audioSegmentCount].size(), (const uint8_t*)kAudioSegments[m_audioSegmentCount].c_str());
    EXPECT_EQ(m_mediaPipeline->addSegment(m_needDataRequestId, mseData), AddSegmentStatus::OK);

    // Store where the segment should be written so we can check the data
    int32_t segmentId = m_audioSegmentCount;
    writtenAudioSegments.insert({segmentId, *m_locationToWriteAudio});

    // Calibrate the shm info based on segment written
    uint32_t metaDataSize = 0;
    if (m_firstSegmentOfNeedData)
    {
        // For first segment we write the metadata version
        // In V2 we dont set anymore data in the metadata buffer
        metaDataSize += 4;
        m_firstSegmentOfNeedData = false;
    }
    uint32_t segmentDataSize = kAudioSegments[m_audioSegmentCount].size() + 19 + 4; // Not sure why 4 works
    m_locationToWriteAudio->maxMetadataBytes = m_locationToWriteAudio->maxMetadataBytes - metaDataSize;
    m_locationToWriteAudio->metadataOffset = m_locationToWriteAudio->metadataOffset + metaDataSize;
    m_locationToWriteAudio->maxMediaBytes = m_locationToWriteAudio->maxMediaBytes - segmentDataSize;
    m_locationToWriteAudio->mediaDataOffset = m_locationToWriteAudio->mediaDataOffset + segmentDataSize;

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

    EXPECT_TRUE(metadata.has_length());
    EXPECT_EQ(metadata.length(), kAudioSegments[segmentId].size());
    EXPECT_TRUE(metadata.has_time_position());
    EXPECT_EQ(metadata.time_position(), kTimeStamp + 1000 * (segmentId + 1));
    EXPECT_TRUE(metadata.has_sample_duration());
    EXPECT_EQ(metadata.sample_duration(), kDuration);
    EXPECT_TRUE(metadata.has_stream_id());
    EXPECT_EQ(metadata.stream_id(), kAudioSourceId);
    EXPECT_TRUE(metadata.has_sample_rate());
    EXPECT_EQ(metadata.sample_rate(), kSampleRate);
    EXPECT_TRUE(metadata.channels_num());
    EXPECT_EQ(metadata.channels_num(), kNumberOfChannels);

    EXPECT_FALSE(metadata.has_width());
    EXPECT_FALSE(metadata.has_height());
    EXPECT_FALSE(metadata.has_segment_alignment());
    EXPECT_FALSE(metadata.has_extra_data());
    EXPECT_FALSE(metadata.has_media_key_session_id());
    EXPECT_FALSE(metadata.has_key_id());
    EXPECT_FALSE(metadata.has_init_vector());
    EXPECT_FALSE(metadata.has_init_with_last_15());
    EXPECT_EQ(metadata.sub_sample_info().size(), 0);
    EXPECT_FALSE(metadata.has_codec_data());
    EXPECT_FALSE(metadata.has_cipher_mode());
    EXPECT_FALSE(metadata.has_crypt());
    EXPECT_FALSE(metadata.has_skip());
    EXPECT_FALSE(metadata.has_frame_rate());

    EXPECT_TRUE(metadata.has_length());
    dataPosition += *metadataSize;
    std::string data = std::string(reinterpret_cast<char*>(dataPosition), metadata.length());
    EXPECT_EQ(data, kAudioSegments[segmentId]);
}

int32_t MediaPipelineTestMethods::addSegmentMseVideo()
{
    int64_t timestamp = kTimeStamp + 1000 * (m_videoSegmentCount + 1);
    std::unique_ptr<IMediaPipeline::MediaSegment> mseData =
        std::make_unique<IMediaPipeline::MediaSegmentVideo>(kVideoSourceId, timestamp, kDuration, kWidthUhd, kHeightUhd, kFrameRate);
    mseData->setData(kVideoSegments[m_videoSegmentCount].size(), (const uint8_t*)kVideoSegments[m_videoSegmentCount].c_str());
    EXPECT_EQ(m_mediaPipeline->addSegment(m_needDataRequestId, mseData), AddSegmentStatus::OK);

    // Store where the segment should be written so we can check the data
    int32_t segmentId = m_videoSegmentCount;
    writtenVideoSegments.insert({segmentId, *m_locationToWriteVideo});

    // Calibrate the shm info based on segment written
    uint32_t metaDataSize = 0;
    if (m_firstSegmentOfNeedData)
    {
        // For first segment we write the metadata version
        // In V2 we dont set anymore data in the metadata buffer
        metaDataSize += 4;
        m_firstSegmentOfNeedData = false;
    }
    uint32_t segmentDataSize = kVideoSegments[m_videoSegmentCount].size() + 26 + 4; // Not sure why 4 works
    m_locationToWriteVideo->maxMetadataBytes = m_locationToWriteVideo->maxMetadataBytes - metaDataSize;
    m_locationToWriteVideo->metadataOffset = m_locationToWriteVideo->metadataOffset + metaDataSize;
    m_locationToWriteVideo->maxMediaBytes = m_locationToWriteVideo->maxMediaBytes - segmentDataSize;
    m_locationToWriteVideo->mediaDataOffset = m_locationToWriteVideo->mediaDataOffset + segmentDataSize;

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

    EXPECT_TRUE(metadata.has_length());
    EXPECT_EQ(metadata.length(), kVideoSegments[segmentId].size());
    EXPECT_TRUE(metadata.has_time_position());
    EXPECT_EQ(metadata.time_position(), kTimeStamp + 1000 * (segmentId + 1));
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

    EXPECT_FALSE(metadata.has_sample_rate());
    EXPECT_FALSE(metadata.channels_num());
    EXPECT_FALSE(metadata.has_segment_alignment());
    EXPECT_FALSE(metadata.has_extra_data());
    EXPECT_FALSE(metadata.has_media_key_session_id());
    EXPECT_FALSE(metadata.has_key_id());
    EXPECT_FALSE(metadata.has_init_vector());
    EXPECT_FALSE(metadata.has_init_with_last_15());
    EXPECT_EQ(metadata.sub_sample_info().size(), 0);
    EXPECT_FALSE(metadata.has_codec_data());
    EXPECT_FALSE(metadata.has_cipher_mode());
    EXPECT_FALSE(metadata.has_crypt());
    EXPECT_FALSE(metadata.has_skip());

    EXPECT_TRUE(metadata.has_length());
    dataPosition += *metadataSize;
    std::string data = std::string(reinterpret_cast<char*>(dataPosition), metadata.length());
    EXPECT_EQ(data, kVideoSegments[segmentId]);
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

void MediaPipelineTestMethods::shouldNotifyPlaybackStatePlay()
{
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyPlaybackState(PlaybackState::PLAYING))
        .WillOnce(Invoke(this, &MediaPipelineTestMethods::notifyEvent));
}

void MediaPipelineTestMethods::sendNotifyPlaybackStatePlay()
{
    getServerStub()->notifyPlaybackStateChangeEvent(kSessionId, PlaybackState::PLAYING);
    waitEvent();
}

void MediaPipelineTestMethods::shouldPlay()
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, play(_, playRequestMatcher(kSessionId), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn)));
}

void MediaPipelineTestMethods::play()
{
    EXPECT_EQ(m_mediaPipeline->play(), true);
}

void MediaPipelineTestMethods::shouldNotifyNeedDataAudioAfterPreroll()
{
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyNeedMediaData(kAudioSourceId, kMaxFrameCount, m_needDataRequestId, kNullShmInfo))
        .WillOnce(InvokeWithoutArgs([&]()
            {
                // Set the firstSegment flag
                m_firstSegmentOfNeedData = true;
                notifyEvent();
            }));
}

void MediaPipelineTestMethods::sendNotifyNeedDataAudioAfterPreroll()
{
    getServerStub()->notifyNeedMediaDataEvent(kSessionId, kAudioSourceId, kMaxFrameCount, m_needDataRequestId, m_locationToWriteAudio);
    waitEvent();
}

void MediaPipelineTestMethods::shouldNotifyNeedDataVideoAfterPreroll()
{
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyNeedMediaData(kVideoSourceId, kMaxFrameCount, m_needDataRequestId, kNullShmInfo))
        .WillOnce(InvokeWithoutArgs([&]()
            {
                // Set the firstSegment flag
                m_firstSegmentOfNeedData = true;
                notifyEvent();
            }));
}

void MediaPipelineTestMethods::sendNotifyNeedDataVideoAfterPreroll()
{
    getServerStub()->notifyNeedMediaDataEvent(kSessionId, kVideoSourceId, kMaxFrameCount, m_needDataRequestId, m_locationToWriteVideo);
    waitEvent();
}

void MediaPipelineTestMethods::shouldHaveDataAfterPreroll()
{
    shouldHaveDataOk(kMaxFrameCount);
}

void MediaPipelineTestMethods::shouldHaveDataOk(size_t framesWritten)
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, haveData(_, haveDataRequestMatcher(kSessionId, convertMediaSourceStatus(MediaSourceStatus::OK), framesWritten, m_needDataRequestId), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(
            [&](::google::protobuf::RpcController *controller, ::google::protobuf::Closure *done)
            {
                // Increment needData request Id
                m_needDataRequestId++;
                m_mediaPipelineModuleMock->defaultReturn(controller, done);
                resetWriteLocation(m_kAudioShmInfo, m_kVideoShmInfo);
            })));
}

void MediaPipelineTestMethods::shouldHaveDataEos(size_t framesWritten)
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, haveData(_, haveDataRequestMatcher(kSessionId, convertMediaSourceStatus(MediaSourceStatus::EOS), framesWritten, m_needDataRequestId), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(
            [&](::google::protobuf::RpcController *controller, ::google::protobuf::Closure *done)
            {
                // Increment needData request Id
                m_needDataRequestId++;
                m_mediaPipelineModuleMock->defaultReturn(controller, done);
                resetWriteLocation(m_kAudioShmInfo, m_kVideoShmInfo);
            })));
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
    EXPECT_CALL(*m_mediaPipelineModuleMock, removeSource(_, removeSourceRequestMatcher(kSessionId, kVideoSourceId), _, _))
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
    EXPECT_CALL(*m_mediaPipelineModuleMock, stop(_, stopRequestMatcher(kSessionId), _, _))
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
    EXPECT_CALL(*m_mediaPipelineModuleMock, destroySession(_, destroySessionRequestMatcher(kSessionId), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn)));
}

void MediaPipelineTestMethods::destroyMediaPipeline()
{
    m_mediaPipeline.reset();
}
