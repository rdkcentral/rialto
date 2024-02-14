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
constexpr uint32_t kPriority{5};
constexpr int32_t kWebAudioPlayerHandle{1};
constexpr uint32_t kPreferredFrames{1};
constexpr uint32_t kMaximumFrames{0};
constexpr bool kSupportDeferredPlay{true};
} // namespace

namespace firebolt::rialto::client::ct
{
WebAudioPlayerTestMethods::WebAudioPlayerTestMethods()
    : m_webAudioPlayerModuleMock{std::make_shared<StrictMock<WebAudioPlayerModuleMock>>()},
      m_webAudioPlayerClientMock{std::make_shared<StrictMock<WebAudioPlayerClientMock>>()}
{
}

void WebAudioPlayerTestMethods::shouldCreateWebAudioPlayer()
{
    m_config->pcm.rate = 1;
    m_config->pcm.channels = 2;
    m_config->pcm.sampleSize = 16;
    m_config->pcm.isBigEndian = false;
    m_config->pcm.isSigned = false;
    m_config->pcm.isFloat = false;

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
    bool supportDeferredPlaye = true;

    EXPECT_TRUE(m_webAudioPlayer->getDeviceInfo(preferredFrames, maximumFrames, supportDeferredPlaye));
    EXPECT_EQ(kPreferredFrames, preferredFrames);
    EXPECT_EQ(kMaximumFrames, maximumFrames);
    EXPECT_EQ(kSupportDeferredPlay, true);
}

WebAudioPlayerTestMethods::~WebAudioPlayerTestMethods() {}

} // namespace firebolt::rialto::client::ct
