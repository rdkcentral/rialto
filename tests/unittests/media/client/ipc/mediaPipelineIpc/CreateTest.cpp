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

#include "MediaPipelineIpcTestBase.h"

MATCHER_P2(createSessionRequestMatcher, maxWidth, maxHeight, "")
{
    const ::firebolt::rialto::CreateSessionRequest *request =
        dynamic_cast<const ::firebolt::rialto::CreateSessionRequest *>(arg);
    return ((request->max_width() == maxWidth) && (request->max_height() == maxHeight));
}

MATCHER_P(destroySessionRequestMatcher, sessionId, "")
{
    const ::firebolt::rialto::DestroySessionRequest *request =
        dynamic_cast<const ::firebolt::rialto::DestroySessionRequest *>(arg);
    return (request->session_id() == sessionId);
}

class RialtoClientCreateMediaPipelineIpcTest : public MediaPipelineIpcTestBase
{
protected:
    VideoRequirements m_videoReq = {321u, 432u};
    std::unique_ptr<StrictMock<EventThreadMock>> m_eventThreadMock = std::make_unique<StrictMock<EventThreadMock>>();

    virtual void SetUp() { MediaPipelineIpcTestBase::SetUp(); }

    virtual void TearDown() { MediaPipelineIpcTestBase::TearDown(); }
};

/**
 * Test that a MediaPipelineIpc object can be created successfully.
 */
TEST_F(RialtoClientCreateMediaPipelineIpcTest, CreateDestroy)
{
    /* create media player */
    expectInitIpc();
    expectSubscribeEvents();
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_eventThreadFactoryMock, createEventThread(_)).WillOnce(Return(ByMove(std::move(m_eventThread))));
    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("createSession"), m_controllerMock.get(),
                                           createSessionRequestMatcher(m_videoReq.maxWidth, m_videoReq.maxHeight), _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &MediaPipelineIpcTestBase::setCreateSessionResponse)));

    EXPECT_NO_THROW(m_mediaPipelineIpc = std::make_unique<MediaPipelineIpc>(m_clientMock, m_videoReq, *m_ipcClientMock,
                                                                            m_eventThreadFactoryMock));
    EXPECT_NE(m_mediaPipelineIpc, nullptr);

    /* destroy media player */
    expectIpcApiCallSuccess();
    expectUnsubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("destroySession"), m_controllerMock.get(),
                                           destroySessionRequestMatcher(m_sessionId), _, m_blockingClosureMock.get()));

    m_mediaPipelineIpc.reset();
    EXPECT_EQ(m_mediaPipelineIpc, nullptr);
}

/**
 * Test that a MediaPipelineIpc object can be created successfully with channel reconnection.
 */
TEST_F(RialtoClientCreateMediaPipelineIpcTest, CreateDestroyWithReconnection)
{
    /* create media player */
    expectInitIpcWithReconnection();
    expectSubscribeEvents();
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_eventThreadFactoryMock, createEventThread(_)).WillOnce(Return(ByMove(std::move(m_eventThread))));
    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("createSession"), m_controllerMock.get(),
                                           createSessionRequestMatcher(m_videoReq.maxWidth, m_videoReq.maxHeight), _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &MediaPipelineIpcTestBase::setCreateSessionResponse)));

    EXPECT_NO_THROW(m_mediaPipelineIpc = std::make_unique<MediaPipelineIpc>(m_clientMock, m_videoReq, *m_ipcClientMock,
                                                                            m_eventThreadFactoryMock));
    EXPECT_NE(m_mediaPipelineIpc, nullptr);

    /* destroy media player */
    expectIpcApiCallSuccess();
    expectUnsubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("destroySession"), m_controllerMock.get(),
                                           destroySessionRequestMatcher(m_sessionId), _, m_blockingClosureMock.get()));

    m_mediaPipelineIpc.reset();
    EXPECT_EQ(m_mediaPipelineIpc, nullptr);
}

/**
 * Test the factory
 */
TEST_F(RialtoClientCreateMediaPipelineIpcTest, FactoryCreatesObject)
{
    std::shared_ptr<firebolt::rialto::client::IMediaPipelineIpcFactory> factory =
        firebolt::rialto::client::IMediaPipelineIpcFactory::getFactory();
    EXPECT_NE(factory, nullptr);

    /* create media player */
    expectInitIpc();
    expectSubscribeEvents();
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("createSession"), m_controllerMock.get(),
                                           createSessionRequestMatcher(m_videoReq.maxWidth, m_videoReq.maxHeight), _,
                                           m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &MediaPipelineIpcTestBase::setCreateSessionResponse)));

    // Call the factory
    EXPECT_NO_THROW(m_mediaPipelineIpc = factory->createMediaPipelineIpc(m_clientMock, m_videoReq, m_ipcClientMock));
    EXPECT_NE(m_mediaPipelineIpc, nullptr);

    /* destroy media player */
    expectIpcApiCallSuccess();
    expectUnsubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("destroySession"), m_controllerMock.get(),
                                           destroySessionRequestMatcher(m_sessionId), _, m_blockingClosureMock.get()));

    m_mediaPipelineIpc.reset();
    EXPECT_EQ(m_mediaPipelineIpc, nullptr);
}

/**
 * Test that a MediaPipelineIpc object not created when the ipc channel has not been created.
 */
