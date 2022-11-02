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

class RialtoClientMediaPipelineLoadTest : public MediaPipelineTestBase
{
protected:
    MediaType m_type = MediaType::MSE;
    const std::string m_kMimeType = "mime";
    const std::string m_kUrl = "mse://1";

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
 * Test that Load returns success if the IPC API succeeds.
 */
TEST_F(RialtoClientMediaPipelineLoadTest, Success)
{
    EXPECT_CALL(*m_mediaPipelineIpcMock, load(m_type, m_kMimeType, m_kUrl)).WillOnce(Return(true));

    EXPECT_EQ(m_mediaPipeline->load(m_type, m_kMimeType, m_kUrl), true);
}

/**
 * Test that Load returns failure if the IPC API fails.
 */
TEST_F(RialtoClientMediaPipelineLoadTest, Failure)
{
    EXPECT_CALL(*m_mediaPipelineIpcMock, load(m_type, m_kMimeType, m_kUrl)).WillOnce(Return(false));

    EXPECT_EQ(m_mediaPipeline->load(m_type, m_kMimeType, m_kUrl), false);
}
