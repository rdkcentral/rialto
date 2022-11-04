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

#include "tasks/Underflow.h"
#include "GstPlayerClientMock.h"
#include "GstPlayerPrivateMock.h"
#include <gtest/gtest.h>

using testing::_;
using testing::Invoke;
using testing::Return;
using testing::StrictMock;

struct UnderflowTest : public testing::Test
{
    StrictMock<firebolt::rialto::server::GstPlayerPrivateMock> m_gstPlayer;
    StrictMock<firebolt::rialto::server::GstPlayerClientMock> m_gstPlayerClient;
};

TEST_F(UnderflowTest, shouldNotReportUnderflowWhenItIsAlreadyActive)
{
    bool underflowFlag = true;
    firebolt::rialto::server::Underflow task{m_gstPlayer, &m_gstPlayerClient, underflowFlag};
    task.execute();
    EXPECT_TRUE(underflowFlag);
}

TEST_F(UnderflowTest, shouldReportUnderflow)
{
    bool underflowFlag = false;
    firebolt::rialto::server::Underflow task{m_gstPlayer, &m_gstPlayerClient, underflowFlag};
    EXPECT_CALL(m_gstPlayer, stopPositionReportingAndCheckAudioUnderflowTimer());
    EXPECT_CALL(m_gstPlayer, changePipelineState(GST_STATE_PAUSED));
    EXPECT_CALL(m_gstPlayerClient, notifyNetworkState(firebolt::rialto::NetworkState::STALLED));
    task.execute();
    EXPECT_TRUE(underflowFlag);
}
