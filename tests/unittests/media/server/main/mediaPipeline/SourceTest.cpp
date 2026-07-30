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
        std::make_unique<IMediaPipeline::MediaSourceAudio>(m_kMimeType);

    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();

    EXPECT_CALL(*m_gstPlayerMock, attachSource(Ref(mediaSource)));

    EXPECT_EQ(m_mediaPipeline->attachSource(mediaSource), true);
    EXPECT_NE(mediaSource->getId(), -1);
}

/**
 * Test attach audio source with a specific configuration.
 */
TEST_F(RialtoServerMediaPipelineSourceTest, AttachAudioSourceWitSpecificConfiguration)
{
    AudioConfig audioConfig{6, 48000, {1, 2, 3}};
    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceAudio>(m_kMimeType, true, audioConfig);

    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();

    EXPECT_CALL(*m_gstPlayerMock, attachSource(Ref(mediaSource)));

    EXPECT_EQ(m_mediaPipeline->attachSource(mediaSource), true);
    EXPECT_NE(mediaSource->getId(), -1);
}

/**
 * Test that AttachSource fails if load has not been called (no gstreamer player).
 */
TEST_F(RialtoServerMediaPipelineSourceTest, NoGstPlayerFailure)
{
    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceAudio>(m_kMimeType);

    mainThreadWillEnqueueTaskAndWait();
    EXPECT_EQ(m_mediaPipeline->attachSource(mediaSource), false);
    EXPECT_EQ(mediaSource->getId(), -1);
}

/**
 * Test that RemoveSource returns success if the gstreamer player API succeeds.
 */
TEST_F(RialtoServerMediaPipelineSourceTest, RemoveSourceSuccess)
{
    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceAudio>(m_kMimeType);

    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();

    EXPECT_CALL(*m_gstPlayerMock, attachSource(Ref(mediaSource)));
    EXPECT_EQ(m_mediaPipeline->attachSource(mediaSource), true);
    std::int32_t sourceId{mediaSource->getId()};

    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, removeSource(MediaSourceType::AUDIO));
    EXPECT_EQ(m_mediaPipeline->removeSource(sourceId), true);
}

/**
 * Test that RemoveSource fails if load has not been called (no gstreamer player).
 */
TEST_F(RialtoServerMediaPipelineSourceTest, RemoveSourceNoGstPlayerFailure)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_EQ(m_mediaPipeline->removeSource(m_id), false);
}

/**
 * Test that RemoveSource fails if source is not present.
 */
TEST_F(RialtoServerMediaPipelineSourceTest, RemoveSourceNoSourcePresent)
{
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();

    EXPECT_FALSE(m_mediaPipeline->removeSource(m_id));
}

/**
 * Test that after attaching, removing and attaching the same MediaSourceType, new sourceId is generated
 */
TEST_F(RialtoServerMediaPipelineSourceTest, AttachRemoveAttachSourceDifferentId)
{
    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceAudio>(m_kMimeType);

    loadGstPlayer();

    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, attachSource(Ref(mediaSource)));
    EXPECT_EQ(m_mediaPipeline->attachSource(mediaSource), true);
    std::int32_t firstSourceId{mediaSource->getId()};

    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, removeSource(MediaSourceType::AUDIO));
    EXPECT_EQ(m_mediaPipeline->removeSource(firstSourceId), true);

    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, attachSource(Ref(mediaSource)));
    EXPECT_EQ(m_mediaPipeline->attachSource(mediaSource), true);

    EXPECT_NE(mediaSource->getId(), firstSourceId);
}

/**
 * Test that attachSource fails when the same source type is attached for a second time
 */
TEST_F(RialtoServerMediaPipelineSourceTest, UpdateSourceIdNotChanged)
{
    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceAudio>(m_kMimeType);

    loadGstPlayer();

    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, attachSource(Ref(mediaSource)));
    EXPECT_EQ(m_mediaPipeline->attachSource(mediaSource), true);

    mediaSource->setId(-1);

    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, attachSource(Ref(mediaSource)));
    EXPECT_EQ(m_mediaPipeline->attachSource(mediaSource), false);
}

/**
 * Test that AllSourcesAttached returns success if the gstreamer player API succeeds.
 */
TEST_F(RialtoServerMediaPipelineSourceTest, AllSourcesAttachedSuccess)
{
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();

    EXPECT_CALL(*m_gstPlayerMock, allSourcesAttached());
    EXPECT_EQ(m_mediaPipeline->allSourcesAttached(), true);
}

/**
 * Test that AllSourcesAttached fails if there is no gstreamer player.
 */
TEST_F(RialtoServerMediaPipelineSourceTest, AllSourcesAttachedNoGstPlayerFailure)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_EQ(m_mediaPipeline->allSourcesAttached(), false);
}

/**
 * Test that AllSourcesAttached fails if called for a second time
 */
TEST_F(RialtoServerMediaPipelineSourceTest, AllSourcesAttachedCalledTwiceFailure)
{
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();

    EXPECT_CALL(*m_gstPlayerMock, allSourcesAttached());
    EXPECT_EQ(m_mediaPipeline->allSourcesAttached(), true);

    mainThreadWillEnqueueTaskAndWait();
    EXPECT_EQ(m_mediaPipeline->allSourcesAttached(), false);
}

/**
 * Test that SwitchSource returns success if the gstreamer player API succeeds and sets the source id.
 */
TEST_F(RialtoServerMediaPipelineSourceTest, SwitchSourceSuccess)
{
    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceAudio>(m_kMimeType);

    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();

    EXPECT_CALL(*m_gstPlayerMock, switchSource(Ref(mediaSource)));

    EXPECT_EQ(m_mediaPipeline->switchSource(mediaSource), true);
}

/**
 * Test that SwitchSource fails if load has not been called (no gstreamer player).
 */
TEST_F(RialtoServerMediaPipelineSourceTest, SwitchSourceNoGstPlayerFailure)
{
    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceAudio>(m_kMimeType);

    mainThreadWillEnqueueTaskAndWait();
    EXPECT_EQ(m_mediaPipeline->switchSource(mediaSource), false);
}
