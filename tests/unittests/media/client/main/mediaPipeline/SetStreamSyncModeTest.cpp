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

class RialtoClientMediaPipelineSetStreamSyncModeTest : public MediaPipelineTestBase
{
protected:
    const int32_t m_kSourceId{7};
    const int32_t m_kStreamSyncMode{1};

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
 * Test that setStreamSyncMode returns success if the IPC API succeeds.
 */
TEST_F(RialtoClientMediaPipelineSetStreamSyncModeTest, setStreamSyncModeSuccess)
{
    EXPECT_CALL(*m_mediaPipelineIpcMock, setStreamSyncMode(m_kSourceId, m_kStreamSyncMode)).WillOnce(Return(true));

    EXPECT_EQ(m_mediaPipeline->setStreamSyncMode(m_kSourceId, m_kStreamSyncMode), true);
}

/**
 * Test that setStreamSyncMode returns failure if the IPC API fails.
 */
TEST_F(RialtoClientMediaPipelineSetStreamSyncModeTest, setStreamSyncModeFailure)
{
    EXPECT_CALL(*m_mediaPipelineIpcMock, setStreamSyncMode(m_kSourceId, m_kStreamSyncMode)).WillOnce(Return(false));

    EXPECT_EQ(m_mediaPipeline->setStreamSyncMode(m_kSourceId, m_kStreamSyncMode), false);
}
