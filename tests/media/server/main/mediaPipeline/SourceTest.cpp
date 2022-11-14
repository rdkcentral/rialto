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

class RialtoServerMediaPipelineSourceTest : public MediaPipelineTestBase
{
protected:
    int32_t m_id = 456;
    MediaSourceType m_type = MediaSourceType::AUDIO;
    const char *m_kCaps = "CAPS";

    RialtoServerMediaPipelineSourceTest() { createMediaPipeline(); }

    ~RialtoServerMediaPipelineSourceTest() { destroyMediaPipeline(); }
};

/**
 * Test that AttachSource returns success if the gstreamer player API succeeds and sets the source id.
 */
TEST_F(RialtoServerMediaPipelineSourceTest, AttachSourceSuccess)
{
    IMediaPipeline::MediaSource m_mediaSource(m_id, m_type, m_kCaps);

    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();

    EXPECT_CALL(*m_gstPlayerMock, attachSource(m_type, m_kCaps));

    EXPECT_EQ(m_mediaPipeline->attachSource(m_mediaSource), true);
    EXPECT_EQ(m_mediaSource.getId(), static_cast<int32_t>(m_type));
}

/**
 * Test that AttachSource fails if load has not been called (no gstreamer player).
 */
TEST_F(RialtoServerMediaPipelineSourceTest, NoGstPlayerFailure)
{
    IMediaPipeline::MediaSource m_mediaSource(m_id, m_type, m_kCaps);

    mainThreadWillEnqueueTaskAndWait();
    EXPECT_EQ(m_mediaPipeline->attachSource(m_mediaSource), false);
    EXPECT_EQ(m_mediaSource.getId(), -1);
}

/**
 * Test that AttachSource fails if the media source type is unknown.
 */
TEST_F(RialtoServerMediaPipelineSourceTest, TypeUnknownFailure)
{
    IMediaPipeline::MediaSource m_mediaSource(m_id, MediaSourceType::UNKNOWN, m_kCaps);

    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();

    EXPECT_CALL(*m_gstPlayerMock, attachSource(_, _)).Times(0);

    EXPECT_EQ(m_mediaPipeline->attachSource(m_mediaSource), false);
    EXPECT_EQ(m_mediaSource.getId(), -1);
}
