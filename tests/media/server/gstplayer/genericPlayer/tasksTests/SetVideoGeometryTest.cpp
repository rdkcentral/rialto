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

#include "tasks/generic/SetVideoGeometry.h"
#include "GenericPlayerContext.h"
#include "GstGenericPlayerPrivateMock.h"
#include "Matchers.h"
#include <gst/gst.h>
#include <gtest/gtest.h>

using testing::StrictMock;

class SetVideoGeometryTest : public testing::Test
{
protected:
    firebolt::rialto::server::GenericPlayerContext m_context{};
    StrictMock<firebolt::rialto::server::GstGenericPlayerPrivateMock> m_gstPlayer;
    firebolt::rialto::server::Rectangle m_rectangle{1, 2, 3, 4};
    GstElement m_pipeline{};

    SetVideoGeometryTest() { m_context.pipeline = &m_pipeline; }
};

TEST_F(SetVideoGeometryTest, shouldNotSetVideoGeometryWhenPipelineIsNull)
{
    m_context.pipeline = nullptr;
    firebolt::rialto::server::SetVideoGeometry task{m_context, m_gstPlayer, m_rectangle};
    task.execute();
}

TEST_F(SetVideoGeometryTest, shouldSetVideoGeometry)
{
    firebolt::rialto::server::SetVideoGeometry task{m_context, m_gstPlayer, m_rectangle};
    EXPECT_CALL(m_gstPlayer, setWesterossinkRectangle());
    task.execute();
    EXPECT_EQ(m_context.pendingGeometry, m_rectangle);
}
