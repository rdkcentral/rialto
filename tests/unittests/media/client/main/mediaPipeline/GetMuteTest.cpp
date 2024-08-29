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

class RialtoClientMediaPipelineGetMuteTest : public MediaPipelineTestBase
{
protected:
    const int32_t m_kSourceId{1};
    bool m_mute{};

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
 * Test that getMute returns success if the IPC API succeeds.
 */
TEST_F(RialtoClientMediaPipelineGetMuteTest, getMuteSuccess)
{
    bool returnMute = !m_mute;

    EXPECT_CALL(*m_mediaPipelineIpcMock, getMute(m_kSourceId, returnMute))
        .WillOnce(DoAll(SetArgReferee<1>(m_mute), Return(true)));

    EXPECT_EQ(m_mediaPipeline->getMute(m_kSourceId, returnMute), true);
    EXPECT_EQ(returnMute, m_mute);
}

/**
 * Test that getMute returns failure if the IPC API fails.
 */
TEST_F(RialtoClientMediaPipelineGetMuteTest, getMuteFailure)
{
    EXPECT_CALL(*m_mediaPipelineIpcMock, getMute(m_kSourceId, m_mute)).WillOnce(Return(false));

    EXPECT_EQ(m_mediaPipeline->getMute(m_kSourceId, m_mute), false);
}
