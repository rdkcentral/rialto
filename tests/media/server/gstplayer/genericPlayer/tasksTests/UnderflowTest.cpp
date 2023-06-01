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

#include "tasks/generic/Underflow.h"
#include "GstGenericPlayerClientMock.h"
#include "GstGenericPlayerPrivateMock.h"
#include <gtest/gtest.h>

using testing::_;
using testing::Invoke;
using testing::Return;
using testing::StrictMock;

class UnderflowTest : public testing::Test
{
protected:
    firebolt::rialto::server::GenericPlayerContext m_context{};
    StrictMock<firebolt::rialto::server::GstGenericPlayerPrivateMock> m_gstPlayer;
    StrictMock<firebolt::rialto::server::GstGenericPlayerClientMock> m_gstPlayerClient;
    GstElement m_audioAppSrc{};

    UnderflowTest()
    {
        m_context.audioAppSrc = &m_audioAppSrc;
        m_context.streamInfo.emplace(firebolt::rialto::MediaSourceType::AUDIO,
                                     firebolt::rialto::server::StreamInfo{&m_audioAppSrc, true});
    }
};

TEST_F(UnderflowTest, shouldNotReportUnderflowWhenItIsDisabled)
{
    bool underflowFlag{false};
    bool underflowEnabled{false};
    firebolt::rialto::server::tasks::generic::Underflow task{m_context, m_gstPlayer, &m_gstPlayerClient, underflowFlag,
                                                             underflowEnabled};
    task.execute();
}

TEST_F(UnderflowTest, shouldNotReportUnderflowWhenItIsAlreadyActive)
{
    bool underflowFlag{true};
    bool underflowEnabled{true};
    firebolt::rialto::server::tasks::generic::Underflow task{m_context, m_gstPlayer, &m_gstPlayerClient, underflowFlag,
                                                             underflowEnabled};
    task.execute();
    EXPECT_TRUE(underflowFlag);
}

TEST_F(UnderflowTest, shouldReportUnderflow)
{
    bool underflowFlag{false};
    bool underflowEnabled{true};
    m_context.resumeOnUnderflowRecovery = false;

    firebolt::rialto::server::tasks::generic::Underflow task{m_context, m_gstPlayer, &m_gstPlayerClient, underflowFlag,
                                                             underflowEnabled};
    EXPECT_CALL(m_gstPlayer, stopPositionReportingAndCheckAudioUnderflowTimer());
    EXPECT_CALL(m_gstPlayer, changePipelineState(GST_STATE_PAUSED));
    EXPECT_CALL(m_gstPlayerClient, notifyNetworkState(firebolt::rialto::NetworkState::STALLED));
    task.execute();
    EXPECT_TRUE(underflowFlag);
    EXPECT_TRUE(m_context.resumeOnUnderflowRecovery);
}

TEST_F(UnderflowTest, shouldNotReportEosWhenEosAlreadyNotified)
{
    m_context.endOfStreamInfo.emplace(firebolt::rialto::MediaSourceType::AUDIO, GST_ELEMENT(&m_audioAppSrc));
    m_context.eosNotified = true;

    bool underflowFlag{false};
    bool underflowEnabled{true};
    firebolt::rialto::server::tasks::generic::Underflow task{m_context, m_gstPlayer, &m_gstPlayerClient, underflowFlag,
                                                             underflowEnabled};
    task.execute();
    EXPECT_FALSE(underflowFlag);
}

TEST_F(UnderflowTest, shouldReportEos)
{
    m_context.endOfStreamInfo.emplace(firebolt::rialto::MediaSourceType::AUDIO, GST_ELEMENT(&m_audioAppSrc));

    bool underflowFlag{false};
    bool underflowEnabled{true};
    firebolt::rialto::server::tasks::generic::Underflow task{m_context, m_gstPlayer, &m_gstPlayerClient, underflowFlag,
                                                             underflowEnabled};
    EXPECT_CALL(m_gstPlayerClient, notifyPlaybackState(firebolt::rialto::PlaybackState::END_OF_STREAM));
    task.execute();
    EXPECT_FALSE(underflowFlag);
    EXPECT_TRUE(m_context.eosNotified);
}
