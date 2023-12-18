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

#ifndef FIREBOLT_RIALTO_CLIENT_CT_MEDIA_PIPELINE_TEST_METHODS_H_
#define FIREBOLT_RIALTO_CLIENT_CT_MEDIA_PIPELINE_TEST_METHODS_H_

#include "IMediaPipeline.h"
#include "MediaPipelineClientMock.h"
#include "MediaPipelineModuleMock.h"
#include "ServerStub.h"
#include <gtest/gtest.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::InvokeWithoutArgs;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::StrictMock;
using ::testing::WithArgs;

using namespace firebolt::rialto;

// Forward declare metadata so we dont have to include proto file
namespace firebolt::rialto
{
class MediaSegmentMetadata;
};

namespace firebolt::rialto::client::ct
{
class MediaPipelineTestMethods
{
public:
    MediaPipelineTestMethods(const std::vector<firebolt::rialto::MediaPlayerShmInfo> &audioShmInfo,
                             const std::vector<firebolt::rialto::MediaPlayerShmInfo> &videoShmInfo);
    virtual ~MediaPipelineTestMethods();

protected:
    // Strict Mocks
    std::shared_ptr<StrictMock<MediaPipelineClientMock>> m_mediaPipelineClientMock;
    std::shared_ptr<StrictMock<MediaPipelineModuleMock>> m_mediaPipelineModuleMock;
    std::shared_ptr<StrictMock<MediaPipelineClientMock>> m_mediaPipelineClientSecondaryMock;

    // Objects
    std::shared_ptr<IMediaPipelineFactory> m_mediaPipelineFactory;
    std::unique_ptr<IMediaPipeline> m_mediaPipeline;
    std::unique_ptr<IMediaPipeline> m_mediaPipelineSecondary;

    // MediaPipeline Expect methods
    void shouldCreateMediaSession();
    void shouldCreateMediaSessionSecondary();
    void shouldLoad();
    void shouldLoadSecondary();
    void shouldPause();
    void shouldAttachVideoSource();
    void shouldAttachVideoSourceSecondary();
    void shouldAttachAudioSource();
    void shouldAttachAudioSourceMpeg();
    void shouldAttachAudioSourceMp4();
    void shouldAttachAudioSourceEacs();
    void shouldAllSourcesAttached();
    void shouldAllSourcesAttachedSecondary();
    void shouldPlay();
    void shouldPlaySecondary();
    void shouldHaveDataAfterPreroll();
    void shouldHaveDataBeforePreroll();
    void shouldHaveDataOk(size_t framesWritten);
    void shouldHaveDataEos(size_t framesWritten);
    void shouldHaveDataOkSecondary(size_t framesWritten);
    void shouldRemoveVideoSource();
    void shouldRemoveVideoSourceSecondary();
    void shouldRemoveAudioSource();
    void shouldStop();
    void shouldStopSecondary();
    void shouldDestroyMediaSession();
    void shouldDestroyMediaSessionSecondary();
    void shouldPlayWithFailure();
    void shouldPauseWithFailure();
    void shouldStopWithFailure();
    void shouldSetPlaybackRate2x();
    void shouldSetPlaybackRateNegative2x();
    void shouldSetPlaybackRateFailure();
    void shouldSetPositionTo10();
    void shouldSetPositionTo0();
    void shouldSetVolume(const double expectedVolume);
    void shouldGetVolume(const double volume);
    void shouldSetMute(const bool expectedMute);
    void shouldGetMute(const bool mute);
    void shouldSetVideoWindow(const uint32_t expectedX, const uint32_t expectedY, const uint32_t expectedWidth,
                              const uint32_t expectedHeight);
    void shouldRenderFrame();
    void shouldRenderFrameFailure();
    void shouldGetPosition(const int64_t position);

