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

using ::testing::ByMove;

MATCHER_P(VideoRequirementsMatcher, expectedReq, "")
{
    return ((expectedReq.maxWidth == arg.maxWidth) && (expectedReq.maxHeight == arg.maxHeight));
}

class RialtoServerMediaPipelineLoadTest : public MediaPipelineTestBase
{
protected:
    MediaType m_type = MediaType::MSE;
    const std::string m_kMimeType = "mime";
    const std::string m_kUrl = "mse://1";

    RialtoServerMediaPipelineLoadTest() { createMediaPipeline(); }

    ~RialtoServerMediaPipelineLoadTest() { destroyMediaPipeline(); }
};

/**
 * Test that Load returns success if create gstreamer player succeeds.
 */
TEST_F(RialtoServerMediaPipelineLoadTest, Success)
{
    mainThreadWillEnqueueTaskAndWait();
    mainThreadWillEnqueueTask();
    EXPECT_CALL(*m_gstPlayerFactoryMock, createGstGenericPlayer(_, _, m_type, VideoRequirementsMatcher(m_videoReq), _))
        .WillOnce(Return(ByMove(std::move(m_gstPlayer))));
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyNetworkState(NetworkState::BUFFERING));

    EXPECT_EQ(m_mediaPipeline->load(m_type, m_kMimeType, m_kUrl), true);
}

/**
 * Test that Load returns failure if the create gstreamer player API fails.
 * No update of NetworkState.
 */
TEST_F(RialtoServerMediaPipelineLoadTest, CreateGstPlayerFailure)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerFactoryMock, createGstGenericPlayer(_, _, m_type, VideoRequirementsMatcher(m_videoReq), _))
        .WillOnce(Return(ByMove(nullptr)));
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyNetworkState(_)).Times(0);

    EXPECT_EQ(m_mediaPipeline->load(m_type, m_kMimeType, m_kUrl), false);
}
