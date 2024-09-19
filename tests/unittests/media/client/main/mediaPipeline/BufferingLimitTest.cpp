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

class RialtoClientMediaPipelineBufferingLimitTest : public MediaPipelineTestBase
{
protected:
    const uint32_t m_kBufferingLimit{1234};

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
 * Test that SetBufferingLimit returns success if the IPC API succeeds.
 */
TEST_F(RialtoClientMediaPipelineBufferingLimitTest, SetSuccess)
{
    EXPECT_CALL(*m_mediaPipelineIpcMock, setBufferingLimit(m_kBufferingLimit)).WillOnce(Return(true));

    EXPECT_EQ(m_mediaPipeline->setBufferingLimit(m_kBufferingLimit), true);
}

/**
 * Test that SetBufferingLimit returns failure if the IPC API fails.
 */
TEST_F(RialtoClientMediaPipelineBufferingLimitTest, SetFailure)
{
    EXPECT_CALL(*m_mediaPipelineIpcMock, setBufferingLimit(m_kBufferingLimit)).WillOnce(Return(false));

    EXPECT_EQ(m_mediaPipeline->setBufferingLimit(m_kBufferingLimit), false);
}

/**
 * Test that GetBufferingLimit returns success if the IPC API succeeds.
 */
TEST_F(RialtoClientMediaPipelineBufferingLimitTest, GetSuccess)
{
    uint32_t bufferingLimit{0};
    EXPECT_CALL(*m_mediaPipelineIpcMock, getBufferingLimit(_))
        .WillOnce(DoAll(SetArgReferee<0>(m_kBufferingLimit), Return(true)));

    EXPECT_EQ(m_mediaPipeline->getBufferingLimit(bufferingLimit), true);
    EXPECT_EQ(m_kBufferingLimit, bufferingLimit);
}

/**
 * Test that GetBufferingLimit returns failure if the IPC API fails.
 */
TEST_F(RialtoClientMediaPipelineBufferingLimitTest, GetFailure)
{
    uint32_t bufferingLimit{0};
    EXPECT_CALL(*m_mediaPipelineIpcMock, getBufferingLimit(_)).WillOnce(Return(false));

    EXPECT_EQ(m_mediaPipeline->getBufferingLimit(bufferingLimit), false);
}
