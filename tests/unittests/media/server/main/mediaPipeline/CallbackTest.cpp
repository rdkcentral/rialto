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

MATCHER_P(QosInfoMatcher, expectedQosInfo, "")
{
    return ((expectedQosInfo.processed == arg.processed) && (expectedQosInfo.dropped == arg.dropped));
}

class RialtoServerMediaPipelineCallbackTest : public MediaPipelineTestBase
{
protected:
    RialtoServerMediaPipelineCallbackTest()
    {
        createMediaPipeline();

        loadGstPlayer();
    }

    ~RialtoServerMediaPipelineCallbackTest() { destroyMediaPipeline(); }

    void setPlaybackStatePlaying()
    {
        mainThreadWillEnqueueTask();
        EXPECT_CALL(*m_mediaPipelineClientMock, notifyPlaybackState(PlaybackState::PLAYING));

        m_gstPlayerCallback->notifyPlaybackState(PlaybackState::PLAYING);
    }
};

/**
 * Test a notification of the playback state is forwarded to the registered client.
 */
TEST_F(RialtoServerMediaPipelineCallbackTest, NotifyPlaybackState)
{
    PlaybackState state = PlaybackState::IDLE;
    mainThreadWillEnqueueTask();
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyPlaybackState(state));

    m_gstPlayerCallback->notifyPlaybackState(state);
}

/**
 * Test a notification of the position is forwarded to the registered client.
 */
TEST_F(RialtoServerMediaPipelineCallbackTest, notifyPosition)
{
    int64_t position{12345};
    mainThreadWillEnqueueTask();
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyPosition(position));

    m_gstPlayerCallback->notifyPosition(position);
}

/**
 * Test a notification of the network state is forwarded to the registered client.
 */
TEST_F(RialtoServerMediaPipelineCallbackTest, notifyNetworkState)
{
    auto state{firebolt::rialto::NetworkState::BUFFERING};
    mainThreadWillEnqueueTask();
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyNetworkState(state));

    m_gstPlayerCallback->notifyNetworkState(state);
}

/**
 * Test a notification of the need media data is forwarded to the registered client in prerolling state.
 */
TEST_F(RialtoServerMediaPipelineCallbackTest, notifyNeedMediaDataInPrerollingState)
{
    auto mediaSourceType = firebolt::rialto::MediaSourceType::VIDEO;
    int sourceId = attachSource(mediaSourceType, "video/h264");
    constexpr int kNumFrames{24};

    expectNotifyNeedData(mediaSourceType, sourceId, kNumFrames);

    m_gstPlayerCallback->notifyNeedMediaData(mediaSourceType);
}

/**
 * Test a notification of the need media data is forwarded to the registered client in playing state.
 */
TEST_F(RialtoServerMediaPipelineCallbackTest, notifyNeedMediaDataInPlayingState)
{
    auto mediaSourceType = firebolt::rialto::MediaSourceType::VIDEO;
    int sourceId = attachSource(mediaSourceType, "video/h264");
    int numFrames{24};

    setPlaybackStatePlaying();

    expectNotifyNeedData(mediaSourceType, sourceId, numFrames);

    m_gstPlayerCallback->notifyNeedMediaData(mediaSourceType);
}

/**
 * Test a notification of the need media data is not forwarded when source id is not present
 */
TEST_F(RialtoServerMediaPipelineCallbackTest, notifyNeedMediaDataFailureDueToSourceIdNotPresent)
{
    auto mediaSourceType = firebolt::rialto::MediaSourceType::VIDEO;
    mainThreadWillEnqueueTaskAndWait();
    ASSERT_TRUE(m_sharedMemoryBufferMock);
    ASSERT_TRUE(m_activeRequestsMock);
    EXPECT_CALL(*m_sharedMemoryBufferMock,
                clearData(ISharedMemoryBuffer::MediaPlaybackType::GENERIC, m_kSessionId, mediaSourceType))
        .WillOnce(Return(true));

    m_gstPlayerCallback->notifyNeedMediaData(mediaSourceType);
}

/**
 * Test a notification of the need media data for audio is ignored if EOS.
 */
TEST_F(RialtoServerMediaPipelineCallbackTest, notifyNeedMediaDataAudioInEos)
{
    auto mediaSourceType = firebolt::rialto::MediaSourceType::AUDIO;
    attachSource(mediaSourceType, "audio/x-opus");

    setPlaybackStatePlaying();
    setEos(mediaSourceType);

    expectNotifyNeedDataEos(mediaSourceType);

    m_gstPlayerCallback->notifyNeedMediaData(mediaSourceType);
}

/**
 * Test a notification of the need media data for video is ignored if EOS.
 */