TEST_F(RialtoClientCreateMediaPipelineIpcTest, CreateNoIpcChannel)
{
    expectInitIpcButAttachChannelFailure();
    EXPECT_CALL(*m_eventThreadFactoryMock, createEventThread(_)).WillOnce(Return(ByMove(std::move(m_eventThread))));

    EXPECT_THROW(m_mediaPipelineIpc = std::make_unique<MediaPipelineIpc>(m_clientMock, m_videoReq, *m_ipcClientMock,
                                                                         m_eventThreadFactoryMock),
                 std::runtime_error);
}

/**
 * Test that a MediaPipelineIpc object not created when the ipc channel has not been created after reconnection.
 */
TEST_F(RialtoClientCreateMediaPipelineIpcTest, CreateNoIpcChannelAfterReconnect)
{
    expectInitIpcButNotConnectedChannelAfterReconnect();
    EXPECT_CALL(*m_eventThreadFactoryMock, createEventThread(_)).WillOnce(Return(ByMove(std::move(m_eventThread))));

    EXPECT_THROW(m_mediaPipelineIpc = std::make_unique<MediaPipelineIpc>(m_clientMock, m_videoReq, *m_ipcClientMock,
                                                                         m_eventThreadFactoryMock),
                 std::runtime_error);
}

/**
 * Test that a MediaPipelineIpc object not created when the ipc channel is not connected.
 */
TEST_F(RialtoClientCreateMediaPipelineIpcTest, CreateIpcChannelDisconnected)
{
    EXPECT_CALL(*m_eventThreadFactoryMock, createEventThread(_)).WillOnce(Return(ByMove(std::move(m_eventThread))));
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock));
    EXPECT_CALL(*m_channelMock, isConnected()).WillOnce(Return(false));
    EXPECT_CALL(*m_ipcClientMock, reconnect()).WillOnce(Return(false));

    EXPECT_THROW(m_mediaPipelineIpc = std::make_unique<MediaPipelineIpc>(m_clientMock, m_videoReq, *m_ipcClientMock,
                                                                         m_eventThreadFactoryMock),
                 std::runtime_error);
}

/**
 * Test that a MediaPipelineIpc object not created when subscribing to events fails, and any event subscribed
 * are unsubscribed.
 */
TEST_F(RialtoClientCreateMediaPipelineIpcTest, SubscribeEventFailure)
{
    expectInitIpc();
    EXPECT_CALL(*m_eventThreadFactoryMock, createEventThread(_)).WillOnce(Return(ByMove(std::move(m_eventThread))));

    EXPECT_CALL(*m_channelMock, subscribeImpl("firebolt.rialto.PlaybackStateChangeEvent", _, _))
        .WillOnce(Invoke(
            [this](const std::string &eventName, const google::protobuf::Descriptor *descriptor,
                   std::function<void(const std::shared_ptr<google::protobuf::Message> &msg)> &&handler)
            {
                m_playbackStateCb = std::move(handler);
                return static_cast<int>(EventTags::PlaybackStateChangeEvent);
            }));
    EXPECT_CALL(*m_channelMock, subscribeImpl("firebolt.rialto.PositionChangeEvent", _, _)).WillOnce(Return(-1));
    EXPECT_CALL(*m_channelMock, unsubscribe(static_cast<int>(EventTags::PlaybackStateChangeEvent)));

    EXPECT_THROW(m_mediaPipelineIpc = std::make_unique<MediaPipelineIpc>(m_clientMock, m_videoReq, *m_ipcClientMock,
                                                                         m_eventThreadFactoryMock),
                 std::runtime_error);
}

/**
 * Test that a MediaPipelineIpc object not created when create session fails.
 */
TEST_F(RialtoClientCreateMediaPipelineIpcTest, CreateSessionFailure)
{
    expectInitIpc();
    expectSubscribeEvents();
    expectIpcApiCallFailure();
    expectUnsubscribeEvents();

    EXPECT_CALL(*m_eventThreadFactoryMock, createEventThread(_)).WillOnce(Return(ByMove(std::move(m_eventThread))));
    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("createSession"), _, _, _, _));

    EXPECT_THROW(m_mediaPipelineIpc = std::make_unique<MediaPipelineIpc>(m_clientMock, m_videoReq, *m_ipcClientMock,
                                                                         m_eventThreadFactoryMock),
                 std::runtime_error);
}

/**
 * Test that when destroy session fails, the MediaPipelineIpc object is still destroyed.
 */
TEST_F(RialtoClientCreateMediaPipelineIpcTest, DestroySessionFailure)
{
    /* create media player */
    createMediaPipelineIpc();

    /* destroy media player */
    expectUnsubscribeEvents();
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("destroySession"), _, _, _, _));

    m_mediaPipelineIpc.reset();
    EXPECT_EQ(m_mediaPipelineIpc, nullptr);
}

/**
 * Test that when the ipc channel is disconnected, the MediaPipelineIpc object is still destroyed.
 */
TEST_F(RialtoClientCreateMediaPipelineIpcTest, DestructorChannelDisconnected)
{
    /* create media player */
    createMediaPipelineIpc();

    /* destroy media player */
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    m_mediaPipelineIpc.reset();
    EXPECT_EQ(m_mediaPipelineIpc, nullptr);
}
