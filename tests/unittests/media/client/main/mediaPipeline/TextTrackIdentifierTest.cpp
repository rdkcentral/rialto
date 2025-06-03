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

class RialtoClientMediaPipelineTextTrackIdentifierTest : public MediaPipelineTestBase
{
protected:
    const std::string m_kTextTrackIdentifier{"Identifier"};

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
 * Test that getTextTrackIdentifier returns success if the IPC API succeeds.
 */
TEST_F(RialtoClientMediaPipelineTextTrackIdentifierTest, getTextTrackIdentifierSuccess)
{
    std::string textTrackIdentifier;
    EXPECT_CALL(*m_mediaPipelineIpcMock, getTextTrackIdentifier(_))
        .WillOnce(DoAll(SetArgReferee<0>(m_kTextTrackIdentifier), Return(true)));

    EXPECT_TRUE(m_mediaPipeline->getTextTrackIdentifier(textTrackIdentifier));
    EXPECT_EQ(textTrackIdentifier, m_kTextTrackIdentifier);
}

/**
 * Test that getTextTrackIdentifier returns failure if the IPC API fails.
 */
TEST_F(RialtoClientMediaPipelineTextTrackIdentifierTest, getTextTrackIdentifierFailure)
{
    std::string textTrackIdentifier;
    EXPECT_CALL(*m_mediaPipelineIpcMock, getTextTrackIdentifier(_)).WillOnce(Return(false));

    EXPECT_FALSE(m_mediaPipeline->getTextTrackIdentifier(textTrackIdentifier));
}

/**
 * Test that setTextTrackIdentifier returns success if the IPC API succeeds.
 */
TEST_F(RialtoClientMediaPipelineTextTrackIdentifierTest, setTextTrackIdentifierSuccess)
{
    EXPECT_CALL(*m_mediaPipelineIpcMock, setTextTrackIdentifier(m_kTextTrackIdentifier)).WillOnce(Return(true));

    EXPECT_TRUE(m_mediaPipeline->setTextTrackIdentifier(m_kTextTrackIdentifier));
}

/**
 * Test that setTextTrackIdentifier returns failure if the IPC API fails.
 */
TEST_F(RialtoClientMediaPipelineTextTrackIdentifierTest, setTextTrackIdentifierFailure)
{
    EXPECT_CALL(*m_mediaPipelineIpcMock, setTextTrackIdentifier(m_kTextTrackIdentifier)).WillOnce(Return(false));

    EXPECT_FALSE(m_mediaPipeline->setTextTrackIdentifier(m_kTextTrackIdentifier));
}
