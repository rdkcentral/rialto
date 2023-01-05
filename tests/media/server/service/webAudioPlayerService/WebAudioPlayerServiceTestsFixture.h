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

#ifndef WEB_AUDIO_PLAYER_SERVICE_TESTS_FIXTURE_H_
#define WEB_AUDIO_PLAYER_SERVICE_TESTS_FIXTURE_H_

#include "PlaybackServiceMock.h"
#include "SharedMemoryBufferMock.h"
#include "WebAudioPlayerServerInternalFactoryMock.h"
#include "WebAudioPlayerServerInternalMock.h"
#include "WebAudioPlayerService.h"
#include <gtest/gtest.h>
#include <memory>

using testing::StrictMock;

class WebAudioPlayerServiceTests : public testing::Test
{
public:
    WebAudioPlayerServiceTests();
    ~WebAudioPlayerServiceTests() = default;

    void webAudioPlayerWillPlay();
    void webAudioPlayerWillFailToPlay();
    void webAudioPlayerWillPause();
    void webAudioPlayerWillFailToPause();
    void webAudioPlayerWillSetEos();
    void webAudioPlayerWillFailToSetEos();
    void webAudioPlayerWillGetBufferAvailable();
    void webAudioPlayerWillFailToGetBufferAvailable();
    void webAudioPlayerWillGetBufferDelay();
    void webAudioPlayerWillFailToGetBufferDelay();
    void webAudioPlayerWillWriteBuffer();
    void webAudioPlayerWillFailToWriteBuffer();
    void webAudioPlayerWillGetDeviceInfo();
    void webAudioPlayerWillFailToGetDeviceInfo();
    void webAudioPlayerWillSetVolume();
    void webAudioPlayerWillFailToSetVolume();
    void webAudioPlayerWillGetVolume();
    void webAudioPlayerWillFailToGetVolume();

    void webAudioPlayerFactoryWillCreateWebAudioPlayer();
    void webAudioPlayerFactoryWillReturnNullptr();

    void playbackServiceWillReturnActive();
    void playbackServiceWillReturnInactive();
    void playbackServiceWillReturnMaxWebAudioPlayers(int maxWebAudioPlayers);
    void playbackServiceWillReturnSharedMemoryBuffer();

    void createWebAudioPlayerService();

    void createWebAudioPlayerShouldSucceed();
    void createWebAudioPlayerShouldFail();
    void destroyWebAudioPlayerShouldSucceed();
    void destroyWebAudioPlayerShouldFail();
    void playShouldSucceed();
    void playShouldFail();
    void pauseShouldSucceed();
    void pauseShouldFail();
    void setEosShouldSucceed();
    void setEosShouldFail();
    void getBufferAvailableShouldSucceed();
    void getBufferAvailableShouldFail();
    void getBufferDelayShouldSucceed();
    void getBufferDelayShouldFail();
    void writeBufferShouldSucceed();
    void writeBufferShouldFail();
    void getDeviceInfoShouldSucceed();
    void getDeviceInfoShouldFail();
    void setVolumeShouldSucceed();
    void setVolumeShouldFail();
    void getVolumeShouldSucceed();
    void getVolumeShouldFail();
    void clearWebAudioPlayers();
    void initWebAudioPlayer();

private:
    std::shared_ptr<StrictMock<firebolt::rialto::server::WebAudioPlayerServerInternalFactoryMock>> m_webAudioPlayerFactoryMock;
    std::shared_ptr<firebolt::rialto::server::ISharedMemoryBuffer> m_shmBuffer;
    StrictMock<firebolt::rialto::server::SharedMemoryBufferMock> &m_shmBufferMock;
    std::unique_ptr<firebolt::rialto::IWebAudioPlayer> m_webAudioPlayer;
    StrictMock<firebolt::rialto::server::WebAudioPlayerServerInternalMock> &m_webAudioPlayerMock;
    StrictMock<firebolt::rialto::server::service::PlaybackServiceMock> m_playbackServiceMock;
    std::unique_ptr<firebolt::rialto::server::service::WebAudioPlayerService> m_sut;
    std::shared_ptr<firebolt::rialto::WebAudioShmInfo> m_shmInfo;
};

#endif // WEB_AUDIO_PLAYER_SERVICE_TESTS_FIXTURE_H_
