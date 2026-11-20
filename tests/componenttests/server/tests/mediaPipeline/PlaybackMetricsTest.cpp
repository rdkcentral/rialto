/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Sky UK
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

#include "ActionTraits.h"
#include "ConfigureAction.h"
#include "Constants.h"
#include "ExpectMessage.h"
#include "MediaPipelineTest.h"
#include "MessageBuilders.h"

using testing::_;
using testing::DoAll;
using testing::Invoke;
using testing::Return;
using testing::SetArgumentPointee;
using testing::StrEq;

namespace
{
constexpr unsigned kFramesToPush{1};
constexpr guint kVideoDecodeError{3};
constexpr guint kOccurrence{1};
constexpr gint64 kPtsNs{123456};
constexpr guint64 kCorruptedFrames{4};
const std::string kElementTypeName{"GenericSink"};
} // namespace

namespace firebolt::rialto::server::ct
{
class PlaybackMetricsTest : public MediaPipelineTest
{
public:
    PlaybackMetricsTest()
    {
        GstElementFactory *elementFactory = gst_element_factory_find("fakesrc");
        m_videoSink = gst_element_factory_create(elementFactory, nullptr);
        gst_object_unref(elementFactory);
    }

    ~PlaybackMetricsTest() override { gst_object_unref(m_videoSink); }

    void willGetStats()
    {
        EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(&m_pipeline, StrEq("video-sink"), _))
            .WillOnce(Invoke(
                [&](gpointer object, const gchar *first_property_name, void *element)
                {
                    GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                    *elementPtr = m_videoSink;
                }));
        EXPECT_CALL(*m_glibWrapperMock, gTypeName(G_OBJECT_TYPE(m_videoSink))).WillOnce(Return(kElementTypeName.c_str()));
        EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, StrEq("stats"), _))
            .WillOnce(Invoke(
                [&](gpointer object, const gchar *first_property_name, void *element)
                {
                    GstStructure **elementPtr = reinterpret_cast<GstStructure **>(element);
                    *elementPtr = &m_testStructure;
                }));
        EXPECT_CALL(*m_gstWrapperMock, gstStructureGetUint64(&m_testStructure, StrEq("rendered"), _))
            .WillOnce(DoAll(SetArgumentPointee<2>(kRenderedFrames), Return(true)));
        EXPECT_CALL(*m_gstWrapperMock, gstStructureGetUint64(&m_testStructure, StrEq("dropped"), _))
            .WillOnce(DoAll(SetArgumentPointee<2>(kDroppedFrames), Return(true)));
        EXPECT_CALL(*m_gstWrapperMock, gstStructureGetUint64(&m_testStructure, StrEq("corrupted"), _))
            .WillOnce(DoAll(SetArgumentPointee<2>(kCorruptedFrames), Return(true)));
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_videoSink)).Times(1);
        EXPECT_CALL(*m_gstWrapperMock, gstStructureFree(&m_testStructure)).Times(1);
    }

    void setMetricSubscription()
    {
        auto req{createSetMetricSubscriptionRequest(m_sessionId, {MediaMetricBatchEvent_MetricType_VIDEO_DECODE_ERROR})};
        ConfigureAction<SetMetricSubscription>(m_clientStub).send(req).expectSuccess();
    }

    void willNotifyMediaMetric()
    {
        // Checked once by the dispatcher sync handler and once by the HandleBusMessage task
        EXPECT_CALL(*m_gstWrapperMock, gstStructureHasName(_, StrEq("media-pipeline-metric")))
            .Times(2)
            .WillRepeatedly(Return(true));
    }

    void notifyMediaMetric()
    {
        ExpectMessage<MediaMetricBatchEvent> expectedMetric{m_clientStub};

        m_gstreamerStub.sendMediaMetric(m_videoSink,
                                        gst_structure_new("media-pipeline-metric", "metric", G_TYPE_UINT,
                                                          kVideoDecodeError, "observation-kind", G_TYPE_UINT,
                                                          kOccurrence, "pts-ns", G_TYPE_INT64, kPtsNs, nullptr));

        auto receivedMetric = expectedMetric.getMessage();
        ASSERT_TRUE(receivedMetric);
        EXPECT_EQ(receivedMetric->session_id(), m_sessionId);
        EXPECT_EQ(receivedMetric->missed_count(), 0u);
        ASSERT_EQ(receivedMetric->metrics_size(), 1);
        const auto &kEntry = receivedMetric->metrics(0);
        EXPECT_EQ(kEntry.source_id(), m_videoSourceId);
        EXPECT_EQ(kEntry.type(), MediaMetricBatchEvent_MetricType_VIDEO_DECODE_ERROR);
        EXPECT_EQ(kEntry.phase(), MediaMetricBatchEvent_MetricPhase_OCCURRENCE);
        EXPECT_EQ(kEntry.pts_ns(), kPtsNs);
        EXPECT_FALSE(kEntry.has_count());
        EXPECT_FALSE(kEntry.has_duration_ns());
        EXPECT_GT(kEntry.observed_at_monotonic_ns(), 0u);
    }

    void getFrameCounters()
    {
        auto req{createGetStatsRequest(m_sessionId, m_videoSourceId)};
        ConfigureAction<GetStats>(m_clientStub)
            .send(req)
            .expectSuccess()
            .matchResponse(
                [&](const auto &resp)
                {
                    EXPECT_EQ(resp.rendered_frames(), kRenderedFrames);
                    EXPECT_EQ(resp.dropped_frames(), kDroppedFrames);
                    EXPECT_EQ(resp.corrupted_frames(), kCorruptedFrames);
                });
    }

