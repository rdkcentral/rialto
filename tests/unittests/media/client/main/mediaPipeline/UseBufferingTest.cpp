/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Sky UK
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

class RialtoClientMediaPipelineUseBufferingTest : public MediaPipelineTestBase
{
protected:
    const bool m_kUseBuffering{true};

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
 * Test that SetUseBuffering returns success if the IPC API succeeds.
 */
TEST_F(RialtoClientMediaPipelineUseBufferingTest, SetSuccess)
{
    EXPECT_CALL(*m_mediaPipelineIpcMock, setUseBuffering(m_kUseBuffering)).WillOnce(Return(true));

    EXPECT_EQ(m_mediaPipeline->setUseBuffering(m_kUseBuffering), true);
}

/**
 * Test that SetUseBuffering returns failure if the IPC API fails.
 */
TEST_F(RialtoClientMediaPipelineUseBufferingTest, SetFailure)
{
    EXPECT_CALL(*m_mediaPipelineIpcMock, setUseBuffering(m_kUseBuffering)).WillOnce(Return(false));

    EXPECT_EQ(m_mediaPipeline->setUseBuffering(m_kUseBuffering), false);
}

/**
 * Test that GetUseBuffering returns success if the IPC API succeeds.
 */
TEST_F(RialtoClientMediaPipelineUseBufferingTest, GetSuccess)
{
    bool useBuffering{0};
    EXPECT_CALL(*m_mediaPipelineIpcMock, getUseBuffering(_))
        .WillOnce(DoAll(SetArgReferee<0>(m_kUseBuffering), Return(true)));

    EXPECT_EQ(m_mediaPipeline->getUseBuffering(useBuffering), true);
    EXPECT_EQ(m_kUseBuffering, useBuffering);
}

/**
 * Test that GetUseBuffering returns failure if the IPC API fails.
 */
TEST_F(RialtoClientMediaPipelineUseBufferingTest, GetFailure)
{
    bool useBuffering{0};
    EXPECT_CALL(*m_mediaPipelineIpcMock, getUseBuffering(_)).WillOnce(Return(false));

    EXPECT_EQ(m_mediaPipeline->getUseBuffering(useBuffering), false);
}
