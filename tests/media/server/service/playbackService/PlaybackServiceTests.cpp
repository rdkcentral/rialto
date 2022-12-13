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

#include "PlaybackServiceTestsFixture.h"

TEST_F(PlaybackServiceTests, shouldFailToCreatePlaybackService)
{
    mediaPipelineCapabilitiesFactoryWillReturnNullptr();
}

TEST_F(PlaybackServiceTests, shouldFailToCreateSessionInInactiveState)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    createSessionShouldFail();
}

TEST_F(PlaybackServiceTests, shouldFailToCreateSessionAfterSwitchToInactive)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    triggerSwitchToInactive();
    createSessionShouldFail();
}

TEST_F(PlaybackServiceTests, shouldFailToCreateSessionAfterFailedSwitchToActive)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillFailToInitialize();
    triggerSwitchToActive();
    createSessionShouldFail();
}

TEST_F(PlaybackServiceTests, shouldFailToCreateSessionWhenMaxPlaybackSessionsIsReached)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    constexpr int maxPlaybacks{0};
    triggerSetMaxPlaybacks(maxPlaybacks);
    sharedMemoryBufferWillBeInitialized(maxPlaybacks);
    triggerSwitchToActive();
    createSessionShouldFail();
}

TEST_F(PlaybackServiceTests, shouldFailToCreateSessionWhenFactoryReturnsNull)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mediaPipelineFactoryWillReturnNullptr();
    createSessionShouldFail();
}

TEST_F(PlaybackServiceTests, shouldCreateSession)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mediaPipelineFactoryWillCreateMediaPipeline();
    createSessionShouldSucceed();
}

TEST_F(PlaybackServiceTests, shouldFailToCreateSessionWithTheSameIdTwice)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    constexpr int maxPlaybacks{2};

    triggerSetMaxPlaybacks(maxPlaybacks);
    sharedMemoryBufferWillBeInitialized(maxPlaybacks);
    triggerSwitchToActive();
    mediaPipelineFactoryWillCreateMediaPipeline();
    createSessionShouldSucceed();
    createSessionShouldFail();
}

TEST_F(PlaybackServiceTests, shouldFailToDestroyNotExistingSession)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    destroySessionShouldFail();
}

TEST_F(PlaybackServiceTests, shouldDestroySession)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mediaPipelineFactoryWillCreateMediaPipeline();
    createSessionShouldSucceed();
    destroySessionShouldSucceed();
}

TEST_F(PlaybackServiceTests, shouldDestroySessionWhenSwitchedToInactive)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mediaPipelineFactoryWillCreateMediaPipeline();
    createSessionShouldSucceed();
    triggerSwitchToInactive();
    destroySessionShouldFail();
}

TEST_F(PlaybackServiceTests, shouldFailToLoadNotExistingSession)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    loadShouldFail();
}

TEST_F(PlaybackServiceTests, shouldFailToLoadSession)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mediaPipelineFactoryWillCreateMediaPipeline();
    createSessionShouldSucceed();
    mediaPipelineWillFailToLoad();
    loadShouldFail();
}

TEST_F(PlaybackServiceTests, shouldLoadSession)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mediaPipelineFactoryWillCreateMediaPipeline();
    createSessionShouldSucceed();
    mediaPipelineWillLoad();
    loadShouldSucceed();
}

TEST_F(PlaybackServiceTests, shouldFailToAttachSourceForNotExistingSession)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    attachSourceShouldFail();
}

TEST_F(PlaybackServiceTests, shouldFailToAttachSource)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mediaPipelineFactoryWillCreateMediaPipeline();
    createSessionShouldSucceed();
    mediaPipelineWillFailToAttachSource();
    attachSourceShouldFail();
}

TEST_F(PlaybackServiceTests, shouldAttachSource)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mediaPipelineFactoryWillCreateMediaPipeline();
    createSessionShouldSucceed();
    mediaPipelineWillAttachSource();
    attachSourceShouldSucceed();
}

TEST_F(PlaybackServiceTests, shouldFailToRemoveSourceForNotExistingSession)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    removeSourceShouldFail();
}

TEST_F(PlaybackServiceTests, shouldFailToRemoveSource)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mediaPipelineFactoryWillCreateMediaPipeline();
    createSessionShouldSucceed();
    mediaPipelineWillFailToRemoveSource();
    removeSourceShouldFail();
}

TEST_F(PlaybackServiceTests, shouldRemoveSource)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mediaPipelineFactoryWillCreateMediaPipeline();
    createSessionShouldSucceed();
    mediaPipelineWillRemoveSource();
    removeSourceShouldSucceed();
}

TEST_F(PlaybackServiceTests, shouldFailToPlayForNotExistingSession)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    playShouldFail();
}

TEST_F(PlaybackServiceTests, shouldFailToPlay)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mediaPipelineFactoryWillCreateMediaPipeline();
    createSessionShouldSucceed();
    mediaPipelineWillFailToPlay();
    playShouldFail();
}

TEST_F(PlaybackServiceTests, shouldPlay)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mediaPipelineFactoryWillCreateMediaPipeline();
    createSessionShouldSucceed();
    mediaPipelineWillPlay();
    playShouldSucceed();
}

TEST_F(PlaybackServiceTests, shouldFailToStopForNotExistingSession)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    stopShouldFail();
}

