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

#include "WebAudioPlayerTestMethods.h"
#include "WebAudioPlayerProtoRequestMatchers.h"
#include <memory>
#include <string>
#include <vector>

namespace
{
const std::string kAudioMimeType{"audio/x-raw"};
const uint32_t kNumberOfFrames{1};
std::vector<uint8_t> dataSrc{1, 2, 3, 4};
const uint32_t kPriority{5};
const int32_t kWebAudioPlayerHandle{1};
constexpr uint32_t kPreferredFrames{1};
constexpr uint32_t kMaximumFrames{1};
constexpr bool kSupportDeferredPlay{true};
constexpr uint32_t kPartition{0};
constexpr uint32_t kBufferDelay{12};
} // namespace

namespace firebolt::rialto::client::ct
{
WebAudioPlayerTestMethods::WebAudioPlayerTestMethods(const std::vector<firebolt::rialto::WebAudioShmInfo> &webAudioShmInfo)
    : m_webAudioPlayerModuleMock{std::make_shared<StrictMock<WebAudioPlayerModuleMock>>()},
      m_webAudioPlayerClientMock{std::make_shared<StrictMock<WebAudioPlayerClientMock>>()}
{
    m_config->pcm.rate = 1;
    m_config->pcm.channels = 2;
    m_config->pcm.sampleSize = 16;
    m_config->pcm.isBigEndian = false;
    m_config->pcm.isSigned = false;
    m_config->pcm.isFloat = false;

    m_bytesPerFrame = m_config->pcm.channels * (m_config->pcm.sampleSize / CHAR_BIT);
}

void WebAudioPlayerTestMethods::shouldGetBufferAvailable()
{
    if (m_webAudioShmInfo == nullptr)
    {
        throw std::runtime_error("m_webAudioShmInfo is null");
    }

    m_webAudioShmInfo->lengthMain = 4;

    // This is used to calculate the availbaleFrames by taking the entire length of the buffer used by webaudio and
    // dividing it by bytesPerFrame to convert it into frames
    uint32_t availableFrames = (m_webAudioShmInfo->lengthMain + m_webAudioShmInfo->lengthWrap) / m_bytesPerFrame;

    EXPECT_CALL(*m_webAudioPlayerModuleMock,
                getBufferAvailable(_, webAudioGetBufferAvailableRequestMatcher(kWebAudioPlayerHandle), _, _))
        .WillOnce(
            DoAll(SetArgPointee<2>(m_webAudioPlayerModuleMock->webAudioGetBufferAvailableResponse(availableFrames,
                                                                                                  m_webAudioShmInfo)),
                  (WithArgs<0, 3>(Invoke(&(*m_webAudioPlayerModuleMock), &WebAudioPlayerModuleMock::defaultReturn)))));
}

void WebAudioPlayerTestMethods::getBufferAvailable()
{
    uint32_t availableFrames = (m_webAudioShmInfo->lengthMain + m_webAudioShmInfo->lengthWrap) / m_bytesPerFrame;

    EXPECT_EQ(m_webAudioPlayer->getBufferAvailable(availableFrames, m_webAudioShmInfo), true);
}

void WebAudioPlayerTestMethods::shouldWriteBuffer()
{
    EXPECT_CALL(*m_webAudioPlayerModuleMock,
                writeBuffer(_, webAudioWriteBufferRequestMatcher(kWebAudioPlayerHandle, kNumberOfFrames), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_webAudioPlayerModuleMock), &WebAudioPlayerModuleMock::defaultReturn)));
}

void WebAudioPlayerTestMethods::writeBuffer()
{
    EXPECT_TRUE(m_webAudioPlayer->writeBuffer(kNumberOfFrames, dataSrc.data()));
}

void WebAudioPlayerTestMethods::checkBuffer()
{
    uint8_t *dataPtr{reinterpret_cast<uint8_t *>(getShmAddress()) + m_webAudioShmInfo->offsetMain};
    size_t dataLength = kNumberOfFrames * m_bytesPerFrame;
    std::vector<uint8_t> data = std::vector<uint8_t>(dataPtr, dataPtr + dataLength);
    EXPECT_EQ(data, dataSrc);
}

