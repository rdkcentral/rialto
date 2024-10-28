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
#include "MediaPipelineTest.h"
#include "MessageBuilders.h"

using testing::_;
using testing::Invoke;
using testing::Return;
using testing::StrEq;

namespace firebolt::rialto::server::ct
{
class VolumeTest : public MediaPipelineTest
{
public:
    VolumeTest()
    {
        GstElementFactory *elementFactory = gst_element_factory_find("fakesrc");
        m_audioSink = gst_element_factory_create(elementFactory, nullptr);
        gst_object_unref(elementFactory);
    }
    ~VolumeTest() override { gst_object_unref(m_audioSink); }

    void mockAudioSink()
    {
        EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, StrEq("audio-sink"), _))
            .WillOnce(Invoke(
                [&](gpointer object, const gchar *first_property_name, void *element)
                {
                    GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                    *elementPtr = m_audioSink;
                }));

        EXPECT_CALL(*m_glibWrapperMock, gTypeName(_)).WillRepeatedly(Return("GstStreamVolume"));
    }

    void willSetVolumeWhenVolumeDurationIsZero()
    {
        mockAudioSink();
        EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, StrEq("audio-fade"))).Times(0);
        EXPECT_CALL(*m_gstWrapperMock, gstStreamVolumeSetVolume(_, GST_STREAM_VOLUME_FORMAT_LINEAR, kVolume));
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_audioSink))
            .WillOnce(Invoke(this, &MediaPipelineTest::workerFinished));
    }

    void willSetVolumeWhenVolumeDurationMoreThanZero()
    {
        mockAudioSink();
        EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, StrEq("audio-fade"))).WillOnce(Return(nullptr));
        EXPECT_CALL(*m_rdkGstreamerUtilsWrapperMock, isSocAudioFadeSupported()).WillOnce(Return(true));

        firebolt::rialto::wrappers::rgu_Ease convertedEaseType =
            static_cast<firebolt::rialto::wrappers::rgu_Ease>(kEaseType);

        EXPECT_CALL(*m_rdkGstreamerUtilsWrapperMock, doAudioEasingonSoc(kVolume, kVolumeDuration, convertedEaseType));
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_audioSink))
            .WillOnce(Invoke(this, &MediaPipelineTest::workerFinished));
    }

    void willGetVolume()
    {
        mockAudioSink();

        EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, StrEq("fade-volume"))).WillOnce(Return(nullptr));

        EXPECT_CALL(*m_gstWrapperMock, gstStreamVolumeGetVolume(_, GST_STREAM_VOLUME_FORMAT_LINEAR))
            .WillOnce(Return(kVolume));

        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_audioSink))
            .WillOnce(Invoke(this, &MediaPipelineTest::workerFinished));
    }

    void setVolumeNormal()
    {
        ConfigureAction<SetVolume>(m_clientStub).send(createSetVolumeNormalRequest(m_sessionId)).expectSuccess();
        waitWorker();
    }

    void setVolumeWithFade()
    {
        ConfigureAction<SetVolume>(m_clientStub).send(createSetVolumeWithFadeRequest(m_sessionId)).expectSuccess();
        waitWorker();
    }

    void getVolume()
    {
        ConfigureAction<GetVolume>(m_clientStub)
            .send(createGetVolumeRequest(m_sessionId))
            .expectSuccess()
            .matchResponse([&](const auto &resp) { EXPECT_EQ(resp.volume(), kVolume); });
    }

private:
    GstElement *m_audioSink{nullptr};
    GParamSpec m_paramSpec{};
};

/*
 * Component Test: Volume Test
 * Test Objective:
 *  Test the Volume API. Set/Get Volume API calls should be supported by Rialto Server.
 *
 * Sequence Diagrams:
 *  Volume
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
 *  Step 5: Set Volume with no fade
 *   Set the volume when the volume duration is zero
 *   Send SetVolumeReq and expect successful response
 *
 *  Step 6: Set volume with fade
 *   Set the volume when the volume duration is more than zero
 *   Send SetVolumeReq and expect successful response
 *
 *  Step 7: Get Volume
 *   Send GetVolumeReq and expect successful response and current volume level
 *
 *  Step 8: Remove sources
 *   Remove the audio source.
 *   Expect that audio source is removed.
 *   Remove the video source.
 *   Expect that video source is removed.
 *
 *  Step 9: Stop
 *   Stop the playback.
 *   Expect that stop propagated to the gstreamer pipeline.
 *   Expect that server notifies the client that the Playback state has changed to STOPPED.
 *
 *  Step 10: Destroy media session
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
TEST_F(VolumeTest, Volume)
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

    // Step 5: Set Volume with no fade
    willSetVolumeWhenVolumeDurationIsZero();
    setVolumeNormal();

    // Step 6: Set volume with fade
    willSetVolumeWhenVolumeDurationMoreThanZero();
    setVolumeWithFade();

    // Step 7: Get Volume
    willGetVolume();
    getVolume();

    // Step 8: Remove sources
    willRemoveAudioSource();
    removeSource(m_audioSourceId);
    removeSource(m_videoSourceId);

    // Step 9: Stop
    willStop();
    stop();

    // Step 10: Destroy media session
    gstPlayerWillBeDestructed();
    destroySession();
}
} // namespace firebolt::rialto::server::ct
