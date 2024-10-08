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

class RialtoClientMediaPipelineSetSyncOffTest : public MediaPipelineTestBase
{
protected:
    const bool m_kSyncOff{false};

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
 * Test that setSyncOff returns success if the IPC API succeeds.
 */
TEST_F(RialtoClientMediaPipelineSetSyncOffTest, setSyncOffSuccess)
{
    EXPECT_CALL(*m_mediaPipelineIpcMock, setSyncOff(m_kSyncOff)).WillOnce(Return(true));

    EXPECT_EQ(m_mediaPipeline->setSyncOff(m_kSyncOff), true);
}

/**
 * Test that setSyncOff returns failure if the IPC API fails.
 */
TEST_F(RialtoClientMediaPipelineSetSyncOffTest, setSyncOffFailure)
{
    EXPECT_CALL(*m_mediaPipelineIpcMock, setSyncOff(m_kSyncOff)).WillOnce(Return(false));

    EXPECT_EQ(m_mediaPipeline->setSyncOff(m_kSyncOff), false);
}
