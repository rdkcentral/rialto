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

#include "tasks/generic/SetMute.h"
#include "GenericPlayerContext.h"
#include "GstWrapperMock.h"
#include <gst/gst.h>
#include <gtest/gtest.h>

using testing::_;
using testing::Return;
using testing::StrictMock;

namespace
{
constexpr bool kMute{false};
} // namespace

class SetMuteTest : public testing::Test
{
public:
    SetMuteTest() = default;

    firebolt::rialto::server::GenericPlayerContext m_context;
    std::shared_ptr<firebolt::rialto::server::GstWrapperMock> m_gstWrapper{
        std::make_shared<StrictMock<firebolt::rialto::server::GstWrapperMock>>()};
    GstElement m_pipeline{};
};

TEST_F(SetMuteTest, shouldFailToSetMuteWhenPipelineIsNull)
{
    firebolt::rialto::server::tasks::generic::SetMute task{m_context, m_gstWrapper, kMute};
    task.execute();
}

TEST_F(SetMuteTest, shouldSetMute)
{
    m_context.pipeline = &m_pipeline;
    EXPECT_CALL(*m_gstWrapper, gstStreamVolumeSetMute(_, kMute));
    firebolt::rialto::server::tasks::generic::SetMute task{m_context, m_gstWrapper, kMute};
    task.execute();
}