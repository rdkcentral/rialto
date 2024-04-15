/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Sky UK
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

#ifndef FIREBOLT_RIALTO_CLIENT_CT_WEB_AUDIO_PLAYER_TEST_METHODS_H_
#define FIREBOLT_RIALTO_CLIENT_CT_WEB_AUDIO_PLAYER_TEST_METHODS_H_

#include "IWebAudioPlayer.h"
#include "ServerStub.h"
#include "WebAudioPlayerClientMock.h"
#include "WebAudioPlayerModuleMock.h"
#include <gtest/gtest.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

using ::testing::_;
using ::testing::Invoke;
using ::testing::SetArgPointee;
using ::testing::StrictMock;
using ::testing::WithArgs;

using namespace firebolt::rialto;

namespace firebolt::rialto::client::ct
{
class WebAudioPlayerTestMethods
{
public:
    explicit WebAudioPlayerTestMethods(const std::vector<firebolt::rialto::WebAudioShmInfo> &webAudioShmInfo);
    virtual ~WebAudioPlayerTestMethods();

protected:
    // Strict Mocks
    std::shared_ptr<StrictMock<WebAudioPlayerModuleMock>> m_webAudioPlayerModuleMock;
    std::shared_ptr<StrictMock<WebAudioPlayerClientMock>> m_webAudioPlayerClientMock;

    // Objects
    std::shared_ptr<IWebAudioPlayerFactory> m_webAudioPlayerFactory;
    std::shared_ptr<IWebAudioPlayer> m_webAudioPlayer;
    std::shared_ptr<WebAudioConfig> m_config = std::make_shared<WebAudioConfig>();
    uint32_t m_bytesPerFrame;
    std::shared_ptr<WebAudioShmInfo> m_webAudioShmInfo = std::make_shared<WebAudioShmInfo>();
    std::shared_ptr<WebAudioShmInfo> shmInfo = std::make_shared<WebAudioShmInfo>();

    // Test methods
    void sendNotifyWebAudioPlayerStateIdle();
    void sendNotifyWebAudioPlayerStatePause();
    void sendNotifyWebAudioPlayerStatePlay();
    void sendNotifyWebAudioPlayerStateEos();
    void sendNotifyWebAudioPlayerStateFailure();

    // Expect methods
    void shouldCreateWebAudioPlayer();
    void shouldNotCreateWebAudioPlayer();
    void doesNotCreateWebAudioPlayer();
    void shouldDestroyWebAudioPlayer();
    void shouldNotifyWebAudioPlayerStateIdle();
    void shouldNotifyWebAudioPlayerStatePause();
    void shouldNotifyWebAudioPlayerStatePlay();
    void shouldNotifyWebAudioPlayerStateEos();
    void shouldNotifyWebAudioPlayerStateFailure();
    void shouldGetDeviceInfo();
    void checkWebAudioPlayerClient();
    void shouldPlay();
    void shouldNotPlay();
    void doesNotPlay();
    void shouldPause();
    void shouldNotPause();
    void doesNotPause();
    void shouldEos();
    void shouldGetBufferAvailable();
    void shouldWriteBuffer();
    void shouldNotWriteBuffer();
    void doesNotWriteBuffer();
    void checkBuffer();
    void shouldGetBufferDelay();
    void shouldSetVolume(const double expectedVolume);
    void shouldGetVolume(const double volume);

    // Api methods
    void createWebAudioPlayer();
    void destroyWebAudioPlayer();
    void getDeviceInfo();
    void play();
    void pause();
    void setEos();
    void getBufferAvailable();
    void writeBuffer();
    void getBufferDelay();
    void setVolume(const double volume);
    void getVolume(const double expectedVolume);

    // Component test helpers
    virtual std::shared_ptr<ServerStub> &getServerStub() = 0;
    virtual void waitEvent() = 0;
    virtual void notifyEvent() = 0;
    virtual void *getShmAddress() = 0;

private:
    // Non const variables
    std::vector<std::shared_ptr<WebAudioShmInfo>> m_locationToWriteWebAudio;

    void resetWriteLocation(uint32_t partitionId);
};
} // namespace firebolt::rialto::client::ct

#endif // FIREBOLT_RIALTO_CLIENT_CT_WEB_AUDIO_PLAYER_TEST_METHODS_H_
