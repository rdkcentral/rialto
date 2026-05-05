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
using ::testing::Return;

class RialtoServerMediaPipelineFlushTest : public MediaPipelineTestBase
{
protected:
    const MediaSourceType m_kType{MediaSourceType::VIDEO};
    const char *m_kMimeType{"video/mpeg"};
    const bool m_kResetTime{true};
    const int m_kDummySourceId{123};

    RialtoServerMediaPipelineFlushTest() { createMediaPipeline(); }

    ~RialtoServerMediaPipelineFlushTest() { destroyMediaPipeline(); }
};

/**
 * Test that Flush returns success if the gstreamer player API succeeds.
 */
TEST_F(RialtoServerMediaPipelineFlushTest, FlushSuccess)
{
    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceVideo>(m_kMimeType);

    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();

    EXPECT_CALL(*m_gstPlayerMock, attachSource(Ref(mediaSource)));
    EXPECT_EQ(m_mediaPipeline->attachSource(mediaSource), true);
    std::int32_t sourceId{mediaSource->getId()};

    bool async{false};
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, flush(m_kType, m_kResetTime, async));
    EXPECT_TRUE(m_mediaPipeline->flush(sourceId, m_kResetTime, async));
}

/**
 * Test that Flush fails if load has not been called (no gstreamer player).
 */
TEST_F(RialtoServerMediaPipelineFlushTest, FlushNoGstPlayerFailure)
{
    bool async{false};
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_FALSE(m_mediaPipeline->flush(m_kDummySourceId, m_kResetTime, async));
}

/**
 * Test that Flush fails if source is not present.
 */
TEST_F(RialtoServerMediaPipelineFlushTest, FlushNoSourcePresent)
{
    bool async{false};
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();

    EXPECT_FALSE(m_mediaPipeline->flush(m_kDummySourceId, m_kResetTime, async));
}

/**
 * Test that Flush resets the Eos flag on success
 */
TEST_F(RialtoServerMediaPipelineFlushTest, FlushResetEos)
{
    bool async{false};
    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceVideo>(m_kMimeType);

    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();

    EXPECT_CALL(*m_gstPlayerMock, attachSource(Ref(mediaSource)));
    EXPECT_EQ(m_mediaPipeline->attachSource(mediaSource), true);
    std::int32_t sourceId{mediaSource->getId()};
    setEos(firebolt::rialto::MediaSourceType::VIDEO);

    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, flush(m_kType, m_kResetTime, async));
    EXPECT_TRUE(m_mediaPipeline->flush(sourceId, m_kResetTime, async));

    // Expect need data notified to client
    expectNotifyNeedData(firebolt::rialto::MediaSourceType::VIDEO, sourceId, 3);
    m_gstPlayerCallback->notifyNeedMediaData(firebolt::rialto::MediaSourceType::VIDEO);
}
