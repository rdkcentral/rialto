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

#include "MediaPipelineTestBase.h"

class RialtoClientMediaPipelineSetLowLatencyTest : public MediaPipelineTestBase
{
protected:
    const bool m_kLowLatency{false};

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
 * Test that setLowLatency returns success if the IPC API succeeds.
 */
TEST_F(RialtoClientMediaPipelineSetLowLatencyTest, setLowLatencySuccess)
{
    EXPECT_CALL(*m_mediaPipelineIpcMock, setLowLatency(m_kLowLatency)).WillOnce(Return(true));

    EXPECT_EQ(m_mediaPipeline->setLowLatency(m_kLowLatency), true);
}

/**
 * Test that setLowLatency returns failure if the IPC API fails.
 */
TEST_F(RialtoClientMediaPipelineSetLowLatencyTest, setLowLatencyFailure)
{
    EXPECT_CALL(*m_mediaPipelineIpcMock, setLowLatency(m_kLowLatency)).WillOnce(Return(false));

    EXPECT_EQ(m_mediaPipeline->setLowLatency(m_kLowLatency), false);
}
