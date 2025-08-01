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

#ifndef MEDIA_PIPELINE_SERVICE_TESTS_FIXTURE_H_
#define MEDIA_PIPELINE_SERVICE_TESTS_FIXTURE_H_

#include "DecryptionServiceMock.h"
#include "HeartbeatProcedureMock.h"
#include "MediaPipelineCapabilitiesFactoryMock.h"
#include "MediaPipelineCapabilitiesMock.h"
#include "MediaPipelineServerInternalFactoryMock.h"
#include "MediaPipelineServerInternalMock.h"
#include "MediaPipelineService.h"
#include "PlaybackServiceMock.h"
#include "SharedMemoryBufferMock.h"
#include <gtest/gtest.h>
#include <memory>

using testing::StrictMock;

class MediaPipelineServiceTests : public testing::Test
{
public:
    MediaPipelineServiceTests();
    ~MediaPipelineServiceTests() = default;

    void mediaPipelineWillLoad();
    void mediaPipelineWillFailToLoad();
    void mediaPipelineWillAttachSource();
    void mediaPipelineWillFailToAttachSource();
    void mediaPipelineWillRemoveSource();
    void mediaPipelineWillFailToRemoveSource();
    void mediaPipelineWillAllSourcesAttached();
    void mediaPipelineWillFailToAllSourcesAttached();
    void mediaPipelineWillPlay();
    void mediaPipelineWillFailToPlay();
    void mediaPipelineWillPause();
    void mediaPipelineWillFailToPause();
    void mediaPipelineWillStop();
    void mediaPipelineWillFailToStop();
    void mediaPipelineWillSetPlaybackRate();
    void mediaPipelineWillFailToSetPlaybackRate();
    void mediaPipelineWillSetPosition();
    void mediaPipelineWillFailToSetPosition();
    void mediaPipelineWillSetVideoWindow();
    void mediaPipelineWillFailToSetVideoWindow();
    void mediaPipelineWillHaveData();
    void mediaPipelineWillFailToHaveData();
    void mediaPipelineWillGetPosition();
    void mediaPipelineWillFailToGetPosition();
    void mediaPipelineWillSetImmediateOutput();
    void mediaPipelineWillFailToSetImmediateOutput();
    void mediaPipelineWillGetImmediateOutput();
    void mediaPipelineWillFailToGetImmediateOutput();
    void mediaPipelineWillGetStats();
    void mediaPipelineWillFailToGetStats();
    void mediaPipelineWillRenderFrame();
    void mediaPipelineWillFailToRenderFrame();
    void mediaPipelineWillSetVolume();
    void mediaPipelineWillFailToSetVolume();
    void mediaPipelineWillGetVolume();
    void mediaPipelineWillFailToGetVolume();
    void mediaPipelineWillSetMute();
    void mediaPipelineWillFailToSetMute();
    void mediaPipelineWillGetMute();
    void mediaPipelineWillFailToGetMute();
    void mediaPipelineWillSetLowLatency();
    void mediaPipelineWillFailToSetLowLatency();
    void mediaPipelineWillSetSync();
    void mediaPipelineWillFailToSetSync();
    void mediaPipelineWillGetSync();
    void mediaPipelineWillFailToGetSync();
    void mediaPipelineWillSetSyncOff();
    void mediaPipelineWillFailToSetSyncOff();
    void mediaPipelineWillSetStreamSyncMode();
    void mediaPipelineWillFailToSetStreamSyncMode();
    void mediaPipelineWillGetStreamSyncMode();
    void mediaPipelineWillFailToGetStreamSyncMode();
    void mediaPipelineWillFlush();
    void mediaPipelineWillFailToFlush();
    void mediaPipelineWillSetSourcePosition();
    void mediaPipelineWillFailToSetSourcePosition();
    void mediaPipelineWillProcessAudioGap();
    void mediaPipelineWillFailToProcessAudioGap();
    void mediaPipelineWillSetTextTrackIdentifier();
    void mediaPipelineWillFailToSetTextTrackIdentifier();
    void mediaPipelineWillGetTextTrackIdentifier();
    void mediaPipelineWillFailToGetTextTrackIdentifier();
    void mediaPipelineWillSetBufferingLimit();
    void mediaPipelineWillFailToSetBufferingLimit();
    void mediaPipelineWillGetBufferingLimit();
    void mediaPipelineWillFailToGetBufferingLimit();
    void mediaPipelineWillSetUseBuffering();
    void mediaPipelineWillFailToSetUseBuffering();
    void mediaPipelineWillGetUseBuffering();
    void mediaPipelineWillFailToGetUseBuffering();
    void mediaPipelineWillSwitchSource();
    void mediaPipelineWillFailToSwitchSource();
    void mediaPipelineWillCheckIfVideoIsMaster();
    void mediaPipelineWillFailToCheckIfVideoIsMaster();

    void mediaPipelineWillPing();

    void mediaPipelineFactoryWillCreateMediaPipeline();
    void mediaPipelineFactoryWillReturnNullptr();

    void playbackServiceWillReturnActive();
    void playbackServiceWillReturnInactive();
    void playbackServiceWillReturnMaxPlaybacks(int maxPlaybacks);
    void playbackServiceWillReturnSharedMemoryBuffer();

