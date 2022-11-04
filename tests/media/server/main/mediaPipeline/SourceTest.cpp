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

#include "ActiveRequestsMock.h"
#include "DataReaderFactoryMock.h"
#include "DecryptionServiceMock.h"
#include "GstPlayerFactoryMock.h"
#include "GstPlayerMock.h"
#include "MediaPipelineClientMock.h"
#include "MediaPipelineServerInternal.h"
#include "SharedMemoryBufferMock.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;
using namespace firebolt::rialto::server;

using ::testing::_;
using ::testing::ByMove;
using ::testing::Return;
using ::testing::StrictMock;

class RialtoServerMediaPipelineSourceTest : public ::testing::Test
{
protected:
    std::shared_ptr<StrictMock<MediaPipelineClientMock>> m_mediaPipelineClient;
    std::unique_ptr<MediaPipelineServerInternal> m_mediaPipeline;
    std::shared_ptr<StrictMock<GstPlayerFactoryMock>> m_gstPlayerFactoryMock;
    StrictMock<GstPlayerMock> *m_gstPlayerMock = nullptr;
    std::shared_ptr<StrictMock<SharedMemoryBufferMock>> m_sharedMemoryBufferMock;
    std::unique_ptr<IDataReaderFactory> m_dataReaderFactoryMock;
    std::unique_ptr<IActiveRequests> m_activeRequestsMock;
    StrictMock<DecryptionServiceMock> m_decryptionServiceMock;
    const int m_kSessionId{1};
    int32_t m_id = 456;
    MediaSourceType m_type = MediaSourceType::AUDIO;
    const char *m_kCaps = "CAPS";

    virtual void SetUp()
    {
        m_mediaPipelineClient = std::make_shared<StrictMock<MediaPipelineClientMock>>();

        m_gstPlayerFactoryMock = std::make_shared<StrictMock<GstPlayerFactoryMock>>();
        m_sharedMemoryBufferMock = std::make_shared<StrictMock<SharedMemoryBufferMock>>();

        createMediaPipeline();
    }

    virtual void TearDown()
    {
        EXPECT_CALL(*m_sharedMemoryBufferMock, unmapPartition(m_kSessionId)).WillOnce(Return(true));
        m_mediaPipeline.reset();

        m_gstPlayerFactoryMock.reset();
        m_gstPlayerMock = nullptr;

        m_mediaPipelineClient.reset();
    }

    void createMediaPipeline()
    {
        VideoRequirements videoReq = {};

        EXPECT_CALL(*m_sharedMemoryBufferMock, mapPartition(m_kSessionId)).WillOnce(Return(true));
        EXPECT_NO_THROW(
            m_mediaPipeline =
                std::make_unique<MediaPipelineServerInternal>(m_mediaPipelineClient, videoReq, m_gstPlayerFactoryMock,
                                                              m_kSessionId, m_sharedMemoryBufferMock,
                                                              std::move(m_dataReaderFactoryMock),
                                                              std::move(m_activeRequestsMock), m_decryptionServiceMock););
        EXPECT_NE(m_mediaPipeline, nullptr);
    }

    void LoadGstPlayer()
    {
        std::unique_ptr<StrictMock<GstPlayerMock>> gstPlayerMock = std::make_unique<StrictMock<GstPlayerMock>>();

        // Save a raw pointer to the unique object for use when testing mocks
        // Object shall be freed by the holder of the unique ptr on destruction
        m_gstPlayerMock = gstPlayerMock.get();

        EXPECT_CALL(*m_gstPlayerFactoryMock, createGstPlayer(_, _, _)).WillOnce(Return(ByMove(std::move(gstPlayerMock))));
        EXPECT_CALL(*m_mediaPipelineClient, notifyNetworkState(NetworkState::BUFFERING));

        EXPECT_EQ(m_mediaPipeline->load(MediaType::MSE, "mime", "mse://1"), true);
    }
};

/**
 * Test that AttachSource returns success if the gstreamer player API succeeds and sets the source id.
 */
TEST_F(RialtoServerMediaPipelineSourceTest, AttachSourceSuccess)
{
    IMediaPipeline::MediaSource m_mediaSource(m_id, m_type, m_kCaps);

    LoadGstPlayer();

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

    EXPECT_EQ(m_mediaPipeline->attachSource(m_mediaSource), false);
    EXPECT_EQ(m_mediaSource.getId(), -1);
}

/**
 * Test that AttachSource fails if the media source type is unknown.
 */
TEST_F(RialtoServerMediaPipelineSourceTest, TypeUnknownFailure)
{
    IMediaPipeline::MediaSource m_mediaSource(m_id, MediaSourceType::UNKNOWN, m_kCaps);

    LoadGstPlayer();

    EXPECT_CALL(*m_gstPlayerMock, attachSource(_, _)).Times(0);

    EXPECT_EQ(m_mediaPipeline->attachSource(m_mediaSource), false);
    EXPECT_EQ(m_mediaSource.getId(), -1);
}
