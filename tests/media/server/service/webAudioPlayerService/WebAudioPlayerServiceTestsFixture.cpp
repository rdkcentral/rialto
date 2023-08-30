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
#include "MediaCommon.h"
#include <string>
#include <utility>
#include <vector>

using testing::_;
using testing::ByMove;
using testing::DoAll;
using testing::Invoke;
using testing::Return;
using testing::SetArgReferee;
using testing::Throw;

namespace
{
constexpr int webAudioPlayerHandle{0};
const std::string audioMimeType{"audio/x-raw"};
constexpr uint32_t priority{4};
constexpr firebolt::rialto::WebAudioPcmConfig pcmConfig{1, 2, 3, false, true, false};
constexpr firebolt::rialto::WebAudioPlayerState webAudioPlayerState{firebolt::rialto::WebAudioPlayerState::END_OF_STREAM};
constexpr uint32_t availableFrames{11};
const firebolt::rialto::WebAudioShmInfo shmInfo{12, 13, 14, 15};
constexpr uint32_t delayFrames{16};
constexpr uint32_t numberOfFrames{17};
constexpr uint32_t preferredFrames{18};
constexpr uint32_t maximumFrames{19};
constexpr bool supportDeferredPlay{true};
constexpr double volume{1.5};
const std::shared_ptr<firebolt::rialto::IWebAudioPlayerClient> webAudioPlayerClient; // nullptr as it's not used anywhere in tests
} // namespace

WebAudioPlayerServiceTests::WebAudioPlayerServiceTests()
    : m_webAudioPlayerFactoryMock{std::make_shared<
          StrictMock<firebolt::rialto::server::WebAudioPlayerServerInternalFactoryMock>>()},
      m_shmBuffer{std::make_shared<StrictMock<firebolt::rialto::server::SharedMemoryBufferMock>>()},
      m_shmBufferMock{dynamic_cast<StrictMock<firebolt::rialto::server::SharedMemoryBufferMock> &>(*m_shmBuffer)},
      m_webAudioPlayer{std::make_unique<StrictMock<firebolt::rialto::server::WebAudioPlayerServerInternalMock>>()},
      m_webAudioPlayerMock{
          dynamic_cast<StrictMock<firebolt::rialto::server::WebAudioPlayerServerInternalMock> &>(*m_webAudioPlayer)},
      m_shmInfo{std::make_shared<firebolt::rialto::WebAudioShmInfo>(shmInfo)}
{
}

void WebAudioPlayerServiceTests::webAudioPlayerWillPlay()
{
    EXPECT_CALL(m_webAudioPlayerMock, play()).WillOnce(Return(true));
}

void WebAudioPlayerServiceTests::webAudioPlayerWillFailToPlay()
{
    EXPECT_CALL(m_webAudioPlayerMock, play()).WillOnce(Return(false));
}

void WebAudioPlayerServiceTests::webAudioPlayerWillPause()
{
    EXPECT_CALL(m_webAudioPlayerMock, pause()).WillOnce(Return(true));
}

void WebAudioPlayerServiceTests::webAudioPlayerWillFailToPause()
{
    EXPECT_CALL(m_webAudioPlayerMock, pause()).WillOnce(Return(false));
}

void WebAudioPlayerServiceTests::webAudioPlayerWillSetEos()
{
    EXPECT_CALL(m_webAudioPlayerMock, setEos()).WillOnce(Return(true));
}

void WebAudioPlayerServiceTests::webAudioPlayerWillFailToSetEos()
{
    EXPECT_CALL(m_webAudioPlayerMock, setEos()).WillOnce(Return(false));
}

void WebAudioPlayerServiceTests::webAudioPlayerWillGetBufferAvailable()
{
    EXPECT_CALL(m_webAudioPlayerMock, getBufferAvailable(_, _))
        .WillOnce(DoAll(SetArgReferee<0>(availableFrames), SetArgReferee<1>(m_shmInfo), Return(true)));
}

void WebAudioPlayerServiceTests::webAudioPlayerWillFailToGetBufferAvailable()
{
    EXPECT_CALL(m_webAudioPlayerMock, getBufferAvailable(_, _)).WillOnce(Return(false));
}

void WebAudioPlayerServiceTests::webAudioPlayerWillGetBufferDelay()
{
    EXPECT_CALL(m_webAudioPlayerMock, getBufferDelay(_)).WillOnce(DoAll(SetArgReferee<0>(delayFrames), Return(true)));
}

void WebAudioPlayerServiceTests::webAudioPlayerWillFailToGetBufferDelay()
{
    EXPECT_CALL(m_webAudioPlayerMock, getBufferDelay(_)).WillOnce(Return(false));
}

void WebAudioPlayerServiceTests::webAudioPlayerWillWriteBuffer()
{
    EXPECT_CALL(m_webAudioPlayerMock, writeBuffer(numberOfFrames, _)).WillOnce(Return(true));
}

void WebAudioPlayerServiceTests::webAudioPlayerWillFailToWriteBuffer()
{
    EXPECT_CALL(m_webAudioPlayerMock, writeBuffer(numberOfFrames, _)).WillOnce(Return(false));
}

