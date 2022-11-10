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
using ::testing::Invoke;
using ::testing::Return;
using ::testing::StrictMock;

class RialtoServerMediaPipelineMiscellaneousFunctionsTest : public ::testing::Test
{
protected:
    std::shared_ptr<IMediaPipelineClient> m_mediaPipelineClient;
    std::unique_ptr<IMediaPipelineServerInternal> m_mediaPipeline;
    std::shared_ptr<StrictMock<GstPlayerFactoryMock>> m_gstPlayerFactoryMock;
    StrictMock<GstPlayerMock> *m_gstPlayerMock = nullptr;
    std::shared_ptr<StrictMock<SharedMemoryBufferMock>> m_sharedMemoryBufferMock;
    std::unique_ptr<IDataReaderFactory> m_dataReaderFactoryMock;
    std::unique_ptr<IActiveRequests> m_activeRequestsMock;
    StrictMock<DecryptionServiceMock> m_decryptionServiceMock;
    const int m_kSessionId{1};
    const int64_t m_kPosition{4028596027};
    const double m_kPlaybackRate{1.5};

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
        MediaPipelineClientMock &mediaPipelineClientMock =
            dynamic_cast<MediaPipelineClientMock &>(*m_mediaPipelineClient);
        EXPECT_CALL(mediaPipelineClientMock, notifyNetworkState(NetworkState::BUFFERING));

        EXPECT_EQ(m_mediaPipeline->load(MediaType::MSE, "mime", "mse://1"), true);
    }
};

/**
 * Test that Play returns success if the gstreamer player API succeeds and sets the pipeline state.
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, PlaySuccess)
{
    LoadGstPlayer();

    EXPECT_CALL(*m_gstPlayerMock, play());
    EXPECT_TRUE(m_mediaPipeline->play());
}

/**
 * Test that Play returns failure if the gstreamer player is not initialized
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, PlayFailureDueToUninitializedPlayer)
{
    EXPECT_FALSE(m_mediaPipeline->play());
}

/**
 * Test that Stop returns success if the gstreamer player API succeeds and sets the pipeline state.
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, StopSuccess)
{
    LoadGstPlayer();

    EXPECT_CALL(*m_gstPlayerMock, stop());
    EXPECT_TRUE(m_mediaPipeline->stop());
}

/**
 * Test that Stop returns failure if the gstreamer player is not initialized
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, StopFailureDueToUninitializedPlayer)
{
    EXPECT_FALSE(m_mediaPipeline->stop());
}

/**
 * Test that Pause returns success if the gstreamer player API succeeds and sets the pipeline state.
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, PauseSuccess)
{
    LoadGstPlayer();

    EXPECT_CALL(*m_gstPlayerMock, pause());
    EXPECT_TRUE(m_mediaPipeline->pause());
}

/**
 * Test that Pause returns failure if the gstreamer player is not initialized
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, PauseFailureDueToUninitializedPlayer)
{
    EXPECT_FALSE(m_mediaPipeline->pause());
}

/**
 * Test that SetVideoWindow returns success if the gstreamer player API succeeds and sets the video geometry
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetVideoWindowSuccess)
{
    uint32_t x{3};
    uint32_t y{4};
    uint32_t width{640};
    uint32_t height{480};
    LoadGstPlayer();

    EXPECT_CALL(*m_gstPlayerMock, setVideoGeometry(x, y, width, height));
    EXPECT_TRUE(m_mediaPipeline->setVideoWindow(x, y, width, height));
}

/**
 * Test that SetVideoWindow returns failure if the gstreamer player is not initialized
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetVideoWindowFailureDueToUninitializedPlayer)
{
    uint32_t x{3};
    uint32_t y{4};
    uint32_t width{640};
    uint32_t height{480};
    EXPECT_FALSE(m_mediaPipeline->setVideoWindow(x, y, width, height));
}

/**
 * Test that SetPosition returns failure if the gstreamer player is not initialized
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetPositionFailureDueToUninitializedPlayer)
{
    EXPECT_FALSE(m_mediaPipeline->setPosition(m_kPosition));
}

/**
 * Test that SetPosition returns failure if the gstreamer player is not initialized
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetPositionSuccess)
{
    LoadGstPlayer();

    EXPECT_CALL(*m_gstPlayerMock, setPosition(m_kPosition));
    EXPECT_TRUE(m_mediaPipeline->setPosition(m_kPosition));
}

/**
 * Test that SetPlaybackRate returns failure if the gstreamer player is not initialized
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetPlaybackRateFailureDueToUninitializedPlayer)
{
    EXPECT_FALSE(m_mediaPipeline->setPlaybackRate(m_kPlaybackRate));
}

/**
 * Test that SetPlaybackRate returns failure if rate is 0.0
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetPlaybackRateFailureDueToWrongRateValue)
{
    EXPECT_FALSE(m_mediaPipeline->setPlaybackRate(0.0));
}

/**
 * Test that SetPlaybackRate returns failure if the gstreamer API succeeds and sets playback rate
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetPlaybackRateSuccess)
{
    LoadGstPlayer();

    EXPECT_CALL(*m_gstPlayerMock, setPlaybackRate(m_kPlaybackRate));
    EXPECT_TRUE(m_mediaPipeline->setPlaybackRate(m_kPlaybackRate));
}

/**
 * Test that GetPosition returns failure if the gstreamer player is not initialized
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, GetPositionFailureDueToUninitializedPlayer)
{
    int64_t targetPosition{};
    EXPECT_FALSE(m_mediaPipeline->getPosition(targetPosition));
}

/**
 * Test that GetPosition returns failure if the gstreamer API fails
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, GetPositionFailure)
{
    LoadGstPlayer();
    int64_t targetPosition{};
    EXPECT_CALL(*m_gstPlayerMock, getPosition(_)).WillOnce(Return(false));
    EXPECT_FALSE(m_mediaPipeline->getPosition(targetPosition));
}

/**
 * Test that GetPosition returns success if the gstreamer API succeeds and gets playback rate
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, GetPositionSuccess)
{
    LoadGstPlayer();
    int64_t targetPosition{};
    EXPECT_CALL(*m_gstPlayerMock, getPosition(_))
        .WillOnce(Invoke(
            [&](int64_t &pos)
            {
                pos = m_kPosition;
                return true;
            }));
    EXPECT_TRUE(m_mediaPipeline->getPosition(targetPosition));
    EXPECT_EQ(targetPosition, m_kPosition);
}
