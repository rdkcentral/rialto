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
#include <memory>
#include <string>
#include <utility>

void MediaPipelineIpcTestBase::SetUp() // NOLINT(build/function_format)
{
    // Create StrictMocks
    m_clientMock = new StrictMock<MediaPipelineIpcClientMock>();
    m_eventThreadFactoryMock = std::make_shared<StrictMock<EventThreadFactoryMock>>();
    m_eventThread = std::make_unique<StrictMock<EventThreadMock>>();
    m_eventThreadMock = m_eventThread.get();
}

void MediaPipelineIpcTestBase::TearDown() // NOLINT(build/function_format)
{
    // Destroy StrickMocks
    m_eventThreadFactoryMock.reset();
    m_eventThreadMock = nullptr;
    delete m_clientMock;
}

void MediaPipelineIpcTestBase::createMediaPipelineIpc()
{
    VideoRequirements videoReq = {};

    expectInitIpc();
    expectSubscribeEvents();
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_eventThreadFactoryMock, createEventThread(_)).WillOnce(Return(ByMove(std::move(m_eventThread))));
    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("createSession"), _, _, _, _))
        .WillOnce(WithArgs<3>(Invoke(this, &MediaPipelineIpcTestBase::setCreateSessionResponse)));

    EXPECT_NO_THROW(m_mediaPipelineIpc = std::make_unique<MediaPipelineIpc>(m_clientMock, videoReq, m_ipcClientMock,
                                                                            m_eventThreadFactoryMock));
    EXPECT_NE(m_mediaPipelineIpc, nullptr);
}

void MediaPipelineIpcTestBase::expectSubscribeEvents()
{
    EXPECT_CALL(*m_channelMock, subscribeImpl("firebolt.rialto.PlaybackStateChangeEvent", _, _))
        .WillOnce(Invoke(
            [this](const std::string &eventName, const google::protobuf::Descriptor *descriptor,
                   std::function<void(const std::shared_ptr<google::protobuf::Message> &msg)> &&handler)
            {
                m_playbackStateCb = std::move(handler);
                return static_cast<int>(EventTags::PlaybackStateChangeEvent);
            }))
        .RetiresOnSaturation();
    EXPECT_CALL(*m_channelMock, subscribeImpl("firebolt.rialto.PositionChangeEvent", _, _))
        .WillOnce(Invoke(
            [this](const std::string &eventName, const google::protobuf::Descriptor *descriptor,
                   std::function<void(const std::shared_ptr<google::protobuf::Message> &msg)> &&handler)
            {
                m_positionChangeCb = std::move(handler);
                return static_cast<int>(EventTags::PositionChangeEvent);
            }))
        .RetiresOnSaturation();
    EXPECT_CALL(*m_channelMock, subscribeImpl("firebolt.rialto.NetworkStateChangeEvent", _, _))
        .WillOnce(Invoke(
            [this](const std::string &eventName, const google::protobuf::Descriptor *descriptor,
                   std::function<void(const std::shared_ptr<google::protobuf::Message> &msg)> &&handler)
            {
                m_networkStateCb = std::move(handler);
                return static_cast<int>(EventTags::NetworkStateChangeEvent);
            }))
        .RetiresOnSaturation();
    EXPECT_CALL(*m_channelMock, subscribeImpl("firebolt.rialto.NeedMediaDataEvent", _, _))
        .WillOnce(Invoke(
            [this](const std::string &eventName, const google::protobuf::Descriptor *descriptor,
                   std::function<void(const std::shared_ptr<google::protobuf::Message> &msg)> &&handler)
            {
                m_needDataCb = std::move(handler);
                return static_cast<int>(EventTags::NeedMediaDataEvent);
            }))
        .RetiresOnSaturation();
    EXPECT_CALL(*m_channelMock, subscribeImpl("firebolt.rialto.QosEvent", _, _))
        .WillOnce(Invoke(
            [this](const std::string &eventName, const google::protobuf::Descriptor *descriptor,
                   std::function<void(const std::shared_ptr<google::protobuf::Message> &msg)> &&handler)
            {
                m_qosCb = std::move(handler);
                return static_cast<int>(EventTags::QosEvent);
            }))
        .RetiresOnSaturation();
    EXPECT_CALL(*m_channelMock, subscribeImpl("firebolt.rialto.BufferUnderflowEvent", _, _))
        .WillOnce(Invoke(
            [this](const std::string &eventName, const google::protobuf::Descriptor *descriptor,
                   std::function<void(const std::shared_ptr<google::protobuf::Message> &msg)> &&handler)
            {
                m_bufferUnderflowCb = std::move(handler);
                return static_cast<int>(EventTags::BufferUnderflowEvent);
            }))
        .RetiresOnSaturation();
}

void MediaPipelineIpcTestBase::expectUnsubscribeEvents()
{
    EXPECT_CALL(*m_channelMock, unsubscribe(static_cast<int>(EventTags::PlaybackStateChangeEvent))).WillOnce(Return(true));
    EXPECT_CALL(*m_channelMock, unsubscribe(static_cast<int>(EventTags::PositionChangeEvent))).WillOnce(Return(true));
    EXPECT_CALL(*m_channelMock, unsubscribe(static_cast<int>(EventTags::NetworkStateChangeEvent))).WillOnce(Return(true));
    EXPECT_CALL(*m_channelMock, unsubscribe(static_cast<int>(EventTags::NeedMediaDataEvent))).WillOnce(Return(true));
    EXPECT_CALL(*m_channelMock, unsubscribe(static_cast<int>(EventTags::QosEvent))).WillOnce(Return(true));
    EXPECT_CALL(*m_channelMock, unsubscribe(static_cast<int>(EventTags::BufferUnderflowEvent))).WillOnce(Return(true));
}

void MediaPipelineIpcTestBase::destroyMediaPipelineIpc()
{
    expectIpcApiCallSuccess();
    expectUnsubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("destroySession"), _, _, _, _));

    m_mediaPipelineIpc.reset();
    EXPECT_EQ(m_mediaPipelineIpc, nullptr);
}

void MediaPipelineIpcTestBase::setCreateSessionResponse(google::protobuf::Message *response)
{
    firebolt::rialto::CreateSessionResponse *createSessionResponse =
        dynamic_cast<firebolt::rialto::CreateSessionResponse *>(response);
    createSessionResponse->set_session_id(m_sessionId);
}
