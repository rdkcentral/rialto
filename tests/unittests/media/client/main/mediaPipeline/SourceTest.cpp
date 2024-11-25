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
#include "MediaSourceUtil.h"

using ::testing::Ref;

class RialtoClientMediaPipelineSourceTest : public MediaPipelineTestBase
{
protected:
    int32_t m_id = 456;
    MediaSourceType m_type = MediaSourceType::AUDIO;
    const char *m_kMimeType = "video/mpeg";

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
 * Test that AttachSource returns success if the IPC API succeeds and sets the source id.
 */
TEST_F(RialtoClientMediaPipelineSourceTest, AttachSourceSuccess)
{
    int32_t m_newId = 123;
    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceAudio>(m_kMimeType);

    EXPECT_CALL(*m_mediaPipelineIpcMock, attachSource(Ref(mediaSource), _))
        .WillOnce(DoAll(SetArgReferee<1>(m_newId), Return(true)));

    EXPECT_EQ(m_mediaPipeline->attachSource(mediaSource), true);
    EXPECT_EQ(mediaSource->getId(), m_newId);
}

/**
 * Test that AttachSource returns failure if the IPC API fails and the source id remains unchanged.
 */
TEST_F(RialtoClientMediaPipelineSourceTest, AttachSourceFailure)
{
    int32_t m_newId = 123;
    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceAudio>(m_kMimeType);

    EXPECT_CALL(*m_mediaPipelineIpcMock, attachSource(Ref(mediaSource), _))
        .WillOnce(DoAll(SetArgReferee<1>(m_newId), Return(false)));

    EXPECT_EQ(m_mediaPipeline->attachSource(mediaSource), false);
    EXPECT_NE(mediaSource->getId(), m_newId);
}

/**
 * Test that RemoveSource returns success if the IPC API succeeds.
 */
TEST_F(RialtoClientMediaPipelineSourceTest, RemoveSourceSuccess)
{
    EXPECT_CALL(*m_mediaPipelineIpcMock, removeSource(m_id)).WillOnce(Return(true));

    EXPECT_EQ(m_mediaPipeline->removeSource(m_id), true);
}

/**
 * Test that RemoveSource returns failure if the IPC API fails.
 */
TEST_F(RialtoClientMediaPipelineSourceTest, RemoveSourceFailure)
{
    EXPECT_CALL(*m_mediaPipelineIpcMock, removeSource(m_id)).WillOnce(Return(false));

    EXPECT_EQ(m_mediaPipeline->removeSource(m_id), false);
}

/**
 * Test that allSourcesAttached returns success if the IPC API succeeds.
 */
TEST_F(RialtoClientMediaPipelineSourceTest, AllSourcesAttachedSuccess)
{
    EXPECT_CALL(*m_mediaPipelineIpcMock, allSourcesAttached()).WillOnce(Return(true));

    EXPECT_EQ(m_mediaPipeline->allSourcesAttached(), true);
}

/**
 * Test that allSourcesAttached returns failure if the IPC API fails.
 */
TEST_F(RialtoClientMediaPipelineSourceTest, AllSourcesAttachedFailure)
{
    EXPECT_CALL(*m_mediaPipelineIpcMock, allSourcesAttached()).WillOnce(Return(false));

    EXPECT_EQ(m_mediaPipeline->allSourcesAttached(), false);
}

/**
 * Test that SwitchSource returns success if the IPC API succeeds
 */
TEST_F(RialtoClientMediaPipelineSourceTest, SwitchSourceSuccess)
{
    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceAudio>(m_kMimeType);

    EXPECT_CALL(*m_mediaPipelineIpcMock, switchSource(Ref(mediaSource))).WillOnce(Return(true));

    EXPECT_EQ(m_mediaPipeline->switchSource(mediaSource), true);
}

/**
 * Test that SwitchSource returns failure if the IPC API fail
 */
TEST_F(RialtoClientMediaPipelineSourceTest, SwitchSourceFailure)
{
    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource =
        std::make_unique<IMediaPipeline::MediaSourceAudio>(m_kMimeType);

    EXPECT_CALL(*m_mediaPipelineIpcMock, switchSource(Ref(mediaSource))).WillOnce(Return(false));

    EXPECT_EQ(m_mediaPipeline->switchSource(mediaSource), false);
}
