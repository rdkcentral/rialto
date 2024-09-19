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
#include "Matchers.h"
#include "MediaPipelineTest.h"
#include "MessageBuilders.h"
#include <gst/gst.h>

using testing::_;
using testing::Invoke;
using testing::Return;
using testing::StrEq;

namespace
{
const std::string kVideoSinkTypeName{"VideoSink"};
} // namespace

namespace firebolt::rialto::server::ct
{
class RenderFrameTest : public MediaPipelineTest
{
public:
    RenderFrameTest()
    {
        GstElementFactory *elementFactory = gst_element_factory_find("fakesrc");
        m_videoSink = gst_element_factory_create(elementFactory, nullptr);
        gst_object_unref(elementFactory);
    }

    ~RenderFrameTest() override { gst_object_unref(m_videoSink); }

    void willRenderFrame()
    {
        EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(&m_pipeline, StrEq("video-sink"), _))
            .WillOnce(Invoke(
                [&](gpointer, const gchar *, void *element)
                {
                    GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                    *elementPtr = m_videoSink;
                }));
        EXPECT_CALL(*m_glibWrapperMock, gTypeName(_)).WillOnce(Return(kVideoSinkTypeName.c_str()));
        EXPECT_CALL(*m_glibWrapperMock,
                    gObjectClassFindProperty(G_OBJECT_GET_CLASS(m_videoSink), StrEq("frame-step-on-preroll")))
            .WillOnce(Return(&m_paramSpec));

        EXPECT_CALL(*m_glibWrapperMock, gObjectSetIntStub(_, StrEq("frame-step-on-preroll"), 1)).Times(1);
        EXPECT_CALL(*m_gstWrapperMock, gstEventNewStep(GST_FORMAT_BUFFERS, 1, 1.0, true, false))
            .WillOnce(Return(&m_newStepEvent));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetIntStub(_, StrEq("frame-step-on-preroll"), 0)).Times(1);
        EXPECT_CALL(*m_gstWrapperMock, gstElementSendEvent(_, &m_newStepEvent));
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(GST_OBJECT(m_videoSink)));
    }

    void renderFrame()
    {
        auto req{createRenderFrameRequest(m_sessionId)};
        ConfigureAction<RenderFrame>(m_clientStub).send(req).expectSuccess();
    }

    void renderFrameFailure()
    {
        auto req{createRenderFrameRequest(m_sessionId + 1)};
        ConfigureAction<RenderFrame>(m_clientStub).send(req).expectFailure();
    }

private:
    GstElement *m_videoSink{nullptr};
    GParamSpec m_paramSpec{};
    GstEvent m_newStepEvent{};
};

/*
 * Component Test: Render Frame Sequence
 * Test Objective:
 *  Test the RenderFrame API.
 *
 * Sequence Diagrams:
 *  Render Frame
 *   - https://wiki.rdkcentral.com/display/ASP/Rialto+Playback+Design
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: MediaPipeline
 *
 * Test Initialize:
 *  Set Rialto Server to Active
 *  Connect Rialto Client Stub
 *  Map Shared Memory
 *
 * Test Steps:
 *  Step 1: Create a new media session
 *   Send CreateSessionRequest to Rialto Server
 *   Expect that successful CreateSessionResponse is received
 *   Save returned session id
 *
 *  Step 2: Load content
 *   Send LoadRequest to Rialto Server
 *   Expect that successful LoadResponse is received
 *   Expect that GstPlayer instance is created.
 *   Expect that client is notified that the NetworkState has changed to BUFFERING.
 *
 *  Step 3: Attach all sources
 *   Attach the audio source.
 *   Expect that audio source is attached.
 *   Attach the video source.
 *   Expect that video source is attached.
 *   Expect that rialto source is setup
 *   Expect that all sources are attached.
 *   Expect that the Playback state has changed to IDLE.
 *
 *  Step 4: Pause
 *   Pause the content.
 *   Expect that gstreamer pipeline is paused.
 *
 *  Step 5: Render Frame
 *   Send RenderFrameReq and expect successful response
 *
 *  Step 6: Remove sources
 *   Remove the audio source.
 *   Expect that audio source is removed.
 *   Remove the video source.
 *   Expect that video source is removed.
 *
 *  Step 7: Stop
 *   Stop the playback.
 *   Expect that stop propagated to the gstreamer pipeline.
 *   Expect that server notifies the client that the Playback state has changed to STOPPED.
 *
 *  Step 8: Destroy media session
 *   Send DestroySessionRequest.
 *   Expect that the session is destroyed on the server.
 *
 * Test Teardown:
 *  Memory region created for the shared buffer is unmapped.
 *  Server is terminated.
 *
 * Expected Results:
 *  RenderFrame API call is successful
 *
 * Code:
 */