    // MediaPipelineClient Expect methods
    void shouldNotifyNetworkStateBuffering();
    void shouldNotifyNetworkStateBufferingSecondary();
    void shouldNotifyPlaybackStateIdle();
    void shouldNotifyPlaybackStateIdleSecondary();
    void shouldNotifyNeedDataAudioBeforePreroll();
    void shouldNotifyNeedDataVideoBeforePreroll();
    void shouldNotifyNetworkStateBuffered();
    void shouldNotifyNetworkStateBufferedSecondary();
    void shouldNotifyPlaybackStatePaused();
    void shouldNotifyPlaybackStatePausedSecondary();
    void shouldNotifyPlaybackStatePlaying();
    void shouldNotifyPlaybackStatePlayingSecondary();
    void shouldNotifyNeedDataAudioAfterPreroll();
    void shouldNotifyNeedDataVideoAfterPreroll();
    void shouldNotifyPlaybackStateEndOfStream();
    void shouldNotifyPlaybackStateStopped();
    void shouldNotifyPlaybackStateStoppedSecondary();
    void shouldNotifyPlaybackStateFailure();
    void shouldNotifyPlaybackStateSeeking();
    void shouldNotifyPlaybackStateFlushed();
    void shouldNotifyNeedDataAudio(const size_t framesToWrite);
    void shouldNotifyNeedDataVideo(const size_t framesToWrite);
    void shouldNotifyNeedDataVideoSecondary(const size_t framesToWrite);
    void shouldNotifyPosition(const uint32_t expectedPosition);

    // Api methods
    void createMediaPipeline();
    void createMediaPipelineSecondary();
    void load();
    void loadSecondary();
    void pause();
    void attachSourceVideo();
    void attachSourceAudio();
    void attachSourceAudioMpeg();
    void attachSourceAudioMp4();
    void attachSourceAudioEacs();
    void attachSourceVideoSecondary();
    void allSourcesAttached();
    void allSourcesAttachedSecondary();
    int32_t addSegmentMseAudio();
    int32_t addSegmentMseVideo();
    int32_t addSegmentMseVideoSecondary();
    void haveDataOk();
    void haveDataEos();
    void haveDataOkSecondary();
    void play();
    void playSecondary();
    void removeSourceVideo();
    void removeSourceAudio();
    void removeSourceVideoSecondary();
    void stop();
    void stopSecondary();
    void destroyMediaPipeline();
    void destroyMediaPipelineSecondary();
    void playFailure();
    void pauseFailure();
    void stopFailure();
    void setPlaybackRate2x();
    void setPlaybackRateNegative2x();
    void setPlaybackRateFailure();
    void setPosition10();
    void setPosition0();
    void setPositionFailure();
    void addSegmentFailure();
    void setVolume(const double volume);
    void getVolume(const double expectedVolume);
    void setMute(const bool mute);
    void getMute(const bool expectedMute);
    void setSetVideoWindow(const uint32_t x, const uint32_t y, const uint32_t width, const uint32_t height);
    void renderFrame();
    void renderFrameFailure();
    void getPosition(const int64_t expectedPosition);

    // Event methods
    void sendNotifyNetworkStateBuffering();
    void sendNotifyNetworkStateBufferingSecondary();
    void sendNotifyPlaybackStateIdle();
    void sendNotifyPlaybackStateIdleSecondary();
    void sendNotifyNeedDataAudioBeforePreroll();
    void sendNotifyNeedDataVideoBeforePreroll();
    void sendNotifyNeedDataAudioAfterPreroll();
    void sendNotifyNeedDataVideoAfterPreroll();
    void sendNotifyNeedDataVideo(uint32_t framesToWrite);
    void sendNotifyNeedDataAudio(uint32_t framesToWrite);
    void sendNotifyNeedDataVideoSecondary(uint32_t framesToWrite);
    void sendNotifyNetworkStateBuffered();
    void sendNotifyNetworkStateBufferedSecondary();
    void sendNotifyPlaybackStatePaused();
    void sendNotifyPlaybackStatePausedSecondary();
    void sendNotifyPlaybackStatePlaying();
    void sendNotifyPlaybackStatePlayingSecondary();
    void sendNotifyPlaybackStateEndOfStream();
    void sendNotifyPlaybackStateStopped();
    void sendNotifyPlaybackStateStoppedSecondary();
    void sendNotifyPlaybackStateFailure();
    void sendNotifyPlaybackStateSeeking();
    void sendNotifyPlaybackStateFlushed();
    void sendNotifyPositionChanged(const int64_t position);

