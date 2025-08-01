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

#include "MediaPipelineServiceTestsFixture.h"

TEST_F(MediaPipelineServiceTests, shouldFailToCreateMediaPipeline)
{
    createMediaPipelineShouldFailWhenMediaPipelineCapabilitiesFactoryReturnsNullptr();
}

TEST_F(MediaPipelineServiceTests, shouldFailToCreateSessionInInactiveState)
{
    createMediaPipelineShouldSuccess();
    playbackServiceWillReturnInactive();
    createSessionShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToCreateSessionWhenMaxPlaybackSessionsIsReached)
{
    createMediaPipelineShouldSuccess();
    playbackServiceWillReturnActive();
    playbackServiceWillReturnMaxPlaybacks(0);
    createSessionShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToCreateSessionWhenFactoryReturnsNull)
{
    createMediaPipelineShouldSuccess();
    playbackServiceWillReturnActive();
    playbackServiceWillReturnMaxPlaybacks(1);
    playbackServiceWillReturnSharedMemoryBuffer();
    mediaPipelineFactoryWillReturnNullptr();
    createSessionShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldCreateSession)
{
    initSession();
}

TEST_F(MediaPipelineServiceTests, shouldFailToCreateSessionWithTheSameIdTwice)
{
    initSession();
    playbackServiceWillReturnActive();
    playbackServiceWillReturnMaxPlaybacks(2);
    createSessionShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToDestroyNotExistingSession)
{
    createMediaPipelineShouldSuccess();
    destroySessionShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldDestroySession)
{
    initSession();
    destroySessionShouldSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldDestroySessionWhenClearingTheMediaPipeline)
{
    initSession();
    clearMediaPipelines();
    // Destroy session should fail because it has already been destroyed when clearing
    destroySessionShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToLoadNotExistingSession)
{
    createMediaPipelineShouldSuccess();
    loadShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToLoadSession)
{
    initSession();
    mediaPipelineWillFailToLoad();
    loadShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldLoadSession)
{
    initSession();
    mediaPipelineWillLoad();
    loadShouldSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldFailToAttachSourceForNotExistingSession)
{
    createMediaPipelineShouldSuccess();
    attachSourceShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToAttachSource)
{
    initSession();
    mediaPipelineWillFailToAttachSource();
    attachSourceShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldAttachSource)
{
    initSession();
    mediaPipelineWillAttachSource();
    attachSourceShouldSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldFailToRemoveSourceForNotExistingSession)
{
    createMediaPipelineShouldSuccess();
    removeSourceShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToRemoveSource)
{
    initSession();
    mediaPipelineWillFailToRemoveSource();
    removeSourceShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldRemoveSource)
{
    initSession();
    mediaPipelineWillRemoveSource();
    removeSourceShouldSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldFailToAllSourcesAttachedForNotExistingSession)
{
    createMediaPipelineShouldSuccess();
    allSourcesAttachedShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToAllSourcesAttached)
{
    initSession();
    mediaPipelineWillFailToAllSourcesAttached();
    allSourcesAttachedShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldAllSourcesAttached)
{
    initSession();
    mediaPipelineWillAllSourcesAttached();
    allSourcesAttachedShouldSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldFailToPlayForNotExistingSession)
{
    createMediaPipelineShouldSuccess();
    playShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToPlay)
{
    initSession();
    mediaPipelineWillFailToPlay();
    playShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldPlay)
{
    initSession();
    mediaPipelineWillPlay();
    playShouldSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldFailToStopForNotExistingSession)
{
    createMediaPipelineShouldSuccess();
    stopShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToStop)
{
    initSession();
    mediaPipelineWillFailToStop();
    stopShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldStop)
{
    initSession();
    mediaPipelineWillStop();
    stopShouldSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldFailToPauseForNotExistingSession)
{
    createMediaPipelineShouldSuccess();
    pauseShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToPause)
{
    initSession();
    mediaPipelineWillFailToPause();
    pauseShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldPause)
{
    initSession();
    mediaPipelineWillPause();
    pauseShouldSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldFailToSetPlaybackRateForNotExistingSession)
{
    createMediaPipelineShouldSuccess();
    setPlaybackRateShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToSetPlaybackRate)
{
    initSession();
    mediaPipelineWillFailToSetPlaybackRate();
    setPlaybackRateShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldSetPlaybackRate)
{
    initSession();
    mediaPipelineWillSetPlaybackRate();
    setPlaybackRateShouldSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldFailToSetPositionForNotExistingSession)
{
    createMediaPipelineShouldSuccess();
    setPositionShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToSetPosition)
{
    initSession();
    mediaPipelineWillFailToSetPosition();
    setPositionShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldSetPosition)
{
    initSession();
    mediaPipelineWillSetPosition();
    setPositionShouldSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldFailToSetVideoWindowForNotExistingSession)
{
    createMediaPipelineShouldSuccess();
    setVideoWindowShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToSetVideoWindow)
{
    initSession();
    mediaPipelineWillFailToSetVideoWindow();
    setVideoWindowShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldSetVideoWindow)
{
    initSession();
    mediaPipelineWillSetVideoWindow();
    setVideoWindowShouldSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldFailToHaveDataForNotExistingSession)
{
    createMediaPipelineShouldSuccess();
    haveDataShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToHaveData)
{
    initSession();
    mediaPipelineWillFailToHaveData();
    haveDataShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldHaveData)
{
    initSession();
    mediaPipelineWillHaveData();
    haveDataShouldSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldFailToGetPositionForNotExistingSession)
{
    createMediaPipelineShouldSuccess();
    getPositionShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToGetPosition)
{
    initSession();
    mediaPipelineWillFailToGetPosition();
    getPositionShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldGetPosition)
{
    initSession();
    mediaPipelineWillGetPosition();
    getPositionShouldSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldFailToSetImmediateOutputForNotExistingSession)
{
    createMediaPipelineShouldSuccess();
    setImmediateOutputShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToGetImmediateOutputForNotExistingSession)
{
    createMediaPipelineShouldSuccess();
    getImmediateOutputShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToSetImmediateOutput)
{
    initSession();
    mediaPipelineWillFailToSetImmediateOutput();
    setImmediateOutputShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToGetImmediateOutput)
{
    initSession();
    mediaPipelineWillFailToGetImmediateOutput();
    getImmediateOutputShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldSetImmediateOutput)
{
    initSession();
    mediaPipelineWillSetImmediateOutput();
    setImmediateOutputShouldSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldGetImmediateOutput)
{
    initSession();
    mediaPipelineWillGetImmediateOutput();
    getImmediateOutputShouldSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldFailToGetStatsForNotExistingSession)
{
    createMediaPipelineShouldSuccess();
    getStatsShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToGetStats)
{
    initSession();
    mediaPipelineWillFailToGetStats();
    getStatsShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldGetStats)
{
    initSession();
    mediaPipelineWillGetStats();
    getStatsShouldSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldGetSupportedMimeTypes)
{
    createMediaPipelineShouldSuccess();
    getSupportedMimeTypesSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldCheckSupportedMimeType)
{
    createMediaPipelineShouldSuccess();
    isMimeTypeSupportedSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldCallGetSupportedProperties)
{
    createMediaPipelineShouldSuccess();
    getSupportedPropertiesSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldFailToRenderframeForNotExistingSession)
{
    createMediaPipelineShouldSuccess();
    renderFrameShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToRenderframe)
{
    initSession();
    mediaPipelineWillFailToRenderFrame();
    renderFrameShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldRenderframe)
{
    initSession();
    mediaPipelineWillRenderFrame();
    renderFrameShouldSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldFailToSetVolumeForNotExistingSession)
{
    createMediaPipelineShouldSuccess();
    setVolumeShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToSetVolume)
{
    initSession();
    mediaPipelineWillFailToSetVolume();
    setVolumeShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldSetVolume)
{
    initSession();
    mediaPipelineWillSetVolume();
    setVolumeShouldSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldFailToGetVolumeForNotExistingSession)
{
    createMediaPipelineShouldSuccess();
    getVolumeShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToGetVolume)
{
    initSession();
    mediaPipelineWillFailToGetVolume();
    getVolumeShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldGetVolume)
{
    initSession();
    mediaPipelineWillGetVolume();
    getVolumeShouldSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldFailToSetMuteForNotExistingSession)
{
    createMediaPipelineShouldSuccess();
    setMuteShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToSetMute)
{
    initSession();
    mediaPipelineWillFailToSetMute();
    setMuteShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldSetMute)
{
    initSession();
    mediaPipelineWillSetMute();
    setMuteShouldSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldFailToGetMuteForNotExistingSession)
{
    createMediaPipelineShouldSuccess();
    getMuteShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToGetMute)
{
    initSession();
    mediaPipelineWillFailToGetMute();
    getMuteShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldGetMute)
{
    initSession();
    mediaPipelineWillGetMute();
    getMuteShouldSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldFailToSetLowLatencyForNotExistingSession)
{
    createMediaPipelineShouldSuccess();
    setLowLatencyShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToSetLowLatency)
{
    initSession();
    mediaPipelineWillFailToSetLowLatency();
    setLowLatencyShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldSetLowLatency)
{
    initSession();
    mediaPipelineWillSetLowLatency();
    setLowLatencyShouldSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldFailToSetSyncForNotExistingSession)
{
    createMediaPipelineShouldSuccess();
    setSyncShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToSetSync)
{
    initSession();
    mediaPipelineWillFailToSetSync();
    setSyncShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldSetSync)
{
    initSession();
    mediaPipelineWillSetSync();
    setSyncShouldSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldFailToGetSyncForNotExistingSession)
{
    createMediaPipelineShouldSuccess();
    getSyncShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToGetSync)
{
    initSession();
    mediaPipelineWillFailToGetSync();
    getSyncShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldGetSync)
{
    initSession();
    mediaPipelineWillGetSync();
    getSyncShouldSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldFailToSetSyncOffForNotExistingSession)
{
    createMediaPipelineShouldSuccess();
    setSyncOffShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToSetSyncOff)
{
    initSession();
    mediaPipelineWillFailToSetSyncOff();
    setSyncOffShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldSetSyncOff)
{
    initSession();
    mediaPipelineWillSetSyncOff();
    setSyncOffShouldSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldFailToSetStreamSyncModeForNotExistingSession)
{
    createMediaPipelineShouldSuccess();
    setStreamSyncModeShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToSetStreamSyncMode)
{
    initSession();
    mediaPipelineWillFailToSetStreamSyncMode();
    setStreamSyncModeShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldSetStreamSyncMode)
{
    initSession();
    mediaPipelineWillSetStreamSyncMode();
    setStreamSyncModeShouldSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldFailToGetStreamSyncModeForNotExistingSession)
{
    createMediaPipelineShouldSuccess();
    getStreamSyncModeShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToGetStreamSyncMode)
{
    initSession();
    mediaPipelineWillFailToGetStreamSyncMode();
    getStreamSyncModeShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldGetStreamSyncMode)
{
    initSession();
    mediaPipelineWillGetStreamSyncMode();
    getStreamSyncModeShouldSucceed();
}
TEST_F(MediaPipelineServiceTests, shouldFailToFlushForNotExistingSession)
{
    createMediaPipelineShouldSuccess();
    flushShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToFlush)
{
    initSession();
    mediaPipelineWillFailToFlush();
    flushShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFlush)
{
    initSession();
    mediaPipelineWillFlush();
    flushShouldSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldFailToSetSourcePositionForNotExistingSession)
{
    createMediaPipelineShouldSuccess();
    setSourcePositionShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToSetSourcePosition)
{
    initSession();
    mediaPipelineWillFailToSetSourcePosition();
    setSourcePositionShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldSetSourcePosition)
{
    initSession();
    mediaPipelineWillSetSourcePosition();
    setSourcePositionShouldSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldFailToProcessAudioGapForNotExistingSession)
{
    createMediaPipelineShouldSuccess();
    processAudioGapShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToProcessAudioGap)
{
    initSession();
    mediaPipelineWillFailToProcessAudioGap();
    processAudioGapShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldProcessAudioGap)
{
    initSession();
    mediaPipelineWillProcessAudioGap();
    processAudioGapShouldSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldFailToSetTextTrackIdentifierForNotExistingSession)
{
    createMediaPipelineShouldSuccess();
    setTextTrackIdentifierShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToSetTextTrackIdentifier)
{
    initSession();
    mediaPipelineWillFailToSetTextTrackIdentifier();
    setTextTrackIdentifierShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldSetTextTrackIdentifier)
{
    initSession();
    mediaPipelineWillSetTextTrackIdentifier();
    setTextTrackIdentifierShouldSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldFailToGetTextTrackIdentifierForNotExistingSession)
{
    createMediaPipelineShouldSuccess();
    getTextTrackIdentifierShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToGetTextTrackIdentifier)
{
    initSession();
    mediaPipelineWillFailToGetTextTrackIdentifier();
    getTextTrackIdentifierShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldGetTextTrackIdentifier)
{
    initSession();
    mediaPipelineWillGetTextTrackIdentifier();
    getTextTrackIdentifierShouldSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldFailToSetBufferingLimitForNotExistingSession)
{
    createMediaPipelineShouldSuccess();
    setBufferingLimitShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToSetBufferingLimit)
{
    initSession();
    mediaPipelineWillFailToSetBufferingLimit();
    setBufferingLimitShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldSetBufferingLimit)
{
    initSession();
    mediaPipelineWillSetBufferingLimit();
    setBufferingLimitShouldSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldFailToGetBufferingLimitForNotExistingSession)
{
    createMediaPipelineShouldSuccess();
    getBufferingLimitShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToGetBufferingLimit)
{
    initSession();
    mediaPipelineWillFailToGetBufferingLimit();
    getBufferingLimitShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldGetBufferingLimit)
{
    initSession();
    mediaPipelineWillGetBufferingLimit();
    getBufferingLimitShouldSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldFailToSetUseBufferingForNotExistingSession)
{
    createMediaPipelineShouldSuccess();
    setUseBufferingShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToSetUseBuffering)
{
    initSession();
    mediaPipelineWillFailToSetUseBuffering();
    setUseBufferingShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldSetUseBuffering)
{
    initSession();
    mediaPipelineWillSetUseBuffering();
    setUseBufferingShouldSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldFailToGetUseBufferingForNotExistingSession)
{
    createMediaPipelineShouldSuccess();
    getUseBufferingShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToGetUseBuffering)
{
    initSession();
    mediaPipelineWillFailToGetUseBuffering();
    getUseBufferingShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldGetUseBuffering)
{
    initSession();
    mediaPipelineWillGetUseBuffering();
    getUseBufferingShouldSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldFailToSwitchSourceForNotExistingSession)
{
    createMediaPipelineShouldSuccess();
    switchSourceShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldFailToSwitchSource)
{
    initSession();
    mediaPipelineWillFailToSwitchSource();
    switchSourceShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldSwitchSource)
{
    initSession();
    mediaPipelineWillSwitchSource();
    switchSourceShouldSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldFailToCheckIfVideoIsMaster)
{
    initSession();
    mediaPipelineWillFailToCheckIfVideoIsMaster();
    isVideoMasterShouldFail();
}

TEST_F(MediaPipelineServiceTests, shouldCheckIfVideoIsMaster)
{
    initSession();
    mediaPipelineWillCheckIfVideoIsMaster();
    isVideoMasterShouldSucceed();
}

TEST_F(MediaPipelineServiceTests, shouldPing)
{
    initSession();
    mediaPipelineWillPing();
    triggerPing();
}
