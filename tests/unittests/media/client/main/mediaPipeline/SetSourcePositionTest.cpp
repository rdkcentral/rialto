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

class RialtoClientMediaPipelineSetSourcePositionTest : public MediaPipelineTestBase
{
protected:
    const int32_t m_kSourceId{1};
    const int64_t m_kPosition{1234};

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
 * Test that setSourcePosition returns success if the IPC API succeeds.
 */
TEST_F(RialtoClientMediaPipelineSetSourcePositionTest, Success)
{
    EXPECT_CALL(*m_mediaPipelineIpcMock, setSourcePosition(m_kSourceId, m_kPosition)).WillOnce(Return(true));

    EXPECT_EQ(m_mediaPipeline->setSourcePosition(m_kSourceId, m_kPosition), true);
}

/**
 * Test that setSourcePosition returns failure if the IPC API fails.
 */
TEST_F(RialtoClientMediaPipelineSetSourcePositionTest, Failure)
{
    EXPECT_CALL(*m_mediaPipelineIpcMock, setSourcePosition(m_kSourceId, m_kPosition)).WillOnce(Return(false));

    EXPECT_EQ(m_mediaPipeline->setSourcePosition(m_kSourceId, m_kPosition), false);
}