    // Check methods
    void checkMseAudioSegmentWritten(int32_t segmentId);
    void checkMseVideoSegmentWritten(int32_t segmentId);
    void checkMseVideoSegmentWrittenSecondary(int32_t segmentId);

    // Helper methods
    void startAudioVideoMediaSessionWaitForPreroll();
    void startAudioVideoMediaSessionPrerollPaused();
    void endAudioVideoMediaSession();
    void writeAudioFrames();
    void writeVideoFrames();
    void writeAudioEos();
    void writeVideoEos();
    void writeVideoFramesSecondary();

    virtual void notifyEvent() = 0;
    virtual void waitEvent() = 0;
    virtual std::shared_ptr<ServerStub> &getServerStub() = 0;
    virtual void *getShmAddress() = 0;

private:
    // Const variables
    const std::vector<firebolt::rialto::MediaPlayerShmInfo> m_kAudioShmInfo;
    const std::vector<firebolt::rialto::MediaPlayerShmInfo> m_kVideoShmInfo;

    // None const variables
    uint32_t m_needDataRequestId{0};
    std::vector<std::shared_ptr<MediaPlayerShmInfo>> m_locationToWriteAudio;
    std::vector<std::shared_ptr<MediaPlayerShmInfo>> m_locationToWriteVideo;
    uint32_t m_audioSegmentCount{0};
    uint32_t m_videoSegmentCount{0};
    std::map<int32_t, MediaPlayerShmInfo> writtenAudioSegments;
    std::map<int32_t, MediaPlayerShmInfo> writtenVideoSegments;
    bool m_firstSegmentOfNeedData{false};

