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

#include "MediaPipelineTestBase.h"

class RialtoClientMediaPipelineGetDurationTest : public MediaPipelineTestBase
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
 * Test that GetDuration returns success if the IPC API succeeds.
 */
TEST_F(RialtoClientMediaPipelineGetDurationTest, GetDurationSuccess)
{
    constexpr int64_t kExpectedDuration{123};
    int64_t resultDuration{};
    EXPECT_CALL(*m_mediaPipelineIpcMock, getDuration(resultDuration))
        .WillOnce(Invoke(
            [&](int64_t &duration)
            {
                duration = kExpectedDuration;
                return true;
            }));
    EXPECT_TRUE(m_mediaPipeline->getDuration(resultDuration));
    EXPECT_EQ(resultDuration, kExpectedDuration);
}

/**
 * Test that GetDuration returns failure if the IPC API fails.
 */
TEST_F(RialtoClientMediaPipelineGetDurationTest, GetDurationFailure)
{
    int64_t resultDuration{};
    EXPECT_CALL(*m_mediaPipelineIpcMock, getDuration(resultDuration)).WillOnce(Return(false));
    EXPECT_FALSE(m_mediaPipeline->getDuration(resultDuration));
}
