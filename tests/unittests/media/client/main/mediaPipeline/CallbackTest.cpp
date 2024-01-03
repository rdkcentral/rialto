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
#include "MediaPipelineStructureMatchers.h"

class RialtoClientMediaPipelineCallbackTest : public MediaPipelineTestBase
{
protected:
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
 * Test a notification of the playback state is forwarded to the registered client.
 */
TEST_F(RialtoClientMediaPipelineCallbackTest, NotifyPlaybackState)
{
    PlaybackState state = PlaybackState::IDLE;
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyPlaybackState(state));

    m_mediaPipelineCallback->notifyPlaybackState(state);
}

/**
 * Test a notification of the network state is forwarded to the registered client.
 */
TEST_F(RialtoClientMediaPipelineCallbackTest, NotifyNetworkState)
{
    NetworkState state = NetworkState::IDLE;
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyNetworkState(state));

    m_mediaPipelineCallback->notifyNetworkState(state);
}

/**
 * Test a notification of qos is forwarded to the registered client.
 */
TEST_F(RialtoClientMediaPipelineCallbackTest, NotifyQos)
{
    QosInfo qosInfo{5u, 2u};
    int32_t sourceId = 1;

    EXPECT_CALL(*m_mediaPipelineClientMock, notifyQos(sourceId, qosInfoMatcher(qosInfo)));

    m_mediaPipelineCallback->notifyQos(sourceId, qosInfo);
}
