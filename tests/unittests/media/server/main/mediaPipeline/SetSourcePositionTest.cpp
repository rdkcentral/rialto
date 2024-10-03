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

class RialtoServerMediaPipelineSetSourcePositionTest : public MediaPipelineTestBase
{
protected:
    const MediaSourceType m_kType{MediaSourceType::VIDEO};
    const char *m_kMimeType{"video/mpeg"};
    const int64_t m_kPosition{4321};
    const int m_kDummySourceId{123};
    const bool m_kResetTime{false};
    const double m_kAppliedRate{2.0};
    const uint64_t m_kRunningTime{4534};

    RialtoServerMediaPipelineSetSourcePositionTest() { createMediaPipeline(); }

    ~RialtoServerMediaPipelineSetSourcePositionTest() { destroyMediaPipeline(); }
};

/**
 * Test that SetSourcePosition returns success if the gstreamer player API succeeds.
 */
TEST_F(RialtoServerMediaPipelineSetSourcePositionTest, SetSourcePositionSuccess)
{
    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceVideo>(m_kMimeType);

    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();

    EXPECT_CALL(*m_gstPlayerMock, attachSource(Ref(mediaSource)));
    EXPECT_EQ(m_mediaPipeline->attachSource(mediaSource), true);
    std::int32_t sourceId{mediaSource->getId()};

    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, setSourcePosition(m_kType, m_kPosition, m_kResetTime, m_kAppliedRate, m_kRunningTime));
    EXPECT_TRUE(m_mediaPipeline->setSourcePosition(sourceId, m_kPosition, m_kResetTime, m_kAppliedRate, m_kRunningTime));
}

/**
 * Test that SetSourcePosition fails if load has not been called (no gstreamer player).
 */
TEST_F(RialtoServerMediaPipelineSetSourcePositionTest, SetSourcePositionNoGstPlayerFailure)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_FALSE(m_mediaPipeline->setSourcePosition(m_kDummySourceId, m_kPosition, m_kResetTime, m_kAppliedRate,
                                                    m_kRunningTime));
}

/**
 * Test that SetSourcePosition fails if source is not present.
 */
TEST_F(RialtoServerMediaPipelineSetSourcePositionTest, SetSourcePositionNoSourcePresent)
{
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();

    EXPECT_FALSE(m_mediaPipeline->setSourcePosition(m_kDummySourceId, m_kPosition, m_kResetTime, m_kAppliedRate,
                                                    m_kRunningTime));
}

/**
 * Test that SetSourcePosition resets the Eos flag on success
 */
TEST_F(RialtoServerMediaPipelineSetSourcePositionTest, SetSourcePositionResetEos)
{
    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceVideo>(m_kMimeType);

    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();

    EXPECT_CALL(*m_gstPlayerMock, attachSource(Ref(mediaSource)));
    EXPECT_EQ(m_mediaPipeline->attachSource(mediaSource), true);
    std::int32_t sourceId{mediaSource->getId()};
    setEos(firebolt::rialto::MediaSourceType::VIDEO);

    mainThreadWillEnqueueTaskAndWait();

    EXPECT_CALL(*m_gstPlayerMock, setSourcePosition(m_kType, m_kPosition, m_kResetTime, m_kAppliedRate, m_kRunningTime));
    EXPECT_TRUE(m_mediaPipeline->setSourcePosition(sourceId, m_kPosition, m_kResetTime, m_kAppliedRate, m_kRunningTime));

    // Expect need data notified to client
    expectNotifyNeedData(firebolt::rialto::MediaSourceType::VIDEO, sourceId, 3);
    m_gstPlayerCallback->notifyNeedMediaData(firebolt::rialto::MediaSourceType::VIDEO);
}
