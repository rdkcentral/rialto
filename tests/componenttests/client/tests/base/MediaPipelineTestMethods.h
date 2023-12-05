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

#ifndef MEDIA_PIPELINE_TEST_METHODS_H_
#define MEDIA_PIPELINE_TEST_METHODS_H_

#include "IMediaPipeline.h"
#include "MediaPipelineClientMock.h"
#include "MediaPipelineModuleMock.h"
#include "ServerStub.h"
#include <gtest/gtest.h>
#include <map>
#include <memory>

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

    // Test methods
    void shouldCreateMediaSession();
    void createMediaPipeline();
    void shouldLoad();
    void load();
    void shouldNotifyNetworkStateBuffering();
    void sendNotifyNetworkStateBuffering();
    void shouldPause();
    void pause();
    void shouldAttachVideoSource();
    void attachSourceVideo();
    void shouldAttachAudioSource();
    void attachSourceAudio();
    void shouldAllSourcesAttached();
    void allSourcesAttached();
    void shouldNotifyPlaybackStateIdle();
    void sendNotifyPlaybackStateIdle();
    void shouldNotifyNeedDataAudioBeforePreroll();
    void sendNotifyNeedDataAudioBeforePreroll();
    void shouldNotifyNeedDataVideoBeforePreroll();
    void sendNotifyNeedDataVideoBeforePreroll();
    void shouldHaveDataBeforePreroll();
    void haveDataOk();
    int32_t addSegmentMseAudio();
    void checkMseAudioSegmentWritten(int32_t segmentId);
    int32_t addSegmentMseVideo();
    void checkMseVideoSegmentWritten(int32_t segmentId);
    void shouldNotifyNetworkStateBuffered();
    void sendNotifyNetworkStateBuffered();
    void shouldNotifyPlaybackStatePaused();
    void sendNotifyPlaybackStatePaused();
    void shouldNotifyPlaybackStatePlay();
    void sendNotifyPlaybackStatePlay();
    void shouldPlay();
    void play();
    void shouldNotifyNeedDataAudioAfterPreroll();
    void sendNotifyNeedDataAudioAfterPreroll();
    void shouldNotifyNeedDataVideoAfterPreroll();
    void sendNotifyNeedDataVideoAfterPreroll();
    void shouldHaveDataAfterPreroll();
    void shouldHaveDataOk(size_t framesWritten);
    void shouldHaveDataEos(size_t framesWritten);
    void haveDataEos();
    void shouldNotifyPlaybackStateEndOfStream();
    void sendNotifyPlaybackStateEndOfStream();
    void shouldRemoveVideoSource();
    void removeSourceVideo();
    void shouldRemoveAudioSource();
    void removeSourceAudio();
    void shouldStop();
    void stop();
    void shouldNotifyPlaybackStateStopped();
    void sendNotifyPlaybackStateStopped();
    void shouldDestroyMediaSession();
    void destroyMediaPipeline();

    void startAudioVideoMediaSessionWaitForPreroll();
    void endAudioVideoMediaSession();
    
    // Component test helpers
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
};

#endif // MEDIA_PIPELINE_TEST_METHODS_H_