private:
    GstElement *m_videoSink{nullptr};
    GstStructure m_testStructure;
};

/*
 * Component Test: Playback metrics
 * Test Objective:
 *  Test that a subscribed media metric posted on the bus is sent to the client in a MediaMetricBatchEvent,
 *  and that the three frame counters of the video source can be queried.
 *
 * Sequence Diagrams:
 *  Playback metrics - docs/PlaybackMetrics.md
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: MediaPipeline
 *
 * Test Initialize:
 *  Create memory region for the shared buffer.
 *  Create a server that handles Control IPC requests.
 *  Initalise the control state to running for this test application.
 *
 * Test Steps:
 *  Step 1: Create a new media session
 *  Step 2: Load content
 *  Step 3: Attach all sources
 *  Step 4: Pause
 *  Step 5: Write 1 audio frame
 *  Step 6: Write 1 video frame
 *  Step 7: Notify buffered and Paused
 *  Step 8: Play
 *  Step 9: Media metric
 *   Subscribe to video decode errors. The decoder posts a video decode error occurrence.
 *   Expect that server sends a MediaMetricBatchEvent with one occurrence for the video source.
 *  Step 10: Get frame counters
 *   Expect that getStats returns rendered, dropped and corrupted from the sink stats.
 *  Step 11: End of stream, remove sources, stop and destroy the media session
 *
 * Test Teardown:
 *  Server is terminated.
 *
 * Expected Results:
 *  The playback metric and frame counters are delivered to the client.
 *
 * Code:
 */
TEST_F(PlaybackMetricsTest, PlaybackMetrics)
{
    // Step 1: Create a new media session
    createSession();

    // Step 2: Load content
    gstPlayerWillBeCreated();
    load();

    // Step 3: Attach all sources
    audioSourceWillBeAttached();
    attachAudioSource();
    videoSourceWillBeAttached();
    attachVideoSource();
    sourceWillBeSetup();
    setupSource();
    willSetupAndAddSource(&m_audioAppSrc);
    willSetupAndAddSource(&m_videoAppSrc);
    willFinishSetupAndAddSource();
    indicateAllSourcesAttached({&m_audioAppSrc, &m_videoAppSrc});

    // Step 4: Pause
    willPause();
    pause();

    // Step 5: Write 1 audio frame
    // Step 6: Write 1 video frame
    // Step 7: Notify buffered and Paused
    {
        ExpectMessage<firebolt::rialto::NetworkStateChangeEvent> expectedNetworkStateChange{m_clientStub};

        pushAudioData(kFramesToPush);
        pushVideoData(kFramesToPush);

        auto receivedNetworkStateChange{expectedNetworkStateChange.getMessage()};
        ASSERT_TRUE(receivedNetworkStateChange);
        EXPECT_EQ(receivedNetworkStateChange->session_id(), m_sessionId);
        EXPECT_EQ(receivedNetworkStateChange->state(), ::firebolt::rialto::NetworkStateChangeEvent_NetworkState_BUFFERED);
    }
    willNotifyPaused();
    notifyPaused();

    // Step 8: Play
    willPlay();
    play();

    // Step 9: Media metric
    setMetricSubscription();
    willNotifyMediaMetric();
    notifyMediaMetric();

    // Step 10: Get frame counters
    willGetStats();
    getFrameCounters();

    // Step 11: End of stream, remove sources, stop and destroy the media session
    willEos(&m_audioAppSrc);
    eosAudio(kFramesToPush);
    willEos(&m_videoAppSrc);
    eosVideo(kFramesToPush);
    gstNotifyEos();
    removeSource(m_audioSourceId);
    removeSource(m_videoSourceId);
    willStop();
    stop();
    gstPlayerWillBeDestructed();
    destroySession();
}
} // namespace firebolt::rialto::server::ct
