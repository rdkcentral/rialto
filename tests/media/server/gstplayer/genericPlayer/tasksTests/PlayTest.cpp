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

#include "tasks/generic/Play.h"
#include "GstGenericPlayerPrivateMock.h"
#include <gtest/gtest.h>

using testing::Return;
using testing::StrictMock;

class PlayTest : public testing::Test
{
protected:
    StrictMock<firebolt::rialto::server::GstGenericPlayerPrivateMock> m_gstPlayer;
};

TEST_F(PlayTest, shouldPlay)
{
    EXPECT_CALL(m_gstPlayer, changePipelineState(GST_STATE_PLAYING)).WillOnce(Return(true));
    firebolt::rialto::server::tasks::generic::Play task{m_gstPlayer};
    task.execute();
}

TEST_F(PlayTest, shouldFailToPlay)
{
    EXPECT_CALL(m_gstPlayer, changePipelineState(GST_STATE_PLAYING)).WillOnce(Return(false));
    firebolt::rialto::server::tasks::generic::Play task{m_gstPlayer};
    task.execute();
}
