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
#include "MediaPipelineStructureMatchers.h"

class RialtoClientMediaPipelineIpcCallbackTest : public MediaPipelineIpcTestBase
{
protected:
    int32_t m_sourceId = 1;
    QosInfo m_qosInfo = {5U, 2U};

    virtual void SetUp()
    {
        MediaPipelineIpcTestBase::SetUp();

        createMediaPipelineIpc();
    }

    virtual void TearDown()
    {
        destroyMediaPipelineIpc();

        MediaPipelineIpcTestBase::TearDown();
    }
};

/**
 * Test that a playback state update over IPC is forwarded to the client.
 */
TEST_F(RialtoClientMediaPipelineIpcCallbackTest, NotifyPlaybackState)
{
    auto updatePlaybackStateEvent = std::make_shared<firebolt::rialto::PlaybackStateChangeEvent>();
    updatePlaybackStateEvent->set_session_id(m_sessionId);
    updatePlaybackStateEvent->set_state(firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_PLAYING);

    EXPECT_CALL(*m_eventThreadMock, addImpl(_)).WillOnce(Invoke([](std::function<void()> &&func) { func(); }));
    EXPECT_CALL(*m_clientMock, notifyPlaybackState(PlaybackState::PLAYING));

    m_playbackStateCb(updatePlaybackStateEvent);
}

/**
 * Test that if the session id of the event is not the same as the playback session the event will be ignored.
 */
TEST_F(RialtoClientMediaPipelineIpcCallbackTest, InvalidSessionIdPlaybackState)
{
    auto updatePlaybackStateEvent = std::make_shared<firebolt::rialto::PlaybackStateChangeEvent>();
    updatePlaybackStateEvent->set_session_id(-1);
    updatePlaybackStateEvent->set_state(firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_PLAYING);

    EXPECT_CALL(*m_eventThreadMock, addImpl(_)).WillOnce(Invoke([](std::function<void()> &&func) { func(); }));

    m_playbackStateCb(updatePlaybackStateEvent);
}

/**
 * Test that a network state update over IPC is forwarded to the client.
 */
TEST_F(RialtoClientMediaPipelineIpcCallbackTest, NotifyNetworkState)
{
    auto updateNetworkStateEvent = std::make_shared<firebolt::rialto::NetworkStateChangeEvent>();
    updateNetworkStateEvent->set_session_id(m_sessionId);
    updateNetworkStateEvent->set_state(firebolt::rialto::NetworkStateChangeEvent_NetworkState_BUFFERING);

    EXPECT_CALL(*m_eventThreadMock, addImpl(_)).WillOnce(Invoke([](std::function<void()> &&func) { func(); }));

    EXPECT_CALL(*m_clientMock, notifyNetworkState(NetworkState::BUFFERING));

    m_networkStateCb(updateNetworkStateEvent);
}

/**
 * Test that if the session id of the event is not the same as the playback session the event will be ignored.
 */
TEST_F(RialtoClientMediaPipelineIpcCallbackTest, InvalidSessionIdNetworkState)
{
    auto updateNetworkStateEvent = std::make_shared<firebolt::rialto::NetworkStateChangeEvent>();
    updateNetworkStateEvent->set_session_id(-1);
    updateNetworkStateEvent->set_state(firebolt::rialto::NetworkStateChangeEvent_NetworkState_BUFFERING);

    EXPECT_CALL(*m_eventThreadMock, addImpl(_)).WillOnce(Invoke([](std::function<void()> &&func) { func(); }));

    m_networkStateCb(updateNetworkStateEvent);
}

/**
 * Test that a qos notification over IPC is forwarded to the client.
 */
TEST_F(RialtoClientMediaPipelineIpcCallbackTest, NotifyQos)
{
    auto updateQosEvent = std::make_shared<firebolt::rialto::QosEvent>();
    updateQosEvent->set_session_id(m_sessionId);
    updateQosEvent->set_source_id(m_sourceId);
    updateQosEvent->mutable_qos_info()->set_processed(m_qosInfo.processed);
    updateQosEvent->mutable_qos_info()->set_dropped(m_qosInfo.dropped);

    EXPECT_CALL(*m_eventThreadMock, addImpl(_)).WillOnce(Invoke([](std::function<void()> &&func) { func(); }));

    EXPECT_CALL(*m_clientMock, notifyQos(m_sourceId, qosInfoMatcher(m_qosInfo)));

    m_qosCb(updateQosEvent);
}

/**
 * Test that if the session id of the event is not the same as the playback session the event will be ignored.
 */
TEST_F(RialtoClientMediaPipelineIpcCallbackTest, InvalidSessionIdQos)
{
    auto updateQosEvent = std::make_shared<firebolt::rialto::QosEvent>();
    updateQosEvent->set_session_id(-1);
    updateQosEvent->set_source_id(m_sourceId);
    updateQosEvent->mutable_qos_info()->set_processed(m_qosInfo.processed);
    updateQosEvent->mutable_qos_info()->set_dropped(m_qosInfo.dropped);

    EXPECT_CALL(*m_eventThreadMock, addImpl(_)).WillOnce(Invoke([](std::function<void()> &&func) { func(); }));

    m_qosCb(updateQosEvent);
}

/**
 * Test that a playback error notification over IPC is forwarded to the client.
 */
TEST_F(RialtoClientMediaPipelineIpcCallbackTest, NotifyPlaybackError)
{
    auto updatePlaybackErrorEvent = std::make_shared<firebolt::rialto::PlaybackErrorEvent>();
    updatePlaybackErrorEvent->set_session_id(m_sessionId);
    updatePlaybackErrorEvent->set_source_id(m_sourceId);
    updatePlaybackErrorEvent->set_error(firebolt::rialto::PlaybackErrorEvent_PlaybackError_DECRYPTION);

    EXPECT_CALL(*m_eventThreadMock, addImpl(_)).WillOnce(Invoke([](std::function<void()> &&func) { func(); }));

    EXPECT_CALL(*m_clientMock, notifyPlaybackError(m_sourceId, PlaybackError::DECRYPTION));

    m_playbackErrorCb(updatePlaybackErrorEvent);
}

/**
 * Test that if the session id of the event is not the same as the playback session the event will be ignored.
 */
TEST_F(RialtoClientMediaPipelineIpcCallbackTest, InvalidSessionIdPlaybackError)
{
    auto updatePlaybackErrorEvent = std::make_shared<firebolt::rialto::PlaybackErrorEvent>();
    updatePlaybackErrorEvent->set_session_id(-1);
    updatePlaybackErrorEvent->set_source_id(m_sourceId);
    updatePlaybackErrorEvent->set_error(firebolt::rialto::PlaybackErrorEvent_PlaybackError_DECRYPTION);

    EXPECT_CALL(*m_eventThreadMock, addImpl(_)).WillOnce(Invoke([](std::function<void()> &&func) { func(); }));

    m_playbackErrorCb(updatePlaybackErrorEvent);
}