TEST_F(PlaybackServiceTests, shouldFailToStop)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mediaPipelineFactoryWillCreateMediaPipeline();
    createSessionShouldSucceed();
    mediaPipelineWillFailToStop();
    stopShouldFail();
}

TEST_F(PlaybackServiceTests, shouldStop)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mediaPipelineFactoryWillCreateMediaPipeline();
    createSessionShouldSucceed();
    mediaPipelineWillStop();
    stopShouldSucceed();
}

TEST_F(PlaybackServiceTests, shouldFailToPauseForNotExistingSession)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    pauseShouldFail();
}

TEST_F(PlaybackServiceTests, shouldFailToPause)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mediaPipelineFactoryWillCreateMediaPipeline();
    createSessionShouldSucceed();
    mediaPipelineWillFailToPause();
    pauseShouldFail();
}

TEST_F(PlaybackServiceTests, shouldPause)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mediaPipelineFactoryWillCreateMediaPipeline();
    createSessionShouldSucceed();
    mediaPipelineWillPause();
    pauseShouldSucceed();
}

TEST_F(PlaybackServiceTests, shouldFailToSetPlaybackRateForNotExistingSession)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    setPlaybackRateShouldFail();
}

TEST_F(PlaybackServiceTests, shouldFailToSetPlaybackRate)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mediaPipelineFactoryWillCreateMediaPipeline();
    createSessionShouldSucceed();
    mediaPipelineWillFailToSetPlaybackRate();
    setPlaybackRateShouldFail();
}

TEST_F(PlaybackServiceTests, shouldSetPlaybackRate)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mediaPipelineFactoryWillCreateMediaPipeline();
    createSessionShouldSucceed();
    mediaPipelineWillSetPlaybackRate();
    setPlaybackRateShouldSucceed();
}

TEST_F(PlaybackServiceTests, shouldFailToSetPositionForNotExistingSession)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    setPositionShouldFail();
}

TEST_F(PlaybackServiceTests, shouldFailToSetPosition)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mediaPipelineFactoryWillCreateMediaPipeline();
    createSessionShouldSucceed();
    mediaPipelineWillFailToSetPosition();
    setPositionShouldFail();
}

TEST_F(PlaybackServiceTests, shouldSetPosition)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mediaPipelineFactoryWillCreateMediaPipeline();
    createSessionShouldSucceed();
    mediaPipelineWillSetPosition();
    setPositionShouldSucceed();
}

TEST_F(PlaybackServiceTests, shouldFailToSetVideoWindowForNotExistingSession)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    setVideoWindowShouldFail();
}

TEST_F(PlaybackServiceTests, shouldFailToSetVideoWindow)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mediaPipelineFactoryWillCreateMediaPipeline();
    createSessionShouldSucceed();
    mediaPipelineWillFailToSetVideoWindow();
    setVideoWindowShouldFail();
}

TEST_F(PlaybackServiceTests, shouldSetVideoWindow)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mediaPipelineFactoryWillCreateMediaPipeline();
    createSessionShouldSucceed();
    mediaPipelineWillSetVideoWindow();
    setVideoWindowShouldSucceed();
}

TEST_F(PlaybackServiceTests, shouldFailToHaveDataForNotExistingSession)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    haveDataShouldFail();
}

TEST_F(PlaybackServiceTests, shouldFailToHaveData)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mediaPipelineFactoryWillCreateMediaPipeline();
    createSessionShouldSucceed();
    mediaPipelineWillFailToHaveData();
    haveDataShouldFail();
}

TEST_F(PlaybackServiceTests, shouldHaveData)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mediaPipelineFactoryWillCreateMediaPipeline();
    createSessionShouldSucceed();
    mediaPipelineWillHaveData();
    haveDataShouldSucceed();
}

TEST_F(PlaybackServiceTests, shouldFailToGetSharedMemoryInInactiveState)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    getSharedMemoryShouldFail();
}

TEST_F(PlaybackServiceTests, shouldGetSharedMemory)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    sharedMemoryBufferWillReturnFdAndSize();
    getSharedMemoryShouldSucceed();
}

TEST_F(PlaybackServiceTests, shouldFailToGetPositionForNotExistingSession)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    getPositionShouldFail();
}

TEST_F(PlaybackServiceTests, shouldFailToGetPosition)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mediaPipelineFactoryWillCreateMediaPipeline();
    createSessionShouldSucceed();
    mediaPipelineWillFailToGetPosition();
    getPositionShouldFail();
}

TEST_F(PlaybackServiceTests, shouldGetPosition)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mediaPipelineFactoryWillCreateMediaPipeline();
    createSessionShouldSucceed();
    mediaPipelineWillGetPosition();
    getPositionShouldSucceed();
}

TEST_F(PlaybackServiceTests, shouldGetSupportedMimeTypes)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    getSupportedMimeTypesSucceed();
}

TEST_F(PlaybackServiceTests, shouldCheckSupportedMimeType)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    isMimeTypeSupportedSucceed();
}

TEST_F(PlaybackServiceTests, shouldRenderframe)
{
    mediaPipelineCapabilitiesFactoryWillCreateMediaPipelineCapabilities();
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mediaPipelineFactoryWillCreateMediaPipeline();
    createSessionShouldSucceed();
    renderFrameSucceed();
}