TEST_F(RenderFrameTest, RenderFrameSuccess)
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
    indicateAllSourcesAttached();

    // Step 4: Pause
    willPause();
    pause();

    // Step 5: Render Frame
    willRenderFrame();
    renderFrame();

    // Step 6: Remove sources
    willRemoveAudioSource();
    removeSource(m_audioSourceId);
    removeSource(m_videoSourceId);

    // Step 7: Stop
    willStop();
    stop();

    // Step 8: Destroy media session
    gstPlayerWillBeDestructed();
    destroySession();
}

/*
 * Component Test: Render Frame Sequence
 * Test Objective:
 *  Test the RenderFrame API.
 *
 * Sequence Diagrams:
 *  Render Frame
 *   - https://wiki.rdkcentral.com/display/ASP/Rialto+Playback+Design
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: MediaPipeline
 *
 * Test Initialize:
 *  Set Rialto Server to Active
 *  Connect Rialto Client Stub
 *  Map Shared Memory
 *
 * Test Steps:
 *  Step 1: Create a new media session
 *   Send CreateSessionRequest to Rialto Server
 *   Expect that successful CreateSessionResponse is received
 *   Save returned session id
 *
 *  Step 2: Load content
 *   Send LoadRequest to Rialto Server
 *   Expect that successful LoadResponse is received
 *   Expect that GstPlayer instance is created.
 *   Expect that client is notified that the NetworkState has changed to BUFFERING.
 *
 *  Step 3: Attach all sources
 *   Attach the audio source.
 *   Expect that audio source is attached.
 *   Attach the video source.
 *   Expect that video source is attached.
 *   Expect that rialto source is setup
 *   Expect that all sources are attached.
 *   Expect that the Playback state has changed to IDLE.
 *
 *  Step 4: Pause
 *   Pause the content.
 *   Expect that gstreamer pipeline is paused.
 *   Expect that server notifies the client that the Network state has changed to PAUSED.
 *
 *  Step 5: Render Frame Failure
 *   Send RenderFrameReq with wrong session id and expect failure response
 *
 *  Step 6: Remove sources
 *   Remove the audio source.
 *   Expect that audio source is removed.
 *   Remove the video source.
 *   Expect that video source is removed.
 *
 *  Step 7: Stop
 *   Stop the playback.
 *   Expect that stop propagated to the gstreamer pipeline.
 *   Expect that server notifies the client that the Playback state has changed to STOPPED.
 *
 *  Step 8: Destroy media session
 *   Send DestroySessionRequest.
 *   Expect that the session is destroyed on the server.
 *
 * Test Teardown:
 *  Memory region created for the shared buffer is unmapped.
 *  Server is terminated.
 *
 * Expected Results:
 *  RenderFrame API call is successful
 *
 * Code:
 */
TEST_F(RenderFrameTest, renderFrameFailure)
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
    indicateAllSourcesAttached();

    // Step 4: Pause
    willPause();
    pause();

    // Step 5: Render Frame Failure
    renderFrameFailure();

    // Step 6: Remove sources
    willRemoveAudioSource();
    removeSource(m_audioSourceId);
    removeSource(m_videoSourceId);

    // Step 7: Stop
    willStop();
    stop();

    // Step 8: Destroy media session
    gstPlayerWillBeDestructed();
    destroySession();
}
} // namespace firebolt::rialto::server::ct
