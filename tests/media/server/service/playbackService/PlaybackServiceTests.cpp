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

TEST_F(PlaybackServiceTests, shouldFailToCreateSessionInInactiveState)
{
    mainThreadWillEnqueueTask();
    createSessionShouldFail();
}

TEST_F(PlaybackServiceTests, shouldFailToCreateSessionAfterSwitchToInactive)
{
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mainThreadWillEnqueueTask();
    triggerSwitchToInactive();
    mainThreadWillEnqueueTask();
    createSessionShouldFail();
}

TEST_F(PlaybackServiceTests, shouldFailToCreateSessionAfterFailedSwitchToActive)
{
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillFailToInitialize();
    triggerSwitchToActive();
    mainThreadWillEnqueueTask();
    createSessionShouldFail();
}

TEST_F(PlaybackServiceTests, shouldFailToCreateSessionWhenMaxPlaybackSessionsIsReached)
{
    constexpr int maxPlaybacks{0};
    triggerSetMaxPlaybacks(maxPlaybacks);
    sharedMemoryBufferWillBeInitialized(maxPlaybacks);
    triggerSwitchToActive();
    mainThreadWillEnqueueTask();
    createSessionShouldFail();
}

TEST_F(PlaybackServiceTests, shouldFailToCreateSessionWhenFactoryReturnsNull)
{
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mediaPipelineFactoryWillReturnNullptr();
    createSessionShouldFail();
}

TEST_F(PlaybackServiceTests, shouldCreateSession)
{
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mediaPipelineFactoryWillCreateMediaPipeline();
    createSessionShouldSucceed();
}

TEST_F(PlaybackServiceTests, shouldFailToCreateSessionWithTheSameIdTwice)
{
    constexpr int maxPlaybacks{2};
    triggerSetMaxPlaybacks(maxPlaybacks);
    sharedMemoryBufferWillBeInitialized(maxPlaybacks);
    triggerSwitchToActive();
    mediaPipelineFactoryWillCreateMediaPipeline();
    createSessionShouldSucceed();
    mainThreadWillEnqueueTask();
    createSessionShouldFail();
}

TEST_F(PlaybackServiceTests, shouldFailToDestroyNotExistingSession)
{
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mainThreadWillEnqueueTask();
    destroySessionShouldFail();
}

TEST_F(PlaybackServiceTests, shouldDestroySession)
{
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mediaPipelineFactoryWillCreateMediaPipeline();
    createSessionShouldSucceed();
    mainThreadWillEnqueueTask();
    destroySessionShouldSucceed();
}

TEST_F(PlaybackServiceTests, shouldDestroySessionWhenSwitchedToInactive)
{
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mediaPipelineFactoryWillCreateMediaPipeline();
    createSessionShouldSucceed();
    mainThreadWillEnqueueTask();
    triggerSwitchToInactive();
    mainThreadWillEnqueueTask();
    destroySessionShouldFail();
}

TEST_F(PlaybackServiceTests, shouldFailToLoadNotExistingSession)
{
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mainThreadWillEnqueueTask();
    loadShouldFail();
}

TEST_F(PlaybackServiceTests, shouldFailToLoadSession)
{
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
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mainThreadWillEnqueueTask();
    attachSourceShouldFail();
}

TEST_F(PlaybackServiceTests, shouldFailToAttachSource)
{
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
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mainThreadWillEnqueueTask();
    removeSourceShouldFail();
}

TEST_F(PlaybackServiceTests, shouldFailToRemoveSource)
{
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
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mainThreadWillEnqueueTask();
    playShouldFail();
}

TEST_F(PlaybackServiceTests, shouldFailToPlay)
{
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
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mainThreadWillEnqueueTask();
    stopShouldFail();
}

TEST_F(PlaybackServiceTests, shouldFailToStop)
{
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
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mainThreadWillEnqueueTask();
    pauseShouldFail();
}

TEST_F(PlaybackServiceTests, shouldFailToPause)
{
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
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mainThreadWillEnqueueTask();
    setPlaybackRateShouldFail();
}

TEST_F(PlaybackServiceTests, shouldFailToSetPlaybackRate)
{
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
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mainThreadWillEnqueueTask();
    setPositionShouldFail();
}

TEST_F(PlaybackServiceTests, shouldFailToSetPosition)
{
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
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mainThreadWillEnqueueTask();
    setVideoWindowShouldFail();
}

TEST_F(PlaybackServiceTests, shouldFailToSetVideoWindow)
{
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
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mainThreadWillEnqueueTask();
    haveDataShouldFail();
}

TEST_F(PlaybackServiceTests, shouldFailToHaveData)
{
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
    mainThreadWillEnqueueTask();
    getSharedMemoryShouldFail();
}

TEST_F(PlaybackServiceTests, shouldGetSharedMemory)
{
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    sharedMemoryBufferWillReturnFdAndSize();
    getSharedMemoryShouldSucceed();
}

TEST_F(PlaybackServiceTests, shouldFailToGetPositionForNotExistingSession)
{
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    getPositionShouldFail();
}

TEST_F(PlaybackServiceTests, shouldFailToGetPosition)
{
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
    triggerSetMaxPlaybacks();
    sharedMemoryBufferWillBeInitialized();
    triggerSwitchToActive();
    mediaPipelineFactoryWillCreateMediaPipeline();
    createSessionShouldSucceed();
    mediaPipelineWillGetPosition();
    getPositionShouldSucceed();
}
