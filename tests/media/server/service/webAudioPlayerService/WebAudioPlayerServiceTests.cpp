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

#include "WebAudioPlayerServiceTestsFixture.h"

TEST_F(WebAudioPlayerServiceTests, shouldFailToCreateWebAudioPlayerInInactiveState)
{
    createWebAudioPlayerService();
    playbackServiceWillReturnInactive();
    createWebAudioPlayerShouldFail();
}

TEST_F(WebAudioPlayerServiceTests, shouldFailToCreateWebAudioPlayerWhenMaxPlaybackWebAudioPlayersIsReached)
{
    createWebAudioPlayerService();
    playbackServiceWillReturnActive();
    playbackServiceWillReturnMaxWebAudioInstances(0);
    createWebAudioPlayerShouldFail();
}

TEST_F(WebAudioPlayerServiceTests, shouldFailToCreateWebAudioPlayerWhenFactoryReturnsNull)
{
    createWebAudioPlayerService();
    playbackServiceWillReturnActive();
    playbackServiceWillReturnMaxWebAudioInstances(1);
    playbackServiceWillReturnSharedMemoryBuffer();
    webAudioPlayerFactoryWillReturnNullptr();
    createWebAudioPlayerShouldFail();
}

TEST_F(WebAudioPlayerServiceTests, shouldCreateWebAudioPlayer)
{
    initWebAudioPlayer();
}

TEST_F(WebAudioPlayerServiceTests, shouldFailToCreateWebAudioPlayerWithTheSameIdTwice)
{
    initWebAudioPlayer();
    playbackServiceWillReturnActive();
    playbackServiceWillReturnMaxWebAudioInstances(2);
    createWebAudioPlayerShouldFail();
}

TEST_F(WebAudioPlayerServiceTests, shouldFailToDestroyNotExistingWebAudioPlayer)
{
    createWebAudioPlayerService();
    destroyWebAudioPlayerShouldFail();
}

TEST_F(WebAudioPlayerServiceTests, shouldDestroyWebAudioPlayer)
{
    initWebAudioPlayer();
    destroyWebAudioPlayerShouldSucceed();
}

TEST_F(WebAudioPlayerServiceTests, shouldDestroyWebAudioPlayerWhenClearingTheWebAudioPlayer)
{
    initWebAudioPlayer();
    clearWebAudioPlayers();
    // Destroy session should fail because it has already been destroyed when clearing
    destroyWebAudioPlayerShouldFail();
}

TEST_F(WebAudioPlayerServiceTests, shouldFailToPlayForNotExistingWebAudioPlayer)
{
    createWebAudioPlayerService();
    playShouldFail();
}

TEST_F(WebAudioPlayerServiceTests, shouldFailToPlay)
{
    initWebAudioPlayer();
    webAudioPlayerWillFailToPlay();
    playShouldFail();
}

TEST_F(WebAudioPlayerServiceTests, shouldPlay)
{
    initWebAudioPlayer();
    webAudioPlayerWillPlay();
    playShouldSucceed();
}

TEST_F(WebAudioPlayerServiceTests, shouldFailToPauseForNotExistingWebAudioPlayer)
{
    createWebAudioPlayerService();
    pauseShouldFail();
}

TEST_F(WebAudioPlayerServiceTests, shouldFailToPause)
{
    initWebAudioPlayer();
    webAudioPlayerWillFailToPause();
    pauseShouldFail();
}

TEST_F(WebAudioPlayerServiceTests, shouldPause)
{
    initWebAudioPlayer();
    webAudioPlayerWillPause();
    pauseShouldSucceed();
}

TEST_F(WebAudioPlayerServiceTests, shouldFailToSetEosForNotExistingWebAudioPlayer)
{
    createWebAudioPlayerService();
    setEosShouldFail();
}

TEST_F(WebAudioPlayerServiceTests, shouldFailToSetEos)
{
    initWebAudioPlayer();
    webAudioPlayerWillFailToSetEos();
    setEosShouldFail();
}

