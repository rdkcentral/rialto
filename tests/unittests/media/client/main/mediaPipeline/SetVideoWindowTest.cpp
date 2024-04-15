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

#include "MediaPipelineTestBase.h"

class RialtoClientMediaPipelineSetVideoWindowTest : public MediaPipelineTestBase
{
protected:
    uint32_t m_x = 1;
    uint32_t m_y = 2;
    uint32_t m_width = 3;
    uint32_t m_height = 4;

    virtual void SetUp()
    {
        MediaPipelineTestBase::SetUp();

        createMediaPipeline();
    }

    virtual void TearDown()
    {
        destroyMediaPipeline();

        MediaPipelineTestBase::TearDown();
    }
};

/**
 * Test that setVideoWindow returns success if the IPC API succeeds.
 */
TEST_F(RialtoClientMediaPipelineSetVideoWindowTest, Success)
{
    EXPECT_CALL(*m_mediaPipelineIpcMock, setVideoWindow(m_x, m_y, m_width, m_height)).WillOnce(Return(true));

    EXPECT_EQ(m_mediaPipeline->setVideoWindow(m_x, m_y, m_width, m_height), true);
}

/**
 * Test that setVideoWindow returns failure if the IPC API fails.
 */
TEST_F(RialtoClientMediaPipelineSetVideoWindowTest, Failure)
{
    EXPECT_CALL(*m_mediaPipelineIpcMock, setVideoWindow(m_x, m_y, m_width, m_height)).WillOnce(Return(false));

    EXPECT_EQ(m_mediaPipeline->setVideoWindow(m_x, m_y, m_width, m_height), false);
}
