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

class RialtoClientMediaPipelineGetPositionTest : public MediaPipelineTestBase
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
TEST_F(RialtoClientMediaPipelineGetPositionTest, GetPositionSuccess)
{
    constexpr int64_t expectedPosition{123};
    int64_t resultPosition{};
    EXPECT_CALL(*m_mediaPipelineIpcMock, getPosition(resultPosition)).WillOnce(Invoke([&](int64_t &position) {
        position = expectedPosition;
        return true;
    }));
    EXPECT_TRUE(m_mediaPipeline->getPosition(resultPosition));
    EXPECT_EQ(resultPosition, expectedPosition);
}

/**
 * Test that Play returns success if the IPC API succeeds.
 */
TEST_F(RialtoClientMediaPipelineGetPositionTest, GetPositionFailure)
{
    int64_t resultPosition{};
    EXPECT_CALL(*m_mediaPipelineIpcMock, getPosition(resultPosition)).WillOnce(Return(false));
    EXPECT_FALSE(m_mediaPipeline->getPosition(resultPosition));
}
