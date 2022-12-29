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

class RialtoClientMediaPipelineSetPositionTest : public MediaPipelineTestBase
{
protected:
    int64_t m_position = 123;

    virtual void SetUp()
    {
        MediaPipelineTestBase::SetUp();

        createMediaPipeline();

        // Set the MediaPipeline state to playing by default
        setPlaybackState(PlaybackState::PLAYING);
    }

    virtual void TearDown()
    {
        destroyMediaPipeline();

        MediaPipelineTestBase::TearDown();
    }

    void setPosition()
    {
        EXPECT_CALL(*m_mediaPipelineIpcMock, setPosition(m_position)).WillOnce(Return(true));

        EXPECT_EQ(m_mediaPipeline->setPosition(m_position), true);
    }

    void testNeedDataRequestRemovedOnSetPosition(PlaybackState state)
    {
        // Init data parameters
        int32_t sourceId = 1;
        size_t frameCount = 2;
        uint32_t requestId1 = 4U;
        uint32_t requestId2 = 5U;
        MediaSourceStatus status = MediaSourceStatus::NO_AVAILABLE_SAMPLES;
        std::shared_ptr<MediaPlayerShmInfo> shmInfo = std::make_shared<MediaPlayerShmInfo>();
        IMediaPipeline::MediaSegmentVector dataVec;
        shmInfo->maxMetadataBytes = 5;
        shmInfo->metadataOffset = 6;
        shmInfo->mediaDataOffset = 7;
        shmInfo->maxMediaBytes = 3U;

        // Notify needData so we can check they are discarded
        setPlaybackState(PlaybackState::PLAYING);
        needData(sourceId, frameCount, requestId1, shmInfo);
        needData(sourceId, frameCount, requestId2, shmInfo);

        // Set the position
        setPlaybackState(state);
        EXPECT_CALL(*m_mediaPipelineIpcMock, setPosition(m_position)).WillOnce(Return(true));
        EXPECT_EQ(m_mediaPipeline->setPosition(m_position), true);

        // needData requests should have been removed
        setPlaybackState(PlaybackState::PLAYING);
        EXPECT_FALSE(m_mediaPipeline->haveData(status, requestId1));
        EXPECT_FALSE(m_mediaPipeline->haveData(status, requestId2));
    }

    void testNeedDataRequestRemovedOnSetPosition(NetworkState state)
    {
        // Init data parameters
        int32_t sourceId = 1;
        size_t frameCount = 2;
        uint32_t requestId1 = 4U;
        uint32_t requestId2 = 5U;
        MediaSourceStatus status = MediaSourceStatus::NO_AVAILABLE_SAMPLES;
        std::shared_ptr<MediaPlayerShmInfo> shmInfo = std::make_shared<MediaPlayerShmInfo>();
        IMediaPipeline::MediaSegmentVector dataVec;
        shmInfo->maxMetadataBytes = 5;
        shmInfo->metadataOffset = 6;
        shmInfo->mediaDataOffset = 7;
        shmInfo->maxMediaBytes = 3U;

        // Notify needData so we can check they are discarded
        setPlaybackState(PlaybackState::PLAYING);
        needData(sourceId, frameCount, requestId1, shmInfo);
        needData(sourceId, frameCount, requestId2, shmInfo);

        // Set the position
        setNetworkState(state);
        EXPECT_CALL(*m_mediaPipelineIpcMock, setPosition(m_position)).WillOnce(Return(true));
        EXPECT_EQ(m_mediaPipeline->setPosition(m_position), true);

        // NeedData requests should have been removed
        setPlaybackState(PlaybackState::PLAYING);
        EXPECT_FALSE(m_mediaPipeline->haveData(status, requestId1));
        EXPECT_FALSE(m_mediaPipeline->haveData(status, requestId2));
    }
};

/**
 * Test that setPosition requests position change over ipc and removes the needDataRequests in valid states.
 */
TEST_F(RialtoClientMediaPipelineSetPositionTest, ValidStates)
{
    std::vector<PlaybackState> validPlaybackStates = {PlaybackState::PLAYING, PlaybackState::END_OF_STREAM,
                                                      PlaybackState::SEEKING, PlaybackState::FLUSHED};
    std::vector<NetworkState> validNetworkStates = {NetworkState::BUFFERING};

    for (auto it = validPlaybackStates.begin(); it != validPlaybackStates.end(); it++)
    {
        testNeedDataRequestRemovedOnSetPosition(*it);
    }

    for (auto it = validNetworkStates.begin(); it != validNetworkStates.end(); it++)
    {
        testNeedDataRequestRemovedOnSetPosition(*it);
    }
}

/**
 * Test that setPosition does not request position change over ipc in invalid states.
 */
TEST_F(RialtoClientMediaPipelineSetPositionTest, InvalidStates)
{
    std::vector<PlaybackState> invalidPlaybackStates = {PlaybackState::STOPPED, PlaybackState::FAILURE};

    for (auto it = invalidPlaybackStates.begin(); it != invalidPlaybackStates.end(); it++)
    {
        setPlaybackState(*it);
        EXPECT_EQ(m_mediaPipeline->setPosition(m_position), false);
    }
}
