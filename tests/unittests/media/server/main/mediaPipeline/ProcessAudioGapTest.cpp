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

using ::testing::Ref;

class RialtoServerMediaPipelineProcessAudioGapTest : public MediaPipelineTestBase
{
protected:
    const int64_t m_kPosition{4321};
    const uint32_t m_kDuration{1234};
    const uint32_t m_kLevel{1};

    RialtoServerMediaPipelineProcessAudioGapTest() { createMediaPipeline(); }

    ~RialtoServerMediaPipelineProcessAudioGapTest() { destroyMediaPipeline(); }
};

/**
 * Test that ProcessAudioGap returns success if the gstreamer player API succeeds.
 */
TEST_F(RialtoServerMediaPipelineProcessAudioGapTest, ProcessAudioGapSuccess)
{
    loadGstPlayer();

    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, processAudioGap(m_kPosition, m_kDuration, m_kLevel));
    EXPECT_TRUE(m_mediaPipeline->processAudioGap(m_kPosition, m_kDuration, m_kLevel));
}

/**
 * Test that ProcessAudioGap fails if load has not been called (no gstreamer player).
 */
TEST_F(RialtoServerMediaPipelineProcessAudioGapTest, ProcessAudioGapNoGstPlayerFailure)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_FALSE(m_mediaPipeline->processAudioGap(m_kPosition, m_kDuration, m_kLevel));
}
