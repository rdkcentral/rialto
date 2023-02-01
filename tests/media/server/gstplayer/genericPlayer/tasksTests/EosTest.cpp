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

#include "tasks/generic/Eos.h"
#include "GenericPlayerContext.h"
#include "GstGenericPlayerPrivateMock.h"
#include "GstWrapperMock.h"
#include <gst/gst.h>
#include <gtest/gtest.h>

using testing::_;
using testing::Return;
using testing::StrictMock;

class EosTest : public testing::Test
{
protected:
    firebolt::rialto::server::GenericPlayerContext m_context{};
    StrictMock<firebolt::rialto::server::GstGenericPlayerPrivateMock> m_gstPlayer;
    std::shared_ptr<firebolt::rialto::server::GstWrapperMock> m_gstWrapper{
        std::make_shared<StrictMock<firebolt::rialto::server::GstWrapperMock>>()};
    GstElement m_src{};
};

TEST_F(EosTest, shouldFailWhenStreamIsNotFound)
{
    firebolt::rialto::server::tasks::generic::Eos task{m_context, m_gstPlayer, m_gstWrapper,
                                                firebolt::rialto::MediaSourceType::AUDIO};
    task.execute();
}

TEST_F(EosTest, shouldSetEos)
{
    m_context.streamInfo.emplace(firebolt::rialto::MediaSourceType::AUDIO, &m_src);
    firebolt::rialto::server::tasks::generic::Eos task{m_context, m_gstPlayer, m_gstWrapper,
                                                firebolt::rialto::MediaSourceType::AUDIO};
    EXPECT_CALL(*m_gstWrapper, gstAppSrcEndOfStream(_)).WillOnce(Return(GST_FLOW_OK));
    task.execute();
}

TEST_F(EosTest, shouldFailToSetEos)
{
    m_context.streamInfo.emplace(firebolt::rialto::MediaSourceType::AUDIO, &m_src);
    firebolt::rialto::server::tasks::generic::Eos task{m_context, m_gstPlayer, m_gstWrapper,
                                                firebolt::rialto::MediaSourceType::AUDIO};
    EXPECT_CALL(*m_gstWrapper, gstAppSrcEndOfStream(_)).WillOnce(Return(GST_FLOW_ERROR));
    task.execute();
}

TEST_F(EosTest, shouldSetEosForAudioAndCancelAudioUnderflow)
{
    m_context.streamInfo.emplace(firebolt::rialto::MediaSourceType::AUDIO, &m_src);
    m_context.audioUnderflowOccured = true;
    firebolt::rialto::server::tasks::generic::Eos task{m_context, m_gstPlayer, m_gstWrapper,
                                                firebolt::rialto::MediaSourceType::AUDIO};
    EXPECT_CALL(m_gstPlayer, cancelUnderflow(m_context.audioUnderflowOccured));
    EXPECT_CALL(*m_gstWrapper, gstAppSrcEndOfStream(_)).WillOnce(Return(GST_FLOW_OK));
    task.execute();
}

TEST_F(EosTest, shouldSetEosForAudioAndSkipCancellingVideoUnderflow)
{
    m_context.streamInfo.emplace(firebolt::rialto::MediaSourceType::AUDIO, &m_src);
    m_context.videoUnderflowOccured = true;
    firebolt::rialto::server::tasks::generic::Eos task{m_context, m_gstPlayer, m_gstWrapper,
                                                firebolt::rialto::MediaSourceType::AUDIO};
    EXPECT_CALL(*m_gstWrapper, gstAppSrcEndOfStream(_)).WillOnce(Return(GST_FLOW_OK));
    task.execute();
}

TEST_F(EosTest, shouldSetEosForVideoAndCancelVideoUnderflow)
{
    m_context.streamInfo.emplace(firebolt::rialto::MediaSourceType::VIDEO, &m_src);
    m_context.videoUnderflowOccured = true;
    firebolt::rialto::server::tasks::generic::Eos task{m_context, m_gstPlayer, m_gstWrapper,
                                                firebolt::rialto::MediaSourceType::VIDEO};
    EXPECT_CALL(m_gstPlayer, cancelUnderflow(m_context.videoUnderflowOccured));
    EXPECT_CALL(*m_gstWrapper, gstAppSrcEndOfStream(_)).WillOnce(Return(GST_FLOW_OK));
    task.execute();
}

TEST_F(EosTest, shouldSetEosForVideoAndSkipCancellingAudioUnderflow)
{
    m_context.streamInfo.emplace(firebolt::rialto::MediaSourceType::VIDEO, &m_src);
    m_context.audioUnderflowOccured = true;
    firebolt::rialto::server::tasks::generic::Eos task{m_context, m_gstPlayer, m_gstWrapper,
                                                firebolt::rialto::MediaSourceType::VIDEO};
    EXPECT_CALL(*m_gstWrapper, gstAppSrcEndOfStream(_)).WillOnce(Return(GST_FLOW_OK));
    task.execute();
}