void WebAudioPlayerTestMethods::shouldNotWriteBuffer()
{
    EXPECT_CALL(*m_webAudioPlayerModuleMock,
                writeBuffer(_, webAudioWriteBufferRequestMatcher(kWebAudioPlayerHandle, kNumberOfFrames), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_webAudioPlayerModuleMock), &WebAudioPlayerModuleMock::failureReturn)));
}

void WebAudioPlayerTestMethods::doesNotWriteBuffer()
{
    EXPECT_FALSE(m_webAudioPlayer->writeBuffer(kNumberOfFrames, dataSrc.data()));
}

void WebAudioPlayerTestMethods::shouldCreateWebAudioPlayer()
{
    EXPECT_CALL(*m_webAudioPlayerModuleMock,
                createWebAudioPlayer(_, createWebAudioPlayerRequestMatcher(kAudioMimeType, kPriority, m_config), _, _))
        .WillOnce(
            DoAll(SetArgPointee<2>(m_webAudioPlayerModuleMock->createWebAudioPlayerResponse(kWebAudioPlayerHandle)),
                  (WithArgs<0, 3>(Invoke(&(*m_webAudioPlayerModuleMock), &WebAudioPlayerModuleMock::defaultReturn)))));
}

void WebAudioPlayerTestMethods::createWebAudioPlayer()
{
    m_webAudioPlayerFactory = firebolt::rialto::IWebAudioPlayerFactory::createFactory();
    EXPECT_NO_THROW(m_webAudioPlayer = m_webAudioPlayerFactory->createWebAudioPlayer(m_webAudioPlayerClientMock,
                                                                                     kAudioMimeType, kPriority, m_config));
    EXPECT_NE(m_webAudioPlayer, nullptr);
}

void WebAudioPlayerTestMethods::shouldNotCreateWebAudioPlayer()
{
    EXPECT_CALL(*m_webAudioPlayerModuleMock,
                createWebAudioPlayer(_, createWebAudioPlayerRequestMatcher(kAudioMimeType, kPriority, m_config), _, _))
        .WillOnce(
            DoAll(SetArgPointee<2>(m_webAudioPlayerModuleMock->createWebAudioPlayerResponse(kWebAudioPlayerHandle)),
                  (WithArgs<0, 3>(Invoke(&(*m_webAudioPlayerModuleMock), &WebAudioPlayerModuleMock::failureReturn)))));
}

void WebAudioPlayerTestMethods::doesNotCreateWebAudioPlayer()
{
    m_webAudioPlayerFactory = firebolt::rialto::IWebAudioPlayerFactory::createFactory();
    EXPECT_NO_THROW(m_webAudioPlayer = m_webAudioPlayerFactory->createWebAudioPlayer(m_webAudioPlayerClientMock,
                                                                                     kAudioMimeType, kPriority, m_config));
    EXPECT_EQ(m_webAudioPlayer, nullptr);
}

void WebAudioPlayerTestMethods::checkWebAudioPlayerClient()
{
    EXPECT_EQ(m_webAudioPlayer->getClient().lock(), m_webAudioPlayerClientMock);
}

void WebAudioPlayerTestMethods::shouldDestroyWebAudioPlayer()
{
    EXPECT_CALL(*m_webAudioPlayerModuleMock,
                destroyWebAudioPlayer(_, destroyWebAudioPlayerRequestMatcher(kWebAudioPlayerHandle), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_webAudioPlayerModuleMock), &WebAudioPlayerModuleMock::defaultReturn)));
}

void WebAudioPlayerTestMethods::destroyWebAudioPlayer()
{
    m_webAudioPlayer.reset();
}

void WebAudioPlayerTestMethods::shouldNotifyWebAudioPlayerStateIdle()
{
    EXPECT_CALL(*m_webAudioPlayerClientMock, notifyState(WebAudioPlayerState::IDLE))
        .WillOnce(Invoke(this, &WebAudioPlayerTestMethods::notifyEvent));
}

void WebAudioPlayerTestMethods::sendNotifyWebAudioPlayerStateIdle()
{
    getServerStub()->notifyWebAudioPlayerStateEvent(kWebAudioPlayerHandle, WebAudioPlayerState::IDLE);
    waitEvent();
}