void WebAudioPlayerServiceTests::webAudioPlayerWillGetDeviceInfo()
{
    EXPECT_CALL(m_webAudioPlayerMock, getDeviceInfo(_, _, _))
        .WillOnce(DoAll(SetArgReferee<0>(preferredFrames), SetArgReferee<1>(maximumFrames),
                        SetArgReferee<2>(supportDeferredPlay), Return(true)));
}

void WebAudioPlayerServiceTests::webAudioPlayerWillFailToGetDeviceInfo()
{
    EXPECT_CALL(m_webAudioPlayerMock, getDeviceInfo(_, _, _)).WillOnce(Return(false));
}

void WebAudioPlayerServiceTests::webAudioPlayerWillSetVolume()
{
    EXPECT_CALL(m_webAudioPlayerMock, setVolume(volume)).WillOnce(Return(true));
}

void WebAudioPlayerServiceTests::webAudioPlayerWillFailToSetVolume()
{
    EXPECT_CALL(m_webAudioPlayerMock, setVolume(volume)).WillOnce(Return(false));
}

void WebAudioPlayerServiceTests::webAudioPlayerWillGetVolume()
{
    EXPECT_CALL(m_webAudioPlayerMock, getVolume(_)).WillOnce(DoAll(SetArgReferee<0>(volume), Return(true)));
}

void WebAudioPlayerServiceTests::webAudioPlayerWillFailToGetVolume()
{
    EXPECT_CALL(m_webAudioPlayerMock, getVolume(_)).WillOnce(Return(false));
}

void WebAudioPlayerServiceTests::webAudioPlayerFactoryWillCreateWebAudioPlayer()
{
    EXPECT_CALL(*m_webAudioPlayerFactoryMock,
                createWebAudioPlayerServerInternal(_, audioMimeType, priority, _, _, webAudioPlayerHandle, _,_,_))
        .WillOnce(Return(ByMove(std::move(m_webAudioPlayer))));
}

void WebAudioPlayerServiceTests::webAudioPlayerFactoryWillReturnNullptr()
{
    EXPECT_CALL(*m_webAudioPlayerFactoryMock,
                createWebAudioPlayerServerInternal(_, audioMimeType, priority, _, _, webAudioPlayerHandle,_,_,_))
        .WillOnce(Return(ByMove(std::unique_ptr<firebolt::rialto::IWebAudioPlayer>())));
}

void WebAudioPlayerServiceTests::playbackServiceWillReturnActive()
{
    EXPECT_CALL(m_playbackServiceMock, isActive()).WillOnce(Return(true)).RetiresOnSaturation();
}

void WebAudioPlayerServiceTests::playbackServiceWillReturnInactive()
{
    EXPECT_CALL(m_playbackServiceMock, isActive()).WillOnce(Return(false)).RetiresOnSaturation();
}

void WebAudioPlayerServiceTests::playbackServiceWillReturnMaxWebAudioPlayers(int maxWebAudioPlayers)
{
    EXPECT_CALL(m_playbackServiceMock, getMaxWebAudioPlayers()).WillOnce(Return(maxWebAudioPlayers)).RetiresOnSaturation();
}

void WebAudioPlayerServiceTests::playbackServiceWillReturnSharedMemoryBuffer()
{
    EXPECT_CALL(m_playbackServiceMock, getShmBuffer()).WillOnce(Return(m_shmBuffer)).RetiresOnSaturation();
}

void WebAudioPlayerServiceTests::createWebAudioPlayerService()
{
    m_sut = std::make_unique<firebolt::rialto::server::service::WebAudioPlayerService>(m_playbackServiceMock,
                                                                                       m_webAudioPlayerFactoryMock);
}

void WebAudioPlayerServiceTests::createWebAudioPlayerShouldSucceed()
{
    EXPECT_TRUE(m_sut->createWebAudioPlayer(webAudioPlayerHandle, webAudioPlayerClient, audioMimeType, priority, nullptr));
}

void WebAudioPlayerServiceTests::createWebAudioPlayerShouldFail()
{
    EXPECT_FALSE(
        m_sut->createWebAudioPlayer(webAudioPlayerHandle, webAudioPlayerClient, audioMimeType, priority, nullptr));
}

void WebAudioPlayerServiceTests::destroyWebAudioPlayerShouldSucceed()
{
    EXPECT_TRUE(m_sut->destroyWebAudioPlayer(webAudioPlayerHandle));
}

void WebAudioPlayerServiceTests::destroyWebAudioPlayerShouldFail()
{
    EXPECT_FALSE(m_sut->destroyWebAudioPlayer(webAudioPlayerHandle));
}

void WebAudioPlayerServiceTests::playShouldSucceed()
{
    EXPECT_TRUE(m_sut->play(webAudioPlayerHandle));
}

void WebAudioPlayerServiceTests::playShouldFail()
{
    EXPECT_FALSE(m_sut->play(webAudioPlayerHandle));
}

