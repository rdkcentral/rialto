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
constexpr guint kVideoDecodeError{2};
constexpr guint kOccurrence{0};
constexpr gint64 kPtsNs{123456};
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
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_videoSink)).Times(1);
        EXPECT_CALL(*m_gstWrapperMock, gstStructureFree(&m_testStructure)).Times(1);
    }

    void willNotifyPlaybackMetric()
    {
        // Checked once by the dispatcher and once by the HandleBusMessage task
        EXPECT_CALL(*m_gstWrapperMock, gstStructureHasName(_, StrEq("media-pipeline-metric")))
            .Times(2)
            .WillRepeatedly(Return(true));
        EXPECT_CALL(*m_gstWrapperMock, gstElementClassGetMetadata(_, StrEq(GST_ELEMENT_METADATA_KLASS)))
            .WillOnce(Return("Sink/Video"));
        willGetStats();
    }

    void notifyPlaybackMetric()
    {
        ExpectMessage<PlaybackMetricEvent> expectedMetric{m_clientStub};

        m_gstreamerStub.sendPlaybackMetric(m_videoSink, kVideoDecodeError, kOccurrence, kPtsNs);

        auto receivedMetric = expectedMetric.getMessage();
        ASSERT_TRUE(receivedMetric);
        EXPECT_EQ(receivedMetric->session_id(), m_sessionId);
        EXPECT_EQ(receivedMetric->source_id(), m_videoSourceId);
        EXPECT_EQ(receivedMetric->type(), PlaybackMetricEvent_MetricType_VIDEO_DECODE_ERROR);
        EXPECT_EQ(receivedMetric->phase(), PlaybackMetricEvent_MetricPhase_OCCURRENCE);
        EXPECT_EQ(receivedMetric->pts_ns(), kPtsNs);
        EXPECT_EQ(receivedMetric->count(), 1u);
        EXPECT_EQ(receivedMetric->rendered_frames(), kRenderedFrames);
        EXPECT_EQ(receivedMetric->dropped_frames(), kDroppedFrames);
        EXPECT_EQ(receivedMetric->corrupted_frames(), 1u);
    }

    void getFrameCounters()
    {
        auto req{createGetFrameCountersRequest(m_sessionId, m_videoSourceId)};
        ConfigureAction<GetFrameCounters>(m_clientStub)
            .send(req)
            .expectSuccess()
            .matchResponse(
                [&](const auto &resp)
                {
                    EXPECT_EQ(resp.rendered_frames(), kRenderedFrames);
                    EXPECT_EQ(resp.dropped_frames(), kDroppedFrames);
                    EXPECT_EQ(resp.corrupted_frames(), 1u);
                });
    }

    void getFrameCountersOfAudioSourceFails()
    {
        auto req{createGetFrameCountersRequest(m_sessionId, m_audioSourceId)};
        ConfigureAction<GetFrameCounters>(m_clientStub).send(req).expectFailure();
    }

private:
    GstElement *m_videoSink{nullptr};
    GstStructure m_testStructure;
};

/*
 * Component Test: Playback metrics
 * Test Objective:
 *  Test that a playback metric posted by the video sink is sent to the client with the frame counters,
 *  and that the frame counters of the video source can be queried.
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
 *  Step 9: Playback metric
 *   The video sink posts a video decode error occurrence.
 *   Expect that server sends PlaybackMetricEvent with the frame counters and one corrupted frame.
 *  Step 10: Get frame counters
 *   Expect that GetFrameCounters returns the sink stats and one corrupted frame.
 *   Expect that GetFrameCounters fails for the audio source.
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

    // Step 9: Playback metric
    willNotifyPlaybackMetric();
    notifyPlaybackMetric();

    // Step 10: Get frame counters
    willGetStats();
    getFrameCounters();
    getFrameCountersOfAudioSourceFails();

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
