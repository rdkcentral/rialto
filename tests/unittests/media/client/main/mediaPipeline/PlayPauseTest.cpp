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

class RialtoClientMediaPipelinePlayPauseTest : public MediaPipelineTestBase
{
protected:
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
 * Test that Play returns success if the IPC API succeeds.
 */
TEST_F(RialtoClientMediaPipelinePlayPauseTest, PlaySuccess)
{
    EXPECT_CALL(*m_mediaPipelineIpcMock, play()).WillOnce(Return(true));

    EXPECT_EQ(m_mediaPipeline->play(), true);
}

/**
 * Test that Play returns failure if the IPC API fails.
 */
TEST_F(RialtoClientMediaPipelinePlayPauseTest, PlayFailure)
{
    EXPECT_CALL(*m_mediaPipelineIpcMock, play()).WillOnce(Return(false));

    EXPECT_EQ(m_mediaPipeline->play(), false);
}

/**
 * Test that Pause returns success if the IPC API succeeds.
 */
TEST_F(RialtoClientMediaPipelinePlayPauseTest, PauseSuccess)
{
    EXPECT_CALL(*m_mediaPipelineIpcMock, pause()).WillOnce(Return(true));

    EXPECT_EQ(m_mediaPipeline->pause(), true);
}

/**
 * Test that Pause returns failure if the IPC API fails.
 */
TEST_F(RialtoClientMediaPipelinePlayPauseTest, PauseFailure)
{
    EXPECT_CALL(*m_mediaPipelineIpcMock, pause()).WillOnce(Return(false));

    EXPECT_EQ(m_mediaPipeline->pause(), false);
}
