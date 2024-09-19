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

class RialtoClientMediaPipelineSetVolumeTest : public MediaPipelineTestBase
{
protected:
    const double m_kTargetVolume{0.7};
    const uint32_t m_kVolumeDuration = 0;
    const firebolt::rialto::EaseType m_kEaseType = firebolt::rialto::EaseType::EASE_LINEAR;

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
 * Test that setVolume returns success if the IPC API succeeds.
 */
TEST_F(RialtoClientMediaPipelineSetVolumeTest, setVolumeSuccess)
{
    EXPECT_CALL(*m_mediaPipelineIpcMock, setVolume(m_kTargetVolume, m_kVolumeDuration, m_kEaseType)).WillOnce(Return(true));

    EXPECT_EQ(m_mediaPipeline->setVolume(m_kTargetVolume, m_kVolumeDuration, m_kEaseType), true);
}

/**
 * Test that setVolume returns success when only volume is passed, and ease type and duration are default.
 */
TEST_F(RialtoClientMediaPipelineSetVolumeTest, setVolumeWithNoEaseTypeAndDuration)
{
    EXPECT_CALL(*m_mediaPipelineIpcMock, setVolume(m_kTargetVolume, m_kVolumeDuration, m_kEaseType)).WillOnce(Return(true));

    auto mediaPipeline = dynamic_cast<IMediaPipeline*>(m_mediaPipeline.get());
    ASSERT_NE(mediaPipeline, nullptr);

    EXPECT_EQ(mediaPipeline->setVolume(m_kTargetVolume), true);
}
/**
 * Test that setVolume returns failure if the IPC API fails.
 */
TEST_F(RialtoClientMediaPipelineSetVolumeTest, setVolumeFailure)
{
    EXPECT_CALL(*m_mediaPipelineIpcMock, setVolume(m_kTargetVolume, m_kVolumeDuration, m_kEaseType)).WillOnce(Return(false));

    EXPECT_EQ(m_mediaPipeline->setVolume(m_kTargetVolume, m_kVolumeDuration, m_kEaseType), false);
}
