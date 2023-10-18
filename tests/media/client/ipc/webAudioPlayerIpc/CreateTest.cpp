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
#include "webaudioplayermodule.pb.h"

MATCHER_P3(createWebAudioPlayerRequestMatcher, audioMimeType, priority, config, "")
{
    const ::firebolt::rialto::CreateWebAudioPlayerRequest *request =
        dynamic_cast<const ::firebolt::rialto::CreateWebAudioPlayerRequest *>(arg);
    return ((request->audio_mime_type() == audioMimeType) && (request->priority() == priority));
}

MATCHER_P(destroyWebAudioPlayerRequestMatcher, web_audio_player_handle, "")
{
    const ::firebolt::rialto::DestroyWebAudioPlayerRequest *request =
        dynamic_cast<const ::firebolt::rialto::DestroyWebAudioPlayerRequest *>(arg);
    return (request->web_audio_player_handle() == web_audio_player_handle);
}

class RialtoClientCreateWebAudioPlayerIpcTest : public WebAudioPlayerIpcTestBase
{
protected:
    VideoRequirements m_videoReq = {321u, 432u};
    std::unique_ptr<StrictMock<EventThreadMock>> m_eventThreadMock = std::make_unique<StrictMock<EventThreadMock>>();

    virtual void SetUp() { WebAudioPlayerIpcTestBase::SetUp(); }

    virtual void TearDown() { WebAudioPlayerIpcTestBase::TearDown(); }
};

/**
 * Test that a WebAudioPlayerIpc object can be created successfully.
 */
TEST_F(RialtoClientCreateWebAudioPlayerIpcTest, CreateDestroy)
{
    /* create media player */
    expectInitIpc();
    expectSubscribeEvents();
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_eventThreadFactoryMock, createEventThread(_)).WillOnce(Return(ByMove(std::move(m_eventThread))));
    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("createWebAudioPlayer"), m_controllerMock.get(),
                                           createWebAudioPlayerRequestMatcher(m_audioMimeType, m_priority, &m_config),
                                           _, m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &WebAudioPlayerIpcTestBase::setCreateWebAudioPlayerResponse)));

    EXPECT_NO_THROW(m_webAudioPlayerIpc = std::make_unique<WebAudioPlayerIpc>(m_clientMock, m_audioMimeType, m_priority,
                                                                              &m_config, m_ipcClientMock,
                                                                              m_eventThreadFactoryMock));
    EXPECT_NE(m_webAudioPlayerIpc, nullptr);

    /* destroy media player */
    expectIpcApiCallSuccess();
    expectUnsubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("destroyWebAudioPlayer"), m_controllerMock.get(),
                                           destroyWebAudioPlayerRequestMatcher(m_webAaudioPlayerHandle), _,
                                           m_blockingClosureMock.get()));

    m_webAudioPlayerIpc.reset();
    EXPECT_EQ(m_webAudioPlayerIpc, nullptr);
}

/**
 * Test that a WebAudioPlayerIpc object not created when the ipc channel has not been created.
 */
TEST_F(RialtoClientCreateWebAudioPlayerIpcTest, CreateNoIpcChannel)
{
    expectInitIpcButAttachChannelFailure();
    EXPECT_CALL(*m_eventThreadFactoryMock, createEventThread(_)).WillOnce(Return(ByMove(std::move(m_eventThread))));

    EXPECT_THROW(m_webAudioPlayerIpc = std::make_unique<WebAudioPlayerIpc>(m_clientMock, m_audioMimeType, m_priority,
                                                                           &m_config, m_ipcClientMock,
                                                                           m_eventThreadFactoryMock),
                 std::runtime_error);
}

/**
 * Test that a WebAudioPlayerIpc object not created when the ipc channel is not connected.
 */
TEST_F(RialtoClientCreateWebAudioPlayerIpcTest, CreateIpcChannelDisconnected)
{
    EXPECT_CALL(*m_eventThreadFactoryMock, createEventThread(_)).WillOnce(Return(ByMove(std::move(m_eventThread))));
    EXPECT_CALL(m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock));
    EXPECT_CALL(*m_channelMock, isConnected()).WillOnce(Return(false));
    EXPECT_CALL(m_ipcClientMock, reconnect()).WillOnce(Return(false));

    EXPECT_THROW(m_webAudioPlayerIpc = std::make_unique<WebAudioPlayerIpc>(m_clientMock, m_audioMimeType, m_priority,
                                                                           &m_config, m_ipcClientMock,
                                                                           m_eventThreadFactoryMock),
                 std::runtime_error);
}

/**
 * Test that a WebAudioPlayerIpc object not created when subscribing to events fails
 */
TEST_F(RialtoClientCreateWebAudioPlayerIpcTest, SubscribeEventFailure)
{
    expectInitIpc();
    EXPECT_CALL(*m_eventThreadFactoryMock, createEventThread(_)).WillOnce(Return(ByMove(std::move(m_eventThread))));

    EXPECT_CALL(*m_channelMock, subscribeImpl("firebolt.rialto.WebAudioPlayerStateEvent", _, _)).WillOnce(Return(-1));

    EXPECT_THROW(m_webAudioPlayerIpc = std::make_unique<WebAudioPlayerIpc>(m_clientMock, m_audioMimeType, m_priority,
                                                                           &m_config, m_ipcClientMock,
                                                                           m_eventThreadFactoryMock),
                 std::runtime_error);
}

/**
 * Test that a WebAudioPlayerIpc object not created when create session fails.
 */
TEST_F(RialtoClientCreateWebAudioPlayerIpcTest, CreateSessionFailure)
{
    expectInitIpc();
    expectSubscribeEvents();
    expectIpcApiCallFailure();
    expectUnsubscribeEvents();

    EXPECT_CALL(*m_eventThreadFactoryMock, createEventThread(_)).WillOnce(Return(ByMove(std::move(m_eventThread))));
    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("createWebAudioPlayer"), _, _, _, _));

    EXPECT_THROW(m_webAudioPlayerIpc = std::make_unique<WebAudioPlayerIpc>(m_clientMock, m_audioMimeType, m_priority,
                                                                           &m_config, m_ipcClientMock,
                                                                           m_eventThreadFactoryMock),
                 std::runtime_error);
}

/**
 * Test that when destroy session fails, the WebAudioPlayerIpc object is still destroyed.
 */
TEST_F(RialtoClientCreateWebAudioPlayerIpcTest, DestroySessionFailure)
{
    /* create media player */
    createWebAudioPlayerIpc();

    /* destroy media player */
    expectUnsubscribeEvents();
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("destroyWebAudioPlayer"), _, _, _, _));

    m_webAudioPlayerIpc.reset();
    EXPECT_EQ(m_webAudioPlayerIpc, nullptr);
}

/**
 * Test that when the ipc channel is disconnected, the WebAudioPlayerIpc object is still destroyed.
 */
TEST_F(RialtoClientCreateWebAudioPlayerIpcTest, DestructorChannelDisconnected)
{
    /* create media player */
    createWebAudioPlayerIpc();

    /* destroy media player */
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    m_webAudioPlayerIpc.reset();
    EXPECT_EQ(m_webAudioPlayerIpc, nullptr);
}
