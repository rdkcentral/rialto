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

class RialtoServerMediaPipelineSetSubtitleOffsetTest : public MediaPipelineTestBase
{
protected:
    const MediaSourceType m_kType{MediaSourceType::SUBTITLE};
    const char *m_kMimeType{"text/plain"};
    const char *m_kTextTrackId{"subtitle1"};
    const int64_t m_kPosition{1234567890};
    const int m_kDummySourceId{123};

    RialtoServerMediaPipelineSetSubtitleOffsetTest() { createMediaPipeline(); }

    ~RialtoServerMediaPipelineSetSubtitleOffsetTest() { destroyMediaPipeline(); }
};

/**
 * Test that SetSubtitleOffset returns success if the gstreamer player API succeeds.
 */
TEST_F(RialtoServerMediaPipelineSetSubtitleOffsetTest, SetSubtitleOffsetSuccess)
{
    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceSubtitle>(m_kMimeType, m_kTextTrackId);

    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();

    EXPECT_CALL(*m_gstPlayerMock, attachSource(Ref(mediaSource)));
    EXPECT_EQ(m_mediaPipeline->attachSource(mediaSource), true);
    std::int32_t sourceId{mediaSource->getId()};

    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, setSubtitleOffset(m_kPosition));
    EXPECT_TRUE(m_mediaPipeline->setSubtitleOffset(sourceId, m_kPosition));
}

/**
 * Test that SetSubtitleOffset fails if load has not been called (no gstreamer player).
 */
TEST_F(RialtoServerMediaPipelineSetSubtitleOffsetTest, SetSubtitleOffsetNoGstPlayerFailure)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_FALSE(m_mediaPipeline->setSubtitleOffset(m_kDummySourceId, m_kPosition));
}

/**
 * Test that SetSubtitleOffset fails if source is not present.
 */
TEST_F(RialtoServerMediaPipelineSetSubtitleOffsetTest, SetSubtitleOffsetNoSourcePresent)
{
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();

    EXPECT_FALSE(m_mediaPipeline->setSubtitleOffset(m_kDummySourceId, m_kPosition));
}