void WebAudioPlayerTestMethods::shouldNotifyWebAudioPlayerStatePause()
{
    EXPECT_CALL(*m_webAudioPlayerClientMock, notifyState(WebAudioPlayerState::PAUSED))
        .WillOnce(Invoke(this, &WebAudioPlayerTestMethods::notifyEvent));
}

void WebAudioPlayerTestMethods::sendNotifyWebAudioPlayerStatePause()
{
    getServerStub()->notifyWebAudioPlayerStateEvent(kWebAudioPlayerHandle, WebAudioPlayerState::PAUSED);
    waitEvent();
}

void WebAudioPlayerTestMethods::shouldNotifyWebAudioPlayerStatePlay()
{
    EXPECT_CALL(*m_webAudioPlayerClientMock, notifyState(WebAudioPlayerState::PLAYING))
        .WillOnce(Invoke(this, &WebAudioPlayerTestMethods::notifyEvent));
}

void WebAudioPlayerTestMethods::sendNotifyWebAudioPlayerStatePlay()
{
    getServerStub()->notifyWebAudioPlayerStateEvent(kWebAudioPlayerHandle, WebAudioPlayerState::PLAYING);
    waitEvent();
}

void WebAudioPlayerTestMethods::shouldNotifyWebAudioPlayerStateEos()
{
    EXPECT_CALL(*m_webAudioPlayerClientMock, notifyState(WebAudioPlayerState::END_OF_STREAM))
        .WillOnce(Invoke(this, &WebAudioPlayerTestMethods::notifyEvent));
}

void WebAudioPlayerTestMethods::sendNotifyWebAudioPlayerStateEos()
{
    getServerStub()->notifyWebAudioPlayerStateEvent(kWebAudioPlayerHandle, WebAudioPlayerState::END_OF_STREAM);
    waitEvent();
}

void WebAudioPlayerTestMethods::shouldNotifyWebAudioPlayerStateFailure()
{
    EXPECT_CALL(*m_webAudioPlayerClientMock, notifyState(WebAudioPlayerState::FAILURE))
        .WillOnce(Invoke(this, &WebAudioPlayerTestMethods::notifyEvent));
}

void WebAudioPlayerTestMethods::sendNotifyWebAudioPlayerStateFailure()
{
    getServerStub()->notifyWebAudioPlayerStateEvent(kWebAudioPlayerHandle, WebAudioPlayerState::FAILURE);
    waitEvent();
}

void WebAudioPlayerTestMethods::shouldGetDeviceInfo()
{
    EXPECT_CALL(*m_webAudioPlayerModuleMock,
                getDeviceInfo(_, webAudioGetDeviceInfoRequestMatcher(kWebAudioPlayerHandle), _, _))
        .WillOnce(
            DoAll(SetArgPointee<2>(m_webAudioPlayerModuleMock->webAudioGetDeviceInfoResponse(kPreferredFrames,
                                                                                             kMaximumFrames,
                                                                                             kSupportDeferredPlay)),
                  (WithArgs<0, 3>(Invoke(&(*m_webAudioPlayerModuleMock), &WebAudioPlayerModuleMock::defaultReturn)))));
}
void WebAudioPlayerTestMethods::getDeviceInfo()
{
    uint32_t preferredFrames = 0;
    uint32_t maximumFrames = 0;
    bool supportDeferredPlay = false;

    EXPECT_TRUE(m_webAudioPlayer->getDeviceInfo(preferredFrames, maximumFrames, supportDeferredPlay));
    EXPECT_EQ(kPreferredFrames, preferredFrames);
    EXPECT_EQ(kMaximumFrames, maximumFrames);
    EXPECT_EQ(kSupportDeferredPlay, true);
}

void WebAudioPlayerTestMethods::shouldPlay()
{
    EXPECT_CALL(*m_webAudioPlayerModuleMock, play(_, webAudioPlayRequestMatcher(kWebAudioPlayerHandle), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_webAudioPlayerModuleMock), &WebAudioPlayerModuleMock::defaultReturn)));
}

void WebAudioPlayerTestMethods::play()
{
    EXPECT_TRUE(m_webAudioPlayer->play());
}