TEST_F(RialtoServerMediaPipelineCallbackTest, notifyNeedMediaDataVideoInEos)
{
    auto mediaSourceType = firebolt::rialto::MediaSourceType::VIDEO;
    attachSource(mediaSourceType, "video/h264");

    setPlaybackStatePlaying();
    setEos(mediaSourceType);

    expectNotifyNeedDataEos(mediaSourceType);

    m_gstPlayerCallback->notifyNeedMediaData(mediaSourceType);
}

/**
 * Test a notification of the need media data is sent if another source is Eos.
 */
TEST_F(RialtoServerMediaPipelineCallbackTest, notifyNeedMediaDataOtherSourcesInEos)
{
    int videoSourceId = attachSource(firebolt::rialto::MediaSourceType::VIDEO, "video/h264");
    attachSource(firebolt::rialto::MediaSourceType::AUDIO, "audio/x-opus");

    int numFrames{24};

    setPlaybackStatePlaying();
    setEos(firebolt::rialto::MediaSourceType::AUDIO);

    expectNotifyNeedData(firebolt::rialto::MediaSourceType::VIDEO, videoSourceId, numFrames);

    m_gstPlayerCallback->notifyNeedMediaData(firebolt::rialto::MediaSourceType::VIDEO);
}

/**
 * Test a notification of qos is forwarded to the registered client.
 */
TEST_F(RialtoServerMediaPipelineCallbackTest, notifyQos)
{
    auto mediaSourceType = firebolt::rialto::MediaSourceType::VIDEO;
    int sourceId = attachSource(mediaSourceType, "audio/x-opus");
    QosInfo qosInfo{5u, 2u};

    mainThreadWillEnqueueTask();
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyQos(sourceId, QosInfoMatcher(qosInfo)));

    m_gstPlayerCallback->notifyQos(mediaSourceType, qosInfo);
}

/**
 * Test a notification of qos fails when sourceid cannot be found.
 */
TEST_F(RialtoServerMediaPipelineCallbackTest, notifyQosFailureSourceIdNotFound)
{
    auto mediaSourceType = firebolt::rialto::MediaSourceType::VIDEO;
    QosInfo qosInfo{5u, 2u};

    mainThreadWillEnqueueTask();

    m_gstPlayerCallback->notifyQos(mediaSourceType, qosInfo);
}

/**
 * Test a notification of playback error is forwarded to the registered client.
 */
TEST_F(RialtoServerMediaPipelineCallbackTest, notifyPlaybackError)
{
    auto mediaSourceType = firebolt::rialto::MediaSourceType::VIDEO;
    auto error = firebolt::rialto::PlaybackError::DECRYPTION;
    int sourceId = attachSource(mediaSourceType, "video/mp4");

    mainThreadWillEnqueueTask();
    EXPECT_CALL(*m_mediaPipelineClientMock, notifyPlaybackError(sourceId, error));

    m_gstPlayerCallback->notifyPlaybackError(mediaSourceType, error);
}

/**
 * Test a notification of playback error fails when sourceid cannot be found.
 */
TEST_F(RialtoServerMediaPipelineCallbackTest, notifyPlaybackErrorFailureSourceIdNotFound)
{
    auto mediaSourceType = firebolt::rialto::MediaSourceType::VIDEO;
    auto error = firebolt::rialto::PlaybackError::DECRYPTION;

    mainThreadWillEnqueueTask();

    m_gstPlayerCallback->notifyPlaybackError(mediaSourceType, error);
}

/**
 * Tests if active request cache is cleared.
 */
TEST_F(RialtoServerMediaPipelineCallbackTest, clearActiveRequestsCache)
{
    mainThreadWillEnqueueTask();
    EXPECT_CALL(*m_activeRequestsMock, clear());

    m_gstPlayerCallback->clearActiveRequestsCache();
}

/**
 * Test a notification of source flushed is forwarded to the registered client.
 */
TEST_F(RialtoServerMediaPipelineCallbackTest, notifySourceFlushed)
{
    auto mediaSourceType = firebolt::rialto::MediaSourceType::VIDEO;
    int sourceId = attachSource(mediaSourceType, "video/mp4");

    mainThreadWillEnqueueTask();
    EXPECT_CALL(*m_mediaPipelineClientMock, notifySourceFlushed(sourceId));

    m_gstPlayerCallback->notifySourceFlushed(mediaSourceType);
}

/**
 * Test a notification of source flushed fails when sourceid cannot be found.
 */
TEST_F(RialtoServerMediaPipelineCallbackTest, notifySourceFlushedFailureSourceIdNotFound)
{
    auto mediaSourceType = firebolt::rialto::MediaSourceType::VIDEO;

    mainThreadWillEnqueueTask();

    m_gstPlayerCallback->notifySourceFlushed(mediaSourceType);
}
