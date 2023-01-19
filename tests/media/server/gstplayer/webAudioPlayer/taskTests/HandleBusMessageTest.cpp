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

#include "tasks/webAudio/HandleBusMessage.h"
#include "GstWebAudioPlayerClientMock.h"
#include "GstWebAudioPlayerPrivateMock.h"
#include "GstWrapperMock.h"
#include "Matchers.h"
#include "WebAudioPlayerContext.h"
#include <gst/gst.h>
#include <gtest/gtest.h>

using testing::_;
using testing::DoAll;
using testing::Return;
using testing::SetArgPointee;
using testing::StrictMock;

class WebAudioHandleBusMessageTest : public testing::Test
{
protected:
    firebolt::rialto::server::WebAudioPlayerContext m_context;
    StrictMock<firebolt::rialto::server::GstWebAudioPlayerPrivateMock> m_gstPlayer;
    StrictMock<firebolt::rialto::server::GstWebAudioPlayerClientMock> m_gstPlayerClient;
    std::shared_ptr<firebolt::rialto::server::GstWrapperMock> m_gstWrapper{
        std::make_shared<StrictMock<firebolt::rialto::server::GstWrapperMock>>()};
    GstElement m_pipeline{};

    WebAudioHandleBusMessageTest() { m_context.pipeline = &m_pipeline; }
};

TEST_F(WebAudioHandleBusMessageTest, shouldNotHandleMessageWithUnknownType)
{
    GstMessage message{};
    GST_MESSAGE_SRC(&message) = GST_OBJECT(&m_pipeline);
    EXPECT_CALL(*m_gstWrapper, gstMessageUnref(&message));
    firebolt::rialto::server::webaudio::HandleBusMessage task{m_context, m_gstPlayer, &m_gstPlayerClient, m_gstWrapper,
                                                              &message};
    task.execute();
}

TEST_F(WebAudioHandleBusMessageTest, shouldNotHandleEosMessageForAnotherPipeline)
{
    GstMessage message{};
    EXPECT_CALL(*m_gstWrapper, gstMessageUnref(&message));
    GST_MESSAGE_TYPE(&message) = GST_MESSAGE_EOS;
    firebolt::rialto::server::webaudio::HandleBusMessage task{m_context, m_gstPlayer, &m_gstPlayerClient, m_gstWrapper,
                                                              &message};
    task.execute();
}

TEST_F(WebAudioHandleBusMessageTest, shouldNotHandleMessageEosWhenPipelineIsNull)
{
    m_context.pipeline = nullptr;
    GstMessage message{};
    GST_MESSAGE_SRC(&message) = GST_OBJECT(&m_pipeline);
    GST_MESSAGE_TYPE(&message) = GST_MESSAGE_EOS;
    EXPECT_CALL(*m_gstWrapper, gstMessageUnref(&message));
    firebolt::rialto::server::webaudio::HandleBusMessage task{m_context, m_gstPlayer, &m_gstPlayerClient, m_gstWrapper,
                                                              &message};
    task.execute();
}

TEST_F(WebAudioHandleBusMessageTest, shouldHandleEosMessage)
{
    GstMessage message{};
    GST_MESSAGE_SRC(&message) = GST_OBJECT(&m_pipeline);
    GST_MESSAGE_TYPE(&message) = GST_MESSAGE_EOS;
    EXPECT_CALL(m_gstPlayerClient, notifyState(firebolt::rialto::WebAudioPlayerState::END_OF_STREAM));
    EXPECT_CALL(*m_gstWrapper, gstMessageUnref(&message));
    firebolt::rialto::server::webaudio::HandleBusMessage task{m_context, m_gstPlayer, &m_gstPlayerClient, m_gstWrapper,
                                                              &message};
    task.execute();
}

TEST_F(WebAudioHandleBusMessageTest, shouldNotHandleStateChangedMessageForAnotherPipeline)
{
    GstMessage message{};
    EXPECT_CALL(*m_gstWrapper, gstMessageUnref(&message));
    GST_MESSAGE_TYPE(&message) = GST_MESSAGE_STATE_CHANGED;
    firebolt::rialto::server::webaudio::HandleBusMessage task{m_context, m_gstPlayer, &m_gstPlayerClient, m_gstWrapper,
                                                              &message};
    task.execute();
}

TEST_F(WebAudioHandleBusMessageTest, shouldNotHandleMessageStateChangedWhenPipelineIsNull)
{
    m_context.pipeline = nullptr;
    GstMessage message{};
    GST_MESSAGE_SRC(&message) = GST_OBJECT(&m_pipeline);
    GST_MESSAGE_TYPE(&message) = GST_MESSAGE_STATE_CHANGED;
    EXPECT_CALL(*m_gstWrapper, gstMessageUnref(&message));
    firebolt::rialto::server::webaudio::HandleBusMessage task{m_context, m_gstPlayer, &m_gstPlayerClient, m_gstWrapper,
                                                              &message};
    task.execute();
}

