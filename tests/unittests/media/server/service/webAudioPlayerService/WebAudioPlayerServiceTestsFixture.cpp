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
#include "HeartbeatHandlerMock.h"
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
constexpr int kWebAudioPlayerHandle{0};
const std::string kAudioMimeType{"audio/x-raw"};
constexpr uint32_t kPriority{4};
constexpr uint32_t kAvailableFrames{11};
const firebolt::rialto::WebAudioShmInfo kShmInfo{12, 13, 14, 15};
constexpr uint32_t kDelayFrames{16};
constexpr uint32_t kNumberOfFrames{17};
constexpr uint32_t kPreferredFrames{18};
constexpr uint32_t kMaximumFrames{19};
constexpr bool kSupportDeferredPlay{true};
constexpr double kVolume{1.5};
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
      m_heartbeatProcedureMock{std::make_shared<StrictMock<firebolt::rialto::server::HeartbeatProcedureMock>>()},
      m_shmInfo{std::make_shared<firebolt::rialto::WebAudioShmInfo>(kShmInfo)}
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
        .WillOnce(DoAll(SetArgReferee<0>(kAvailableFrames), SetArgReferee<1>(m_shmInfo), Return(true)));
}

void WebAudioPlayerServiceTests::webAudioPlayerWillFailToGetBufferAvailable()
{
    EXPECT_CALL(m_webAudioPlayerMock, getBufferAvailable(_, _)).WillOnce(Return(false));
}

void WebAudioPlayerServiceTests::webAudioPlayerWillGetBufferDelay()
{
    EXPECT_CALL(m_webAudioPlayerMock, getBufferDelay(_)).WillOnce(DoAll(SetArgReferee<0>(kDelayFrames), Return(true)));
}

void WebAudioPlayerServiceTests::webAudioPlayerWillFailToGetBufferDelay()
{
    EXPECT_CALL(m_webAudioPlayerMock, getBufferDelay(_)).WillOnce(Return(false));
}

void WebAudioPlayerServiceTests::webAudioPlayerWillWriteBuffer()
{
    EXPECT_CALL(m_webAudioPlayerMock, writeBuffer(kNumberOfFrames, _)).WillOnce(Return(true));
}

void WebAudioPlayerServiceTests::webAudioPlayerWillFailToWriteBuffer()
{
    EXPECT_CALL(m_webAudioPlayerMock, writeBuffer(kNumberOfFrames, _)).WillOnce(Return(false));
}

void WebAudioPlayerServiceTests::webAudioPlayerWillGetDeviceInfo()
{
    EXPECT_CALL(m_webAudioPlayerMock, getDeviceInfo(_, _, _))
        .WillOnce(DoAll(SetArgReferee<0>(kPreferredFrames), SetArgReferee<1>(kMaximumFrames),
                        SetArgReferee<2>(kSupportDeferredPlay), Return(true)));
}

void WebAudioPlayerServiceTests::webAudioPlayerWillFailToGetDeviceInfo()
{
    EXPECT_CALL(m_webAudioPlayerMock, getDeviceInfo(_, _, _)).WillOnce(Return(false));
}

void WebAudioPlayerServiceTests::webAudioPlayerWillSetVolume()
{
    EXPECT_CALL(m_webAudioPlayerMock, setVolume(kVolume)).WillOnce(Return(true));
}

void WebAudioPlayerServiceTests::webAudioPlayerWillFailToSetVolume()
{
    EXPECT_CALL(m_webAudioPlayerMock, setVolume(kVolume)).WillOnce(Return(false));
}

void WebAudioPlayerServiceTests::webAudioPlayerWillGetVolume()
{
    EXPECT_CALL(m_webAudioPlayerMock, getVolume(_)).WillOnce(DoAll(SetArgReferee<0>(kVolume), Return(true)));
}

void WebAudioPlayerServiceTests::webAudioPlayerWillFailToGetVolume()
{
    EXPECT_CALL(m_webAudioPlayerMock, getVolume(_)).WillOnce(Return(false));
}

void WebAudioPlayerServiceTests::webAudioPlayerWillPing()
{
    EXPECT_CALL(*m_heartbeatProcedureMock, createHandler())
        .WillOnce(Return(ByMove(std::make_unique<StrictMock<firebolt::rialto::server::HeartbeatHandlerMock>>())));
    EXPECT_CALL(m_webAudioPlayerMock, ping(_));
}

void WebAudioPlayerServiceTests::webAudioPlayerFactoryWillCreateWebAudioPlayer()
{
    EXPECT_CALL(*m_webAudioPlayerFactoryMock,
                createWebAudioPlayerServerInternal(_, kAudioMimeType, kPriority, _, _, kWebAudioPlayerHandle, _, _, _))
        .WillOnce(Return(ByMove(std::move(m_webAudioPlayer))));
}

void WebAudioPlayerServiceTests::webAudioPlayerFactoryWillReturnNullptr()
{
    EXPECT_CALL(*m_webAudioPlayerFactoryMock,
                createWebAudioPlayerServerInternal(_, kAudioMimeType, kPriority, _, _, kWebAudioPlayerHandle, _, _, _))
        .WillOnce(Return(ByMove(std::unique_ptr<firebolt::rialto::server::IWebAudioPlayerServerInternal>())));
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
    EXPECT_TRUE(m_sut->createWebAudioPlayer(kWebAudioPlayerHandle, webAudioPlayerClient, kAudioMimeType, kPriority,
                                            std::shared_ptr<const firebolt::rialto::WebAudioConfig>{}));
}

