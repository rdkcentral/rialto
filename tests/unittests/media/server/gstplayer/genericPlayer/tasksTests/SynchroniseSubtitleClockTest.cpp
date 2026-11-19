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

#include "tasks/generic/SynchroniseSubtitleClock.h"
#include "GlibWrapperMock.h"
#include "GstGenericPlayerPrivateMock.h"
#include "GstWrapperMock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

using firebolt::rialto::MediaSourceType;
using firebolt::rialto::server::GenericPlayerContext;
using firebolt::rialto::server::GstGenericPlayerPrivateMock;
using firebolt::rialto::server::StreamInfo;
using firebolt::rialto::server::tasks::generic::SynchroniseSubtitleClock;
using firebolt::rialto::wrappers::GlibWrapperMock;
using firebolt::rialto::wrappers::GstWrapperMock;
using testing::Return;
using testing::StrEq;
using testing::StrictMock;

class SynchroniseSubtitleClockTest : public testing::Test
{
protected:
    SynchroniseSubtitleClockTest()
        : m_gstWrapper{std::make_shared<StrictMock<GstWrapperMock>>()},
          m_glibWrapper{std::make_shared<StrictMock<GlibWrapperMock>>()}
    {
        m_context.pipeline = &m_pipeline;
        m_context.streamInfo.emplace(MediaSourceType::SUBTITLE, StreamInfo{&m_subtitleSource, true});
    }

    SynchroniseSubtitleClock createTask()
    {
        return SynchroniseSubtitleClock{m_context, m_player, m_gstWrapper, m_glibWrapper};
    }

    GenericPlayerContext m_context;
    StrictMock<GstGenericPlayerPrivateMock> m_player;
    std::shared_ptr<StrictMock<GstWrapperMock>> m_gstWrapper;
    std::shared_ptr<StrictMock<GlibWrapperMock>> m_glibWrapper;
    GstElement m_pipeline{};
    GstElement m_subtitleSource{};
    GstStructure m_structure{};
    GstEvent m_event{};
};

TEST_F(SynchroniseSubtitleClockTest, RejectsNegativePosition)
{
    EXPECT_CALL(m_player, getPosition(&m_pipeline)).WillOnce(Return(-1)).WillOnce(Return(-2));

    auto task = createTask();
    task.execute();
    task.execute();
}

TEST_F(SynchroniseSubtitleClockTest, SendsNonnegativePositionAsUint64)
{
    constexpr gint64 kPosition{1234};
    EXPECT_CALL(m_player, getPosition(&m_pipeline)).WillOnce(Return(kPosition));
    EXPECT_CALL(*m_gstWrapper, gstStructureNewUint64Stub(StrEq("current-pts"), StrEq("pts"), G_TYPE_UINT64,
                                                         static_cast<guint64>(kPosition)))
        .WillOnce(Return(&m_structure));
    EXPECT_CALL(*m_gstWrapper, gstEventNewCustom(GST_EVENT_CUSTOM_DOWNSTREAM_OOB, &m_structure)).WillOnce(Return(&m_event));
    EXPECT_CALL(*m_gstWrapper, gstElementSendEvent(&m_subtitleSource, &m_event)).WillOnce(Return(TRUE));

    auto task = createTask();
    task.execute();
}
