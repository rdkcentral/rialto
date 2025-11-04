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
class SetVideoWindowTest : public MediaPipelineTest
{
public:
    SetVideoWindowTest()
    {
        GstElementFactory *elementFactory = gst_element_factory_find("fakesrc");
        m_videoSink = gst_element_factory_create(elementFactory, nullptr);
        gst_object_unref(elementFactory);
    }

    ~SetVideoWindowTest() override { gst_object_unref(m_videoSink); }

    void willSetVideoWindow()
    {
        EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, StrEq("video-sink"), _))
            .WillOnce(Invoke(
                [&](gpointer object, const gchar *first_property_name, void *element)
                {
                    GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                    *elementPtr = m_videoSink;
                }));
        EXPECT_CALL(*m_glibWrapperMock, gTypeName(G_OBJECT_TYPE(m_videoSink))).WillOnce(Return(kVideoSinkTypeName.c_str()));
        EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, StrEq("rectangle"))).WillOnce(Return(&m_paramSpec));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(m_videoSink, StrEq("rectangle")));
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_videoSink));
    }

    void setVideoWindow()
    {
        ConfigureAction<SetVideoWindow>(m_clientStub).send(createSetVideoWindowRequest(m_sessionId)).expectSuccess();
    }

private:
    GstElement *m_videoSink{nullptr};
    GParamSpec m_paramSpec{};
};

/*
 * Component Test: SetVideoWindow Test
 * Test Objective:
 *  Test the SetVideoWindow API. SetVideoWindow API calls should be supported by Rialto Server.
 *
 * Sequence Diagrams:
 *  Set Video Window Size & Position
 *   - https://wiki.rdkcentral.com/display/ASP/Rialto+MSE+Misc+Sequence+Diagrams
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
 *  Step 5: Set SetVideoWindow
 *   Send SetVideoWindowReq and expect successful response
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
TEST_F(SetVideoWindowTest, SetVideoWindow)
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

    // Step 5: Set Video Window
    willSetVideoWindow();
    setVideoWindow();

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