void WebAudioPlayerServiceTests::createWebAudioPlayerShouldFail()
{
    EXPECT_FALSE(m_sut->createWebAudioPlayer(kWebAudioPlayerHandle, webAudioPlayerClient, kAudioMimeType, kPriority,
                                             std::shared_ptr<const firebolt::rialto::WebAudioConfig>{}));
}

void WebAudioPlayerServiceTests::destroyWebAudioPlayerShouldSucceed()
{
    EXPECT_TRUE(m_sut->destroyWebAudioPlayer(kWebAudioPlayerHandle));
}

void WebAudioPlayerServiceTests::destroyWebAudioPlayerShouldFail()
{
    EXPECT_FALSE(m_sut->destroyWebAudioPlayer(kWebAudioPlayerHandle));
}

void WebAudioPlayerServiceTests::playShouldSucceed()
{
    EXPECT_TRUE(m_sut->play(kWebAudioPlayerHandle));
}

void WebAudioPlayerServiceTests::playShouldFail()
{
    EXPECT_FALSE(m_sut->play(kWebAudioPlayerHandle));
}

void WebAudioPlayerServiceTests::pauseShouldSucceed()
{
    EXPECT_TRUE(m_sut->pause(kWebAudioPlayerHandle));
}

void WebAudioPlayerServiceTests::pauseShouldFail()
{
    EXPECT_FALSE(m_sut->pause(kWebAudioPlayerHandle));
}

void WebAudioPlayerServiceTests::setEosShouldSucceed()
{
    EXPECT_TRUE(m_sut->setEos(kWebAudioPlayerHandle));
}

void WebAudioPlayerServiceTests::setEosShouldFail()
{
    EXPECT_FALSE(m_sut->setEos(kWebAudioPlayerHandle));
}

void WebAudioPlayerServiceTests::getBufferAvailableShouldSucceed()
{
    uint32_t availableFramesReturn{};
    std::shared_ptr<firebolt::rialto::WebAudioShmInfo> shmInfoReturn;
    EXPECT_TRUE(m_sut->getBufferAvailable(kWebAudioPlayerHandle, availableFramesReturn, shmInfoReturn));
    EXPECT_EQ(availableFramesReturn, kAvailableFrames);
    EXPECT_EQ(shmInfoReturn->offsetMain, kShmInfo.offsetMain);
    EXPECT_EQ(shmInfoReturn->lengthMain, kShmInfo.lengthMain);
    EXPECT_EQ(shmInfoReturn->offsetWrap, kShmInfo.offsetWrap);
    EXPECT_EQ(shmInfoReturn->lengthWrap, kShmInfo.lengthWrap);
}

void WebAudioPlayerServiceTests::getBufferAvailableShouldFail()
{
    uint32_t availableFramesReturn{};
    std::shared_ptr<firebolt::rialto::WebAudioShmInfo> shmInfoReturn;
    EXPECT_FALSE(m_sut->getBufferAvailable(kWebAudioPlayerHandle, availableFramesReturn, shmInfoReturn));
}

void WebAudioPlayerServiceTests::getBufferDelayShouldSucceed()
{
    uint32_t delayFramesReturn{};
    EXPECT_TRUE(m_sut->getBufferDelay(kWebAudioPlayerHandle, delayFramesReturn));
    EXPECT_EQ(delayFramesReturn, kDelayFrames);
}

void WebAudioPlayerServiceTests::getBufferDelayShouldFail()
{
    uint32_t delayFramesReturn{};
    EXPECT_FALSE(m_sut->getBufferDelay(kWebAudioPlayerHandle, delayFramesReturn));
}

void WebAudioPlayerServiceTests::writeBufferShouldSucceed()
{
    EXPECT_TRUE(m_sut->writeBuffer(kWebAudioPlayerHandle, kNumberOfFrames, nullptr));
}

void WebAudioPlayerServiceTests::writeBufferShouldFail()
{
    EXPECT_FALSE(m_sut->writeBuffer(kWebAudioPlayerHandle, kNumberOfFrames, nullptr));
}

void WebAudioPlayerServiceTests::getDeviceInfoShouldSucceed()
{
    uint32_t preferredFramesReturn{};
    uint32_t maximumFramesReturn{};
    bool supportDeferredPlayReturn{};
    EXPECT_TRUE(m_sut->getDeviceInfo(kWebAudioPlayerHandle, preferredFramesReturn, maximumFramesReturn,
                                     supportDeferredPlayReturn));
}

void WebAudioPlayerServiceTests::getDeviceInfoShouldFail()
{
    uint32_t preferredFramesReturn{};
    uint32_t maximumFramesReturn{};
    bool supportDeferredPlayReturn{};
    EXPECT_FALSE(m_sut->getDeviceInfo(kWebAudioPlayerHandle, preferredFramesReturn, maximumFramesReturn,
                                      supportDeferredPlayReturn));
}

void WebAudioPlayerServiceTests::setVolumeShouldSucceed()
{
    EXPECT_TRUE(m_sut->setVolume(kWebAudioPlayerHandle, kVolume));
}

void WebAudioPlayerServiceTests::setVolumeShouldFail()
{
    EXPECT_FALSE(m_sut->setVolume(kWebAudioPlayerHandle, kVolume));
}

void WebAudioPlayerServiceTests::getVolumeShouldSucceed()
{
    double volumeReturn{};
    EXPECT_TRUE(m_sut->getVolume(kWebAudioPlayerHandle, volumeReturn));
    EXPECT_EQ(volumeReturn, kVolume);
}

void WebAudioPlayerServiceTests::getVolumeShouldFail()
{
    double volumeReturn{};
    EXPECT_FALSE(m_sut->getVolume(kWebAudioPlayerHandle, volumeReturn));
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

void WebAudioPlayerServiceTests::triggerPing()
{
    m_sut->ping(m_heartbeatProcedureMock);
}
