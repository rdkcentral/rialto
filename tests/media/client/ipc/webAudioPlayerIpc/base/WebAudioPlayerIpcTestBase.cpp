/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Sky UK
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

#include "WebAudioPlayerIpcTestBase.h"
#include <memory>
#include <string>
#include <utility>

void WebAudioPlayerIpcTestBase::SetUp() // NOLINT(build/function_format)
{
    // Create StrictMocks
    m_clientMock = new StrictMock<WebAudioPlayerIpcClientMock>();
    m_eventThreadFactoryMock = std::make_shared<StrictMock<EventThreadFactoryMock>>();
    m_eventThread = std::make_unique<StrictMock<EventThreadMock>>();
    m_eventThreadMock = m_eventThread.get();
}

void WebAudioPlayerIpcTestBase::TearDown() // NOLINT(build/function_format)
{
    // Destroy StrickMocks
    m_eventThreadFactoryMock.reset();
    m_eventThreadMock = nullptr;
    delete m_clientMock;
}

void WebAudioPlayerIpcTestBase::createWebAudioPlayerIpc()
{
    expectInitIpc();
    expectSubscribeEvents();
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_eventThreadFactoryMock, createEventThread(_)).WillOnce(Return(ByMove(std::move(m_eventThread))));
    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("createWebAudioPlayer"), _, _, _, _))
        .WillOnce(WithArgs<3>(Invoke(this, &WebAudioPlayerIpcTestBase::setCreateWebAudioPlayerResponse)));

    EXPECT_NO_THROW(m_webAudioPlayerIpc = std::make_unique<WebAudioPlayerIpc>(m_clientMock, m_audioMimeType, m_priority,
                                                                              &m_config, m_ipcClientMock,
                                                                              m_eventThreadFactoryMock));
    EXPECT_NE(m_webAudioPlayerIpc, nullptr);
}

void WebAudioPlayerIpcTestBase::expectSubscribeEvents()
{
    EXPECT_CALL(*m_channelMock, subscribeImpl("firebolt.rialto.WebAudioPlayerStateEvent", _, _))
        .WillOnce(Invoke(
            [this](const std::string &eventName, const google::protobuf::Descriptor *descriptor,
                   std::function<void(const std::shared_ptr<google::protobuf::Message> &msg)> &&handler)
            {
                m_notifyStateCb = std::move(handler);
                return static_cast<int>(EventTags::notifyStateEvent);
            }))
        .RetiresOnSaturation();
}

void WebAudioPlayerIpcTestBase::expectUnsubscribeEvents()
{
    EXPECT_CALL(*m_channelMock, unsubscribe(static_cast<int>(EventTags::notifyStateEvent))).WillOnce(Return(true));
}

void WebAudioPlayerIpcTestBase::destroyWebAudioPlayerIpc()
{
    expectIpcApiCallSuccess();
    expectUnsubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("destroyWebAudioPlayer"), _, _, _, _));

    m_webAudioPlayerIpc.reset();
    EXPECT_EQ(m_webAudioPlayerIpc, nullptr);
}

void WebAudioPlayerIpcTestBase::setCreateWebAudioPlayerResponse(google::protobuf::Message *response)
{
    firebolt::rialto::CreateWebAudioPlayerResponse *createWebAudioPlayerResponse =
        dynamic_cast<firebolt::rialto::CreateWebAudioPlayerResponse *>(response);
    createWebAudioPlayerResponse->set_web_audio_player_handle(m_webAaudioPlayerHandle);
}
