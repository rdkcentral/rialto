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

#include "tasks/webAudio/WriteBuffer.h"
#include "GlibWrapperMock.h"
#include "GstWebAudioPlayerPrivateMock.h"
#include "GstWrapperMock.h"
#include "Matchers.h"
#include "WebAudioPlayerContext.h"
#include <gst/gst.h>
#include <gtest/gtest.h>

using testing::_;
using testing::Return;
using testing::StrictMock;

class WebAudioWriteBufferTest : public testing::Test
{
protected:
    firebolt::rialto::server::WebAudioPlayerContext m_context;
    std::shared_ptr<firebolt::rialto::server::GstWrapperMock> m_gstWrapper{
        std::make_shared<StrictMock<firebolt::rialto::server::GstWrapperMock>>()};

    uint8_t m_mainPtr{};
    uint32_t m_mainLength = firebolt::rialto::server::kMaxWebAudioBytes - 20;
    uint8_t m_wrapPtr{};
    uint32_t m_wrapLength = 20;
    GstElement m_appSrc{};
    GstBuffer m_buffer{};

    WebAudioWriteBufferTest() { m_context.source = &m_appSrc; }
};

TEST_F(WebAudioWriteBufferTest, shouldWriteBufferForAllData)
{
    EXPECT_CALL(*m_gstWrapper, gstAppSrcGetCurrentLevelBytes(GST_APP_SRC(&m_appSrc))).WillOnce(Return(0));
    EXPECT_CALL(*m_gstWrapper, gstBufferNewAllocate(_, m_mainLength + m_wrapLength, _)).WillOnce(Return(&m_buffer));
    EXPECT_CALL(*m_gstWrapper, gstBufferFill(&m_buffer, 0, &m_mainPtr, m_mainLength)).WillOnce(Return(m_mainLength));
    EXPECT_CALL(*m_gstWrapper, gstBufferFill(&m_buffer, m_mainLength, &m_wrapPtr, m_wrapLength))
        .WillOnce(Return(m_wrapLength));
    EXPECT_CALL(*m_gstWrapper, gstAppSrcPushBuffer(GST_APP_SRC(&m_appSrc), &m_buffer)).WillOnce(Return(GST_FLOW_OK));

    firebolt::rialto::server::webaudio::WriteBuffer task{m_context,    m_gstWrapper, &m_mainPtr,
                                                         m_mainLength, &m_wrapPtr,   m_wrapLength};
    task.execute();
    EXPECT_EQ(m_context.m_lastBytesWritten, m_mainLength + m_wrapLength);
}

TEST_F(WebAudioWriteBufferTest, shouldWriteBufferForAllMainDataAndPartialWrapData)
{
    EXPECT_CALL(*m_gstWrapper, gstAppSrcGetCurrentLevelBytes(GST_APP_SRC(&m_appSrc))).WillOnce(Return(m_wrapLength / 2));
    EXPECT_CALL(*m_gstWrapper, gstBufferNewAllocate(_, m_mainLength + m_wrapLength / 2, _)).WillOnce(Return(&m_buffer));
    EXPECT_CALL(*m_gstWrapper, gstBufferFill(&m_buffer, 0, &m_mainPtr, m_mainLength)).WillOnce(Return(m_mainLength));
    EXPECT_CALL(*m_gstWrapper, gstBufferFill(&m_buffer, m_mainLength, &m_wrapPtr, m_wrapLength / 2))
        .WillOnce(Return(m_wrapLength / 2));
    EXPECT_CALL(*m_gstWrapper, gstAppSrcPushBuffer(GST_APP_SRC(&m_appSrc), &m_buffer)).WillOnce(Return(GST_FLOW_OK));

    firebolt::rialto::server::webaudio::WriteBuffer task{m_context,    m_gstWrapper, &m_mainPtr,
                                                         m_mainLength, &m_wrapPtr,   m_wrapLength};
    task.execute();
    EXPECT_EQ(m_context.m_lastBytesWritten, m_mainLength + m_wrapLength / 2);
}

