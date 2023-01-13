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

#include "tasks/DeepElementAdded.h"
#include "PlayerContext.h"
#include "RdkGstreamerUtilsWrapperMock.h"
#include <gst/gst.h>
#include <gtest/gtest.h>
#include <memory>

using testing::StrictMock;

class DeepElementAddedTest : public testing::Test
{
protected:
    firebolt::rialto::server::PlayerContext m_context{};
    GstBin m_pipeline{};
    GstBin m_bin{};
    GstElement m_element{};
    std::shared_ptr<firebolt::rialto::server::RdkGstreamerUtilsWrapperMock> m_rdkGstreamerUtilsWrapper{
        std::make_shared<StrictMock<firebolt::rialto::server::RdkGstreamerUtilsWrapperMock>>()};
};

TEST_F(DeepElementAddedTest, shouldAddElement)
{
    EXPECT_CALL(*m_rdkGstreamerUtilsWrapper, deepElementAdded(&m_context.playbackGroup, &m_pipeline, &m_bin, &m_element));
    firebolt::rialto::server::DeepElementAdded task{m_context, m_rdkGstreamerUtilsWrapper, &m_pipeline, &m_bin,
                                                    &m_element};
    task.execute();
}