TEST_F(WebAudioHandleBusMessageTest, shouldNotHandleStateChangedMessageWhenGstPlayerClientIsNull)
{
    GstMessage message{};
    GST_MESSAGE_SRC(&message) = GST_OBJECT(&m_pipeline);
    GST_MESSAGE_TYPE(&message) = GST_MESSAGE_STATE_CHANGED;
    GstState oldState = GST_STATE_READY;
    GstState newState = GST_STATE_NULL;
    GstState pending = GST_STATE_VOID_PENDING;

    EXPECT_CALL(*m_gstWrapper, gstMessageParseStateChanged(&message, _, _, _))
        .WillRepeatedly(DoAll(SetArgPointee<1>(oldState), SetArgPointee<2>(newState), SetArgPointee<3>(pending)));
    EXPECT_CALL(*m_gstWrapper, gstElementStateGetName(oldState)).Times(2).WillRepeatedly(Return("Ready"));
    EXPECT_CALL(*m_gstWrapper, gstElementStateGetName(newState)).Times(2).WillRepeatedly(Return("Null"));
    EXPECT_CALL(*m_gstWrapper, gstElementStateGetName(pending)).WillOnce(Return("Void"));
    EXPECT_CALL(*m_gstWrapper, gstDebugBinToDotFileWithTs(GST_BIN(&m_pipeline), _, _));
    EXPECT_CALL(*m_gstWrapper, gstMessageUnref(&message));
    firebolt::rialto::server::webaudio::HandleBusMessage task{m_context, m_gstPlayer, nullptr, m_gstWrapper, &message};
    task.execute();
}

TEST_F(WebAudioHandleBusMessageTest, shouldHandleStateChangedToPausedMessage)
{
    GstMessage message{};
    GST_MESSAGE_SRC(&message) = GST_OBJECT(&m_pipeline);
    GST_MESSAGE_TYPE(&message) = GST_MESSAGE_STATE_CHANGED;
    GstState oldState = GST_STATE_READY;
    GstState newState = GST_STATE_PAUSED;
    GstState pending = GST_STATE_VOID_PENDING;

    EXPECT_CALL(*m_gstWrapper, gstMessageParseStateChanged(&message, _, _, _))
        .WillRepeatedly(DoAll(SetArgPointee<1>(oldState), SetArgPointee<2>(newState), SetArgPointee<3>(pending)));
    EXPECT_CALL(*m_gstWrapper, gstElementStateGetName(oldState)).Times(2).WillRepeatedly(Return("Ready"));
    EXPECT_CALL(*m_gstWrapper, gstElementStateGetName(newState)).Times(2).WillRepeatedly(Return("Paused"));
    EXPECT_CALL(*m_gstWrapper, gstElementStateGetName(pending)).WillOnce(Return("Void"));
    EXPECT_CALL(*m_gstWrapper, gstDebugBinToDotFileWithTs(GST_BIN(&m_pipeline), _, _));
    EXPECT_CALL(m_gstPlayerClient, notifyState(firebolt::rialto::WebAudioPlayerState::PAUSED));
    EXPECT_CALL(*m_gstWrapper, gstMessageUnref(&message));
    firebolt::rialto::server::webaudio::HandleBusMessage task{m_context, m_gstPlayer, &m_gstPlayerClient, m_gstWrapper,
                                                              &message};
    task.execute();
}

TEST_F(WebAudioHandleBusMessageTest, shouldHandleStateChangedToPausedAndPendingPausedMessage)
{
    GstMessage message{};
    GST_MESSAGE_SRC(&message) = GST_OBJECT(&m_pipeline);
    GST_MESSAGE_TYPE(&message) = GST_MESSAGE_STATE_CHANGED;
    GstState oldState = GST_STATE_PAUSED;
    GstState newState = GST_STATE_PAUSED;
    GstState pending = GST_STATE_PAUSED;

    EXPECT_CALL(*m_gstWrapper, gstMessageParseStateChanged(&message, _, _, _))
        .WillRepeatedly(DoAll(SetArgPointee<1>(oldState), SetArgPointee<2>(newState), SetArgPointee<3>(pending)));
    EXPECT_CALL(*m_gstWrapper, gstElementStateGetName(GST_STATE_PAUSED)).WillRepeatedly(Return("Paused"));
    EXPECT_CALL(*m_gstWrapper, gstDebugBinToDotFileWithTs(GST_BIN(&m_pipeline), _, _));
    EXPECT_CALL(*m_gstWrapper, gstMessageUnref(&message));
    firebolt::rialto::server::webaudio::HandleBusMessage task{m_context, m_gstPlayer, &m_gstPlayerClient, m_gstWrapper,
                                                              &message};
    task.execute();
}

TEST_F(WebAudioHandleBusMessageTest, shouldHandleStateChangedToPlayingMessage)
{
    GstMessage message{};
    GST_MESSAGE_SRC(&message) = GST_OBJECT(&m_pipeline);
    GST_MESSAGE_TYPE(&message) = GST_MESSAGE_STATE_CHANGED;
    GstState oldState = GST_STATE_READY;
    GstState newState = GST_STATE_PLAYING;
    GstState pending = GST_STATE_VOID_PENDING;

    EXPECT_CALL(*m_gstWrapper, gstMessageParseStateChanged(&message, _, _, _))
        .WillRepeatedly(DoAll(SetArgPointee<1>(oldState), SetArgPointee<2>(newState), SetArgPointee<3>(pending)));
    EXPECT_CALL(*m_gstWrapper, gstElementStateGetName(oldState)).Times(2).WillRepeatedly(Return("Ready"));
    EXPECT_CALL(*m_gstWrapper, gstElementStateGetName(newState)).Times(2).WillRepeatedly(Return("Playing"));
    EXPECT_CALL(*m_gstWrapper, gstElementStateGetName(pending)).WillOnce(Return("Void"));
    EXPECT_CALL(*m_gstWrapper, gstDebugBinToDotFileWithTs(GST_BIN(&m_pipeline), _, _));
    EXPECT_CALL(m_gstPlayerClient, notifyState(firebolt::rialto::WebAudioPlayerState::PLAYING));
    EXPECT_CALL(*m_gstWrapper, gstMessageUnref(&message));
    firebolt::rialto::server::webaudio::HandleBusMessage task{m_context, m_gstPlayer, &m_gstPlayerClient, m_gstWrapper,
                                                              &message};
    task.execute();
}
