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

#ifndef FIREBOLT_RIALTO_MEDIA_PIPELINE_TEST_METHODS_H_
#define FIREBOLT_RIALTO_MEDIA_PIPELINE_TEST_METHODS_H_

#include "IMediaPipeline.h"
#include "MediaPipelineClientMock.h"
#include "MediaPipelineModuleMock.h"
#include "ServerStub.h"
#include <gtest/gtest.h>
#include <map>
#include <memory>
#include <string>

using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::InvokeWithoutArgs;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::StrictMock;
using ::testing::WithArgs;

using namespace firebolt::rialto;
using namespace firebolt::rialto::ct::stub;

// Forward declare metadata so we dont have to include proto file
namespace firebolt::rialto
{
class MediaSegmentMetadata;
};

class MediaPipelineTestMethods
{
public:
    MediaPipelineTestMethods(const MediaPlayerShmInfo &audioShmInfo, const MediaPlayerShmInfo &videoShmInfo);
    virtual ~MediaPipelineTestMethods();

protected:
    // Strict Mocks
    std::shared_ptr<StrictMock<MediaPipelineClientMock>> m_mediaPipelineClientMock;
    std::shared_ptr<StrictMock<MediaPipelineModuleMock>> m_mediaPipelineModuleMock;

    // Objects
    std::shared_ptr<IMediaPipelineFactory> m_mediaPipelineFactory;
    std::unique_ptr<IMediaPipeline> m_mediaPipeline;

    // Expect methods
    void shouldCreateMediaSession();
    void shouldLoad();
    void shouldNotifyNetworkStateBuffering();
    void shouldPause();
    void shouldAttachVideoSource();
    void shouldAttachAudioSource();
    void shouldAllSourcesAttached();
    void shouldNotifyPlaybackStateIdle();
    void shouldNotifyNeedDataAudioBeforePreroll();
    void shouldNotifyNeedDataVideoBeforePreroll();
    void shouldHaveDataBeforePreroll();
    void shouldNotifyNetworkStateBuffered();
    void shouldNotifyPlaybackStatePaused();
    void shouldNotifyPlaybackStatePlay();
    void shouldPlay();
    void shouldNotifyNeedDataAudioAfterPreroll();
    void shouldNotifyNeedDataVideoAfterPreroll();
    void shouldHaveDataAfterPreroll();
    void shouldHaveDataOk(size_t framesWritten);
    void shouldHaveDataEos(size_t framesWritten);
    void shouldNotifyPlaybackStateEndOfStream();
    void shouldRemoveVideoSource();
    void shouldRemoveAudioSource();
    void shouldStop();
    void shouldNotifyPlaybackStateStopped();
    void shouldDestroyMediaSession();

    // Api methods
    void createMediaPipeline();
    void load();
    void pause();
    void attachSourceVideo();
    void attachSourceAudio();
    void allSourcesAttached();
    int32_t addSegmentMseAudio();
    int32_t addSegmentMseVideo();
    void haveDataOk();
    void play();
    void haveDataEos();
    void removeSourceVideo();
    void removeSourceAudio();
    void stop();
    void destroyMediaPipeline();

    // Event methods
    void sendNotifyNetworkStateBuffering();
    void sendNotifyPlaybackStateIdle();
    void sendNotifyNeedDataAudioBeforePreroll();
    void sendNotifyNeedDataVideoBeforePreroll();
    void sendNotifyNetworkStateBuffered();
    void sendNotifyPlaybackStatePaused();
    void sendNotifyPlaybackStatePlay();
    void sendNotifyNeedDataAudioAfterPreroll();
    void sendNotifyNeedDataVideoAfterPreroll();
    void sendNotifyPlaybackStateEndOfStream();
    void sendNotifyPlaybackStateStopped();

    // Check methods
    void checkMseAudioSegmentWritten(int32_t segmentId);
    void checkMseVideoSegmentWritten(int32_t segmentId);

    // Helper methods
    void startAudioVideoMediaSessionWaitForPreroll();
    void endAudioVideoMediaSession();

    virtual void notifyEvent() = 0;
    virtual void waitEvent() = 0;
    virtual std::shared_ptr<ServerStub> &getServerStub() = 0;
    virtual void *getShmAddress() = 0;

private:
    // Const variables
    const MediaPlayerShmInfo &m_kAudioShmInfo;
    const MediaPlayerShmInfo &m_kVideoShmInfo;

    // None const variables
    uint32_t m_needDataRequestId{0};
    std::shared_ptr<MediaPlayerShmInfo> m_locationToWriteAudio{std::make_shared<MediaPlayerShmInfo>()};
    std::shared_ptr<MediaPlayerShmInfo> m_locationToWriteVideo{std::make_shared<MediaPlayerShmInfo>()};
    uint32_t m_audioSegmentCount{0};
    uint32_t m_videoSegmentCount{0};
    std::map<int32_t, MediaPlayerShmInfo> writtenAudioSegments;
    std::map<int32_t, MediaPlayerShmInfo> writtenVideoSegments;
    bool m_firstSegmentOfNeedData{false};

    void resetWriteLocation(const MediaPlayerShmInfo &audioShmInfo, const MediaPlayerShmInfo &videoShmInfo);
    uint32_t getTimestamp(uint32_t segmentId);
    void incrementWriteLocation(uint32_t sizeOfSegmentData, const std::shared_ptr<MediaPlayerShmInfo> &writeLocation);
    void checkAudioMetadata(const MediaSegmentMetadata &metadata, uint32_t segmentId);
    void checkHasNoAudioMetadata(const MediaSegmentMetadata &metadata);
    void checkVideoMetadata(const MediaSegmentMetadata &metadata, uint32_t segmentId);
    void checkHasNoVideoMetadata(const MediaSegmentMetadata &metadata);
    void checkHasNoEncryptionMetadata(const MediaSegmentMetadata &metadata);
    void checkHasNoCodacData(const MediaSegmentMetadata &metadata);
    void checkHasNoSegmentAlignment(const MediaSegmentMetadata &metadata);
    void checkHasNoExtraData(const MediaSegmentMetadata &metadata);
    void checkSegmentData(const MediaSegmentMetadata &metadata, uint8_t *dataPtr, const std::string &expectedSegmentData);
};

#endif // FIREBOLT_RIALTO_MEDIA_PIPELINE_TEST_METHODS_H_