TEST_F(WebAudioWriteBufferTest, shouldWriteBufferForPartialMainDataAndNoWrapData)
{
    EXPECT_CALL(*m_gstWrapper, gstAppSrcGetCurrentLevelBytes(GST_APP_SRC(&m_appSrc)))
        .WillOnce(Return(m_mainLength / 2 + m_wrapLength));
    EXPECT_CALL(*m_gstWrapper, gstBufferNewAllocate(_, m_mainLength / 2, _)).WillOnce(Return(&m_buffer));
    EXPECT_CALL(*m_gstWrapper, gstBufferFill(&m_buffer, 0, &m_mainPtr, m_mainLength / 2)).WillOnce(Return(m_mainLength / 2));
    EXPECT_CALL(*m_gstWrapper, gstAppSrcPushBuffer(GST_APP_SRC(&m_appSrc), &m_buffer)).WillOnce(Return(GST_FLOW_OK));

    firebolt::rialto::server::webaudio::WriteBuffer task{m_context,    m_gstWrapper, &m_mainPtr,
                                                         m_mainLength, &m_wrapPtr,   m_wrapLength};
    task.execute();
    EXPECT_EQ(m_context.m_lastBytesWritten, m_mainLength / 2);
}

TEST_F(WebAudioWriteBufferTest, shouldNotWriteBufferIfNewAllocateFails)
{
    EXPECT_CALL(*m_gstWrapper, gstAppSrcGetCurrentLevelBytes(GST_APP_SRC(&m_appSrc))).WillOnce(Return(0));
    EXPECT_CALL(*m_gstWrapper, gstBufferNewAllocate(_, m_mainLength + m_wrapLength, _)).WillOnce(Return(nullptr));

    firebolt::rialto::server::webaudio::WriteBuffer task{m_context,    m_gstWrapper, &m_mainPtr,
                                                         m_mainLength, &m_wrapPtr,   m_wrapLength};
    task.execute();
    EXPECT_EQ(m_context.m_lastBytesWritten, 0);
}

TEST_F(WebAudioWriteBufferTest, shouldWriteBufferIfBytesWrittenLessThanExpected)
{
    EXPECT_CALL(*m_gstWrapper, gstAppSrcGetCurrentLevelBytes(GST_APP_SRC(&m_appSrc))).WillOnce(Return(0));
    EXPECT_CALL(*m_gstWrapper, gstBufferNewAllocate(_, m_mainLength + m_wrapLength, _)).WillOnce(Return(&m_buffer));
    EXPECT_CALL(*m_gstWrapper, gstBufferFill(&m_buffer, 0, &m_mainPtr, m_mainLength)).WillOnce(Return(m_mainLength - 1));
    EXPECT_CALL(*m_gstWrapper, gstBufferFill(&m_buffer, m_mainLength - 1, &m_wrapPtr, m_wrapLength))
        .WillOnce(Return(m_wrapLength));
    EXPECT_CALL(*m_gstWrapper, gstAppSrcPushBuffer(GST_APP_SRC(&m_appSrc), &m_buffer)).WillOnce(Return(GST_FLOW_OK));

    firebolt::rialto::server::webaudio::WriteBuffer task{m_context,    m_gstWrapper, &m_mainPtr,
                                                         m_mainLength, &m_wrapPtr,   m_wrapLength};
    task.execute();
    EXPECT_EQ(m_context.m_lastBytesWritten, m_mainLength + m_wrapLength - 1);
}

TEST_F(WebAudioWriteBufferTest, shouldNotWriteBufferIfPushBufferFails)
{
    EXPECT_CALL(*m_gstWrapper, gstAppSrcGetCurrentLevelBytes(GST_APP_SRC(&m_appSrc))).WillOnce(Return(0));
    EXPECT_CALL(*m_gstWrapper, gstBufferNewAllocate(_, m_mainLength + m_wrapLength, _)).WillOnce(Return(&m_buffer));
    EXPECT_CALL(*m_gstWrapper, gstBufferFill(&m_buffer, 0, &m_mainPtr, m_mainLength)).WillOnce(Return(m_mainLength));
    EXPECT_CALL(*m_gstWrapper, gstBufferFill(&m_buffer, m_mainLength, &m_wrapPtr, m_wrapLength))
        .WillOnce(Return(m_wrapLength));
    EXPECT_CALL(*m_gstWrapper, gstAppSrcPushBuffer(GST_APP_SRC(&m_appSrc), &m_buffer)).WillOnce(Return(GST_FLOW_ERROR));
    EXPECT_CALL(*m_gstWrapper, gstBufferUnref(&m_buffer));

    firebolt::rialto::server::webaudio::WriteBuffer task{m_context,    m_gstWrapper, &m_mainPtr,
                                                         m_mainLength, &m_wrapPtr,   m_wrapLength};
    task.execute();
    EXPECT_EQ(m_context.m_lastBytesWritten, 0);
}
