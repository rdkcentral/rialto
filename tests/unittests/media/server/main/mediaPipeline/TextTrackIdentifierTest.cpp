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

using ::testing::DoAll;
using ::testing::Return;
using ::testing::SetArgReferee;

class RialtoServerMediaPipelineTextTrackIdentifierTest : public MediaPipelineTestBase
{
protected:
    const std::string m_kTextTrackIdentifier{"Identifier"};

    RialtoServerMediaPipelineTextTrackIdentifierTest() { createMediaPipeline(); }

    ~RialtoServerMediaPipelineTextTrackIdentifierTest() { destroyMediaPipeline(); }
};

/**
 * Test that SetTextTrackIdentifier returns success if the gstreamer player API succeeds.
 */
TEST_F(RialtoServerMediaPipelineTextTrackIdentifierTest, SetTextTrackIdentifierSuccess)
{
    loadGstPlayer();

    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, setTextTrackIdentifier(m_kTextTrackIdentifier));
    EXPECT_TRUE(m_mediaPipeline->setTextTrackIdentifier(m_kTextTrackIdentifier));
}

/**
 * Test that SetTextTrackIdentifier fails if load has not been called (no gstreamer player).
 */
TEST_F(RialtoServerMediaPipelineTextTrackIdentifierTest, SetTextTrackIdentifierNoGstPlayerFailure)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_FALSE(m_mediaPipeline->setTextTrackIdentifier(m_kTextTrackIdentifier));
}

/**
 * Test that GetTextTrackIdentifier returns success if the gstreamer player API succeeds.
 */
TEST_F(RialtoServerMediaPipelineTextTrackIdentifierTest, GetTextTrackIdentifierSuccess)
{
    std::string textTrackIdentifier;
    loadGstPlayer();

    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, getTextTrackIdentifier(_))
        .WillOnce(DoAll(SetArgReferee<0>(m_kTextTrackIdentifier), Return(true)));
    EXPECT_TRUE(m_mediaPipeline->getTextTrackIdentifier(textTrackIdentifier));
    EXPECT_EQ(m_kTextTrackIdentifier, textTrackIdentifier);
}

/**
 * Test that GetTextTrackIdentifier returns success if the gstreamer player API fails.
 */
TEST_F(RialtoServerMediaPipelineTextTrackIdentifierTest, GetTextTrackIdentifierFailure)
{
    std::string textTrackIdentifier;
    loadGstPlayer();

    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, getTextTrackIdentifier(_)).WillOnce(Return(false));
    EXPECT_FALSE(m_mediaPipeline->getTextTrackIdentifier(textTrackIdentifier));
}

/**
 * Test that GetTextTrackIdentifier fails if load has not been called (no gstreamer player).
 */
TEST_F(RialtoServerMediaPipelineTextTrackIdentifierTest, GetTextTrackIdentifierNoGstPlayerFailure)
{
    std::string textTrackIdentifier;
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_FALSE(m_mediaPipeline->getTextTrackIdentifier(textTrackIdentifier));
}