void WebAudioPlayerTestMethods::shouldNotPlay()
{
    EXPECT_CALL(*m_webAudioPlayerModuleMock, play(_, webAudioPlayRequestMatcher(kWebAudioPlayerHandle), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_webAudioPlayerModuleMock), &WebAudioPlayerModuleMock::failureReturn)));
}

void WebAudioPlayerTestMethods::doesNotPlay()
{
    EXPECT_FALSE(m_webAudioPlayer->play());
}

void WebAudioPlayerTestMethods::shouldPause()
{
    EXPECT_CALL(*m_webAudioPlayerModuleMock, pause(_, webAudioPauseRequestMatcher(kWebAudioPlayerHandle), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_webAudioPlayerModuleMock), &WebAudioPlayerModuleMock::defaultReturn)));
}

void WebAudioPlayerTestMethods::pause()
{
    EXPECT_TRUE(m_webAudioPlayer->pause());
}

void WebAudioPlayerTestMethods::shouldNotPause()
{
    EXPECT_CALL(*m_webAudioPlayerModuleMock, pause(_, webAudioPauseRequestMatcher(kWebAudioPlayerHandle), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_webAudioPlayerModuleMock), &WebAudioPlayerModuleMock::failureReturn)));
}

void WebAudioPlayerTestMethods::doesNotPause()
{
    EXPECT_FALSE(m_webAudioPlayer->pause());
}

void WebAudioPlayerTestMethods::shouldEos()
{
    EXPECT_CALL(*m_webAudioPlayerModuleMock, setEos(_, webAudioSetEosRequestMatcher(kWebAudioPlayerHandle), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_webAudioPlayerModuleMock), &WebAudioPlayerModuleMock::defaultReturn)));
}

void WebAudioPlayerTestMethods::setEos()
{
    EXPECT_EQ(m_webAudioPlayer->setEos(), true);
}

void WebAudioPlayerTestMethods::shouldGetBufferDelay()
{
    EXPECT_CALL(*m_webAudioPlayerModuleMock,
                getBufferDelay(_, webAudioGetBufferDelayRequestMatcher(kWebAudioPlayerHandle), _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_webAudioPlayerModuleMock->webAudioGetBufferDelayResponse(kBufferDelay)),
                        WithArgs<0, 3>(Invoke(&(*m_webAudioPlayerModuleMock), &WebAudioPlayerModuleMock::defaultReturn))));
}

void WebAudioPlayerTestMethods::getBufferDelay()
{
    uint32_t bufferDelay{51};
    EXPECT_EQ(m_webAudioPlayer->getBufferDelay(bufferDelay), true);
    EXPECT_EQ(kBufferDelay, bufferDelay);
}

void WebAudioPlayerTestMethods::shouldSetVolume(const double expectedVolume)
{
    EXPECT_CALL(*m_webAudioPlayerModuleMock,
                setVolume(_, webAudioSetVolumeRequestMatcher(kWebAudioPlayerHandle, expectedVolume), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_webAudioPlayerModuleMock), &WebAudioPlayerModuleMock::defaultReturn)));
}

void WebAudioPlayerTestMethods::shouldGetVolume(const double volume)
{
    EXPECT_CALL(*m_webAudioPlayerModuleMock, getVolume(_, webAudioGetVolumeRequestMatcher(kWebAudioPlayerHandle), _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_webAudioPlayerModuleMock->webAudioGetVolumeResponse(volume)),
                        WithArgs<0, 3>(Invoke(&(*m_webAudioPlayerModuleMock), &WebAudioPlayerModuleMock::defaultReturn))));
}

void WebAudioPlayerTestMethods::setVolume(const double volume)
{
    EXPECT_EQ(m_webAudioPlayer->setVolume(volume), true);
}

void WebAudioPlayerTestMethods::getVolume(const double expectedVolume)
{
    double returnVolume;
    EXPECT_EQ(m_webAudioPlayer->getVolume(returnVolume), true);
    EXPECT_EQ(returnVolume, expectedVolume);
}

WebAudioPlayerTestMethods::~WebAudioPlayerTestMethods() {}

} // namespace firebolt::rialto::client::ct
