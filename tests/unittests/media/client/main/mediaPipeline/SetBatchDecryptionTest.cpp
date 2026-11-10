/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
 * Copyright 2026 Liberty Global Service B.V.
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

class RialtoClientMediaPipelineSetBatchDecryptionTest : public MediaPipelineTestBase
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
 * Test that setBatchDecryption returns success if the IPC API succeeds.
 */
TEST_F(RialtoClientMediaPipelineSetBatchDecryptionTest, SetBatchDecryptionSuccess)
{
    EXPECT_CALL(*m_mediaPipelineIpcMock, setBatchDecryption(true)).WillOnce(Return(true));
    EXPECT_TRUE(m_mediaPipeline->setBatchDecryption(true));
}

/**
 * Test that setBatchDecryption returns failure if the IPC API fails.
 */
TEST_F(RialtoClientMediaPipelineSetBatchDecryptionTest, SetBatchDecryptionFailure)
{
    EXPECT_CALL(*m_mediaPipelineIpcMock, setBatchDecryption(false)).WillOnce(Return(false));
    EXPECT_FALSE(m_mediaPipeline->setBatchDecryption(false));
}
