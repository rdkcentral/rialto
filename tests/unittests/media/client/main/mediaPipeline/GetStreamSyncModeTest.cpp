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

class RialtoClientMediaPipelineGetStreamSyncModeTest : public MediaPipelineTestBase
{
protected:
    int32_t m_streamSyncMode{};

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
 * Test that getStreamSyncMode returns success if the IPC API succeeds.
 */
TEST_F(RialtoClientMediaPipelineGetStreamSyncModeTest, getStreamSyncModeSuccess)
{
    int32_t returnStreamSyncMode{};

    EXPECT_CALL(*m_mediaPipelineIpcMock, getStreamSyncMode(returnStreamSyncMode)).WillOnce(DoAll(SetArgReferee<0>(m_streamSyncMode), Return(true)));

    EXPECT_EQ(m_mediaPipeline->getStreamSyncMode(returnStreamSyncMode), true);
    EXPECT_EQ(returnStreamSyncMode, m_streamSyncMode);
}

/**
 * Test that getStreamSyncMode returns failure if the IPC API fails.
 */
TEST_F(RialtoClientMediaPipelineGetStreamSyncModeTest, getStreamSyncModeFailure)
{
    EXPECT_CALL(*m_mediaPipelineIpcMock, getStreamSyncMode(m_streamSyncMode)).WillOnce(Return(false));

    EXPECT_EQ(m_mediaPipeline->getStreamSyncMode(m_streamSyncMode), false);
}
