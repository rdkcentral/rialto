/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
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

#include "tasks/generic/FirstFrameReceived.h"
#include "GstGenericPlayerClientMock.h"
#include "GstGenericPlayerPrivateMock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::StrictMock;

namespace
{
constexpr firebolt::rialto::MediaSourceType kSourceType{firebolt::rialto::MediaSourceType::VIDEO};
<<<<<<< HEAD
constexpr firebolt::rialto::MediaSourceType kAudioSourceType{firebolt::rialto::MediaSourceType::AUDIO};
=======
>>>>>>> master
} // namespace

class FirstFrameReceivedTest : public testing::Test
{
protected:
    firebolt::rialto::server::GenericPlayerContext m_context;
    StrictMock<firebolt::rialto::server::GstGenericPlayerPrivateMock> m_gstPlayer;
    StrictMock<firebolt::rialto::server::GstGenericPlayerClientMock> m_gstPlayerClient;
};

TEST_F(FirstFrameReceivedTest, shouldNotifyFirstFrameReceived)
{
    firebolt::rialto::server::tasks::generic::FirstFrameReceived task{m_context, m_gstPlayer, &m_gstPlayerClient,
                                                                      kSourceType};

    EXPECT_CALL(m_gstPlayerClient, notifyFirstFrameReceived(kSourceType));

    task.execute();
}

TEST_F(FirstFrameReceivedTest, shouldNotNotifyFirstFrameReceivedWhenClientIsNull)
{
    firebolt::rialto::server::tasks::generic::FirstFrameReceived task{m_context, m_gstPlayer, nullptr, kSourceType};

    task.execute();
}
<<<<<<< HEAD

TEST_F(FirstFrameReceivedTest, shouldNotifyFirstAudioFrameReceived)
{
    firebolt::rialto::server::tasks::generic::FirstFrameReceived task{m_context, m_gstPlayer, &m_gstPlayerClient,
                                                                      kAudioSourceType};

    EXPECT_CALL(m_gstPlayerClient, notifyFirstFrameReceived(kAudioSourceType));

    task.execute();
}
=======
>>>>>>> master