TEST_F(WebAudioPlayerServiceTests, shouldSetEos)
{
    initWebAudioPlayer();
    webAudioPlayerWillSetEos();
    setEosShouldSucceed();
}

TEST_F(WebAudioPlayerServiceTests, shouldFailToGetBufferAvailableForNotExistingWebAudioPlayer)
{
    createWebAudioPlayerService();
    getBufferAvailableShouldFail();
}

TEST_F(WebAudioPlayerServiceTests, shouldFailToGetBufferAvailable)
{
    initWebAudioPlayer();
    webAudioPlayerWillFailToGetBufferAvailable();
    getBufferAvailableShouldFail();
}

TEST_F(WebAudioPlayerServiceTests, shouldGetBufferAvailable)
{
    initWebAudioPlayer();
    webAudioPlayerWillGetBufferAvailable();
    getBufferAvailableShouldSucceed();
}

TEST_F(WebAudioPlayerServiceTests, shouldFailToGetBufferDelayForNotExistingWebAudioPlayer)
{
    createWebAudioPlayerService();
    getBufferDelayShouldFail();
}

TEST_F(WebAudioPlayerServiceTests, shouldFailToGetBufferDelay)
{
    initWebAudioPlayer();
    webAudioPlayerWillFailToGetBufferDelay();
    getBufferDelayShouldFail();
}

TEST_F(WebAudioPlayerServiceTests, shouldGetBufferDelay)
{
    initWebAudioPlayer();
    webAudioPlayerWillGetBufferDelay();
    getBufferDelayShouldSucceed();
}

TEST_F(WebAudioPlayerServiceTests, shouldFailToWriteBufferForNotExistingWebAudioPlayer)
{
    createWebAudioPlayerService();
    writeBufferShouldFail();
}

TEST_F(WebAudioPlayerServiceTests, shouldFailToWriteBuffer)
{
    initWebAudioPlayer();
    webAudioPlayerWillFailToWriteBuffer();
    writeBufferShouldFail();
}

TEST_F(WebAudioPlayerServiceTests, shouldWriteBuffer)
{
    initWebAudioPlayer();
    webAudioPlayerWillWriteBuffer();
    writeBufferShouldSucceed();
}

TEST_F(WebAudioPlayerServiceTests, shouldFailToGetDeviceInfoForNotExistingWebAudioPlayer)
{
    createWebAudioPlayerService();
    getDeviceInfoShouldFail();
}

TEST_F(WebAudioPlayerServiceTests, shouldFailToGetDeviceInfo)
{
    initWebAudioPlayer();
    webAudioPlayerWillFailToGetDeviceInfo();
    getDeviceInfoShouldFail();
}

TEST_F(WebAudioPlayerServiceTests, shouldGetDeviceInfo)
{
    initWebAudioPlayer();
    webAudioPlayerWillGetDeviceInfo();
    getDeviceInfoShouldSucceed();
}

TEST_F(WebAudioPlayerServiceTests, shouldFailToSetVolumeForNotExistingWebAudioPlayer)
{
    createWebAudioPlayerService();
    setVolumeShouldFail();
}

TEST_F(WebAudioPlayerServiceTests, shouldFailToSetVolume)
{
    initWebAudioPlayer();
    webAudioPlayerWillFailToSetVolume();
    setVolumeShouldFail();
}

TEST_F(WebAudioPlayerServiceTests, shouldSetVolume)
{
    initWebAudioPlayer();
    webAudioPlayerWillSetVolume();
    setVolumeShouldSucceed();
}

TEST_F(WebAudioPlayerServiceTests, shouldFailToGetVolumeForNotExistingWebAudioPlayer)
{
    createWebAudioPlayerService();
    getVolumeShouldFail();
}

TEST_F(WebAudioPlayerServiceTests, shouldFailToGetVolume)
{
    initWebAudioPlayer();
    webAudioPlayerWillFailToGetVolume();
    getVolumeShouldFail();
}

TEST_F(WebAudioPlayerServiceTests, shouldGetVolume)
{
    initWebAudioPlayer();
    webAudioPlayerWillGetVolume();
    getVolumeShouldSucceed();
}
