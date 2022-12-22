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

using ::testing::Ref;

class RialtoServerMediaPipelineSourceTest : public MediaPipelineTestBase
{
protected:
    int32_t m_id = 456;
    MediaSourceType m_type = MediaSourceType::AUDIO;
    const char *m_kMimeType = "video/mpeg";

    RialtoServerMediaPipelineSourceTest() { createMediaPipeline(); }

    ~RialtoServerMediaPipelineSourceTest() { destroyMediaPipeline(); }
};

/**
 * Test that AttachSource returns success if the gstreamer player API succeeds and sets the source id.
 */
TEST_F(RialtoServerMediaPipelineSourceTest, AttachSourceSuccess)
{
    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceAudio>(-1, m_kMimeType);

    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();

    EXPECT_CALL(*m_gstPlayerMock, attachSource(Ref(mediaSource)));

    EXPECT_EQ(m_mediaPipeline->attachSource(mediaSource), true);
    EXPECT_EQ(mediaSource->getId(), static_cast<int32_t>(MediaSourceType::AUDIO));
}

/**
 * Test attach audio source with a specific configuration.
 */
TEST_F(RialtoServerMediaPipelineSourceTest, AttachAudioSourceWitSpecificConfiguration)
{
    AudioConfig audioConfig{6, 48000, {1, 2, 3}};
    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceAudio>(-1, m_kMimeType, audioConfig);

    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();

    EXPECT_CALL(*m_gstPlayerMock, attachSource(Ref(mediaSource)));

    EXPECT_EQ(m_mediaPipeline->attachSource(mediaSource), true);
    EXPECT_EQ(mediaSource->getId(), static_cast<int32_t>(MediaSourceType::AUDIO));
}

/**
 * Test that AttachSource fails if load has not been called (no gstreamer player).
 */
TEST_F(RialtoServerMediaPipelineSourceTest, NoGstPlayerFailure)
{
    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceAudio>(-1, m_kMimeType);

    mainThreadWillEnqueueTaskAndWait();
    EXPECT_EQ(m_mediaPipeline->attachSource(mediaSource), false);
    EXPECT_EQ(mediaSource->getId(), -1);
}