    void resetWriteLocation(uint32_t partitionId);
    uint32_t getTimestamp(uint32_t segmentId);
    void incrementWriteLocation(uint32_t sizeOfSegmentData, const std::shared_ptr<MediaPlayerShmInfo> &writeLocation);
    void checkAudioMetadata(const MediaSegmentMetadata &metadata, uint32_t segmentId);
    void checkHasNoAudioMetadata(const MediaSegmentMetadata &metadata);
    void checkVideoMetadata(const MediaSegmentMetadata &metadata, uint32_t segmentId);
    void checkVideoMetadataSecondary(const MediaSegmentMetadata &metadata, uint32_t segmentId);
    void checkHasNoVideoMetadata(const MediaSegmentMetadata &metadata);
    void checkHasNoEncryptionMetadata(const MediaSegmentMetadata &metadata);
    void checkHasNoCodacData(const MediaSegmentMetadata &metadata);
    void checkHasNoSegmentAlignment(const MediaSegmentMetadata &metadata);
    void checkHasNoExtraData(const MediaSegmentMetadata &metadata);
    void checkSegmentData(const MediaSegmentMetadata &metadata, uint8_t *dataPtr, const std::string &expectedSegmentData);
    void shouldCreateMediaSessionInternal(const int32_t sessionId, const VideoRequirements &videoRequirements);
    void shouldLoadInternal(const int32_t sessionId, const MediaType &mediaType, const std::string &mimeType,
                            const std::string &url);
    void shouldRemoveSourceInternal(const int32_t sessionId, const int32_t sourceId);
    void shouldStopInternal(const int32_t sessionId);
    void shouldPlayInternal(const int32_t sessionId);
    void shouldDestroyMediaSessionInternal(const int32_t sessionId);
    void shouldAttachVideoSourceInternal(const int32_t sessionId, const std::string &mimeType, bool hasNoDrm,
                                         const int32_t width, const int32_t height,
                                         const firebolt::rialto::SegmentAlignment &alignment,
                                         const std::shared_ptr<firebolt::rialto::CodecData> &codacData,
                                         const firebolt::rialto::StreamFormat &streamFormat);
    void shouldAttachAudioSourceInternal(const std::string &mimeType, bool hasNoDrm,
                                         const firebolt::rialto::SegmentAlignment &alignment, const uint32_t noOfChannels,
                                         const uint32_t sampleRate, const std::string &codecSpecificConfigStr,
                                         const std::shared_ptr<firebolt::rialto::CodecData> &codacData,
                                         const firebolt::rialto::StreamFormat &streamFormat);
    void shouldAllSourcesAttachedInternal(const int32_t sessionId);
    void shouldHaveDataInternal(const int32_t sessionId, const MediaSourceStatus status, const size_t framesWritten,
                                const uint32_t partition);
    void shouldNotifyPlaybackStateInternal(const std::shared_ptr<StrictMock<MediaPipelineClientMock>> &clientMock,
                                           const PlaybackState &state);
    void shouldNotifyNetworkStateInternal(const std::shared_ptr<StrictMock<MediaPipelineClientMock>> &clientMock,
                                          const NetworkState &state);
    void shouldNotifyNeedDataInternal(const std::shared_ptr<StrictMock<MediaPipelineClientMock>> &clientMock,
                                      const int32_t sourceId, const size_t framesToWrite);
    void createMediaPipelineInternal(std::unique_ptr<IMediaPipeline> &mediaPipeline,
                                     const std::shared_ptr<StrictMock<MediaPipelineClientMock>> &client,
                                     const VideoRequirements &videoRequirements);
    void loadInternal(const std::unique_ptr<IMediaPipeline> &mediaPipeline, const MediaType &mediaType,
                      const std::string &mimeType, const std::string &url, const bool status);
    void removeSourceInternal(const std::unique_ptr<IMediaPipeline> &mediaPipeline, const int32_t sourceId,
                              const bool status);
    void stopInternal(const std::unique_ptr<IMediaPipeline> &mediaPipeline, const bool status);
    void attachSourceVideoInternal(const std::unique_ptr<IMediaPipeline> &mediaPipeline, const std::string &mimeType,
                                   bool hasNoDrm, const int32_t width, const int32_t height,
                                   const firebolt::rialto::SegmentAlignment &alignment,
                                   const std::shared_ptr<firebolt::rialto::CodecData> &codacData,
                                   const firebolt::rialto::StreamFormat &streamFormat, const bool status);
    void attachSourceAudioInternal(const std::string &mimeType, bool hasNoDrm,
                                   const firebolt::rialto::SegmentAlignment &alignment, const uint32_t noOfChannels,
                                   const uint32_t sampleRate, const std::string &codecSpecificConfigStr,
                                   const std::shared_ptr<firebolt::rialto::CodecData> &codacData,
                                   const firebolt::rialto::StreamFormat &streamFormat, const bool status);
    void allSourcesAttachedInternal(const std::unique_ptr<IMediaPipeline> &mediaPipeline, const bool status);
    int32_t addSegmentMseVideoInternal(const std::unique_ptr<IMediaPipeline> &mediaPipeline, const int64_t duration,
                                       const int32_t width, const int32_t height, const Fraction &frameRate,
                                       const uint32_t partitionId, const AddSegmentStatus &status);
    void haveDataInternal(const std::unique_ptr<IMediaPipeline> &mediaPipeline, const MediaSourceStatus &mediaStatus,
                          const bool status);
    void playInternal(const std::unique_ptr<IMediaPipeline> &mediaPipeline, const bool status);
    void sendNotifyPlaybackStateInternal(const int32_t sessionId, const PlaybackState &state);
    void sendNotifyNetworkStateInternal(const int32_t sessionId, const NetworkState &state);
    void sendNotifyNeedDataInternal(const int32_t sessionId, const int32_t sourceId,
                                    const std::shared_ptr<MediaPlayerShmInfo> &location, uint32_t framesToWrite);
};
} // namespace firebolt::rialto::client::ct

#endif // FIREBOLT_RIALTO_CLIENT_CT_MEDIA_PIPELINE_TEST_METHODS_H_