    void createMediaPipelineShouldSuccess();
    void createMediaPipelineShouldFailWhenMediaPipelineCapabilitiesFactoryReturnsNullptr();

    void createSessionShouldSucceed();
    void createSessionShouldFail();
    void destroySessionShouldSucceed();
    void destroySessionShouldFail();
    void loadShouldSucceed();
    void loadShouldFail();
    void attachSourceShouldSucceed();
    void attachSourceShouldFail();
    void removeSourceShouldSucceed();
    void removeSourceShouldFail();
    void allSourcesAttachedShouldSucceed();
    void allSourcesAttachedShouldFail();
    void playShouldSucceed();
    void playShouldFail();
    void pauseShouldSucceed();
    void pauseShouldFail();
    void stopShouldSucceed();
    void stopShouldFail();
    void setPlaybackRateShouldSucceed();
    void setPlaybackRateShouldFail();
    void setPositionShouldSucceed();
    void setPositionShouldFail();
    void setVideoWindowShouldSucceed();
    void setVideoWindowShouldFail();
    void haveDataShouldSucceed();
    void haveDataShouldFail();
    void getPositionShouldSucceed();
    void getPositionShouldFail();
    void setImmediateOutputShouldSucceed();
    void setImmediateOutputShouldFail();
    void getImmediateOutputShouldSucceed();
    void getImmediateOutputShouldFail();
    void getStatsShouldSucceed();
    void getStatsShouldFail();
    void getSupportedMimeTypesSucceed();
    void isMimeTypeSupportedSucceed();
    void getSupportedPropertiesSucceed();
    void renderFrameShouldSucceed();
    void renderFrameShouldFail();
    void setVolumeShouldSucceed();
    void setVolumeShouldFail();
    void getVolumeShouldSucceed();
    void getVolumeShouldFail();
    void setMuteShouldSucceed();
    void setMuteShouldFail();
    void getMuteShouldSucceed();
    void getMuteShouldFail();
    void setLowLatencyShouldSucceed();
    void setLowLatencyShouldFail();
    void setSyncShouldSucceed();
    void setSyncShouldFail();
    void getSyncShouldSucceed();
    void getSyncShouldFail();
    void setSyncOffShouldSucceed();
    void setSyncOffShouldFail();
    void setStreamSyncModeShouldSucceed();
    void setStreamSyncModeShouldFail();
    void getStreamSyncModeShouldSucceed();
    void getStreamSyncModeShouldFail();
    void flushShouldSucceed();
    void flushShouldFail();
    void setSourcePositionShouldSucceed();
    void setSourcePositionShouldFail();
    void processAudioGapShouldSucceed();
    void processAudioGapShouldFail();
    void setTextTrackIdentifierShouldSucceed();
    void setTextTrackIdentifierShouldFail();
    void getTextTrackIdentifierShouldSucceed();
    void getTextTrackIdentifierShouldFail();
    void setBufferingLimitShouldSucceed();
    void setBufferingLimitShouldFail();
    void getBufferingLimitShouldSucceed();
    void getBufferingLimitShouldFail();
    void setUseBufferingShouldSucceed();
    void setUseBufferingShouldFail();
    void getUseBufferingShouldSucceed();
    void getUseBufferingShouldFail();
    void switchSourceShouldSucceed();
    void switchSourceShouldFail();
    void isVideoMasterShouldSucceed();
    void isVideoMasterShouldFail();

    void clearMediaPipelines();
    void initSession();
    void triggerPing();

private:
    std::shared_ptr<StrictMock<firebolt::rialto::server::MediaPipelineServerInternalFactoryMock>> m_mediaPipelineFactoryMock;
    std::shared_ptr<StrictMock<firebolt::rialto::server::MediaPipelineCapabilitiesFactoryMock>>
        m_mediaPipelineCapabilitiesFactoryMock;
    std::unique_ptr<StrictMock<firebolt::rialto::server::MediaPipelineCapabilitiesMock>> m_mediaPipelineCapabilities;
    StrictMock<firebolt::rialto::server::MediaPipelineCapabilitiesMock> &m_mediaPipelineCapabilitiesMock;
    std::shared_ptr<firebolt::rialto::server::ISharedMemoryBuffer> m_shmBuffer;
    StrictMock<firebolt::rialto::server::SharedMemoryBufferMock> &m_shmBufferMock;
    std::unique_ptr<firebolt::rialto::server::IMediaPipelineServerInternal> m_mediaPipeline;
    StrictMock<firebolt::rialto::server::MediaPipelineServerInternalMock> &m_mediaPipelineMock;
    StrictMock<firebolt::rialto::server::DecryptionServiceMock> m_decryptionServiceMock;
    StrictMock<firebolt::rialto::server::service::PlaybackServiceMock> m_playbackServiceMock;
    std::shared_ptr<StrictMock<firebolt::rialto::server::HeartbeatProcedureMock>> m_heartbeatProcedureMock;
    std::unique_ptr<firebolt::rialto::server::service::MediaPipelineService> m_sut;
};

#endif // MEDIA_PIPELINE_SERVICE_TESTS_FIXTURE_H_