void WebAudioPlayerServiceTests::pauseShouldSucceed()
{
    EXPECT_TRUE(m_sut->pause(webAudioPlayerHandle));
}

void WebAudioPlayerServiceTests::pauseShouldFail()
{
    EXPECT_FALSE(m_sut->pause(webAudioPlayerHandle));
}

void WebAudioPlayerServiceTests::setEosShouldSucceed()
{
    EXPECT_TRUE(m_sut->setEos(webAudioPlayerHandle));
}

void WebAudioPlayerServiceTests::setEosShouldFail()
{
    EXPECT_FALSE(m_sut->setEos(webAudioPlayerHandle));
}

void WebAudioPlayerServiceTests::getBufferAvailableShouldSucceed()
{
    uint32_t availableFramesReturn{};
    std::shared_ptr<firebolt::rialto::WebAudioShmInfo> shmInfoReturn;
    EXPECT_TRUE(m_sut->getBufferAvailable(webAudioPlayerHandle, availableFramesReturn, shmInfoReturn));
    EXPECT_EQ(availableFramesReturn, availableFrames);
    EXPECT_EQ(shmInfoReturn->offsetMain, shmInfo.offsetMain);
    EXPECT_EQ(shmInfoReturn->lengthMain, shmInfo.lengthMain);
    EXPECT_EQ(shmInfoReturn->offsetWrap, shmInfo.offsetWrap);
    EXPECT_EQ(shmInfoReturn->lengthWrap, shmInfo.lengthWrap);
}

void WebAudioPlayerServiceTests::getBufferAvailableShouldFail()
{
    uint32_t availableFramesReturn{};
    std::shared_ptr<firebolt::rialto::WebAudioShmInfo> shmInfoReturn;
    EXPECT_FALSE(m_sut->getBufferAvailable(webAudioPlayerHandle, availableFramesReturn, shmInfoReturn));
}

void WebAudioPlayerServiceTests::getBufferDelayShouldSucceed()
{
    uint32_t delayFramesReturn{};
    EXPECT_TRUE(m_sut->getBufferDelay(webAudioPlayerHandle, delayFramesReturn));
    EXPECT_EQ(delayFramesReturn, delayFrames);
}

void WebAudioPlayerServiceTests::getBufferDelayShouldFail()
{
    uint32_t delayFramesReturn{};
    EXPECT_FALSE(m_sut->getBufferDelay(webAudioPlayerHandle, delayFramesReturn));
}

void WebAudioPlayerServiceTests::writeBufferShouldSucceed()
{
    EXPECT_TRUE(m_sut->writeBuffer(webAudioPlayerHandle, numberOfFrames, nullptr));
}

void WebAudioPlayerServiceTests::writeBufferShouldFail()
{
    EXPECT_FALSE(m_sut->writeBuffer(webAudioPlayerHandle, numberOfFrames, nullptr));
}

void WebAudioPlayerServiceTests::getDeviceInfoShouldSucceed()
{
    uint32_t preferredFramesReturn{};
    uint32_t maximumFramesReturn{};
    bool supportDeferredPlayReturn{};
    EXPECT_TRUE(m_sut->getDeviceInfo(webAudioPlayerHandle, preferredFramesReturn, maximumFramesReturn,
                                     supportDeferredPlayReturn));
}

void WebAudioPlayerServiceTests::getDeviceInfoShouldFail()
{
    uint32_t preferredFramesReturn{};
    uint32_t maximumFramesReturn{};
    bool supportDeferredPlayReturn{};
    EXPECT_FALSE(m_sut->getDeviceInfo(webAudioPlayerHandle, preferredFramesReturn, maximumFramesReturn,
                                      supportDeferredPlayReturn));
}

void WebAudioPlayerServiceTests::setVolumeShouldSucceed()
{
    EXPECT_TRUE(m_sut->setVolume(webAudioPlayerHandle, volume));
}

void WebAudioPlayerServiceTests::setVolumeShouldFail()
{
    EXPECT_FALSE(m_sut->setVolume(webAudioPlayerHandle, volume));
}

void WebAudioPlayerServiceTests::getVolumeShouldSucceed()
{
    double volumeReturn{};
    EXPECT_TRUE(m_sut->getVolume(webAudioPlayerHandle, volumeReturn));
    EXPECT_EQ(volumeReturn, volume);
}

void WebAudioPlayerServiceTests::getVolumeShouldFail()
{
    double volumeReturn{};
    EXPECT_FALSE(m_sut->getVolume(webAudioPlayerHandle, volumeReturn));
}

void WebAudioPlayerServiceTests::clearWebAudioPlayers()
{
    m_sut->clearWebAudioPlayers();
}

void WebAudioPlayerServiceTests::initWebAudioPlayer()
{
    createWebAudioPlayerService();
    playbackServiceWillReturnActive();
    playbackServiceWillReturnMaxWebAudioPlayers(1);
    playbackServiceWillReturnSharedMemoryBuffer();
    webAudioPlayerFactoryWillCreateWebAudioPlayer();
    createWebAudioPlayerShouldSucceed();
}
