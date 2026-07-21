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

#include "Constants.h"
#include "ExpectMessage.h"
#include "MediaPipelineProtoUtils.h"
#include "MediaPipelineTest.h"

using testing::_;
using testing::DoAll;
using testing::Return;
using testing::SetArgPointee;
using testing::StrEq;

namespace
{
constexpr unsigned kFramesToPush{1};
const std::string kAudioDecryptorName{"rialtodecryptoraudio_0"};
const std::string kVideoDecryptorName{"rialtodecryptorvideo_0"};
} // namespace

MATCHER_P2(gstWarningMatcher, error, debug, "")
{
    GstMessage *message = GST_MESSAGE_CAST(arg);
    return (message->type == GST_MESSAGE_WARNING);
}

namespace firebolt::rialto::server::ct
{
class NonFatalPlayerErrorUpdatesTest : public MediaPipelineTest
{
public:
    NonFatalPlayerErrorUpdatesTest() = default;
    ~NonFatalPlayerErrorUpdatesTest() override = default;

    void willHandleWarningMessage(GQuark domain, gint code)
    {
        m_err.domain = domain;
        m_err.code = code;
        m_err.message = m_debug;

        EXPECT_CALL(*m_gstWrapperMock, gstMessageParseWarning(gstWarningMatcher(m_err, m_debug), _, _))
            .WillOnce(DoAll(SetArgPointee<1>(&m_err), SetArgPointee<2>(m_debug)));
        EXPECT_CALL(*m_glibWrapperMock, gFree(m_debug));
        EXPECT_CALL(*m_glibWrapperMock, gErrorFree(&m_err)).WillOnce(Invoke(this, &MediaPipelineTest::workerFinished));
    }

    void gstWarning(const char *srcName)
    {
        m_src = GST_OBJECT_CAST(g_object_new(GST_TYPE_BIN, nullptr));
        gst_object_set_name(m_src, srcName);
        m_gstreamerStub.sendWarning(GST_ELEMENT(m_src), &m_err, m_debug);

        // Warning executed on the bus thread, wait for it to complete
        waitWorker();

        g_object_unref(m_src);
    }

    void notifyPlaybackError(int sourceId, PlaybackError error, const char *srcName)
    {
        ExpectMessage<PlaybackErrorEvent> expectedPlaybackError{m_clientStub};

        gstWarning(srcName);

        auto receivedPlaybackError = expectedPlaybackError.getMessage();
        ASSERT_TRUE(receivedPlaybackError);
        EXPECT_EQ(receivedPlaybackError->session_id(), m_sessionId);
        EXPECT_EQ(receivedPlaybackError->source_id(), sourceId);
        EXPECT_EQ(receivedPlaybackError->error(), convertPlaybackError(error));
    }

private:
    GstObject *m_src{nullptr};
    GError m_err{};
    gchar m_debug[14]{"Error message"};
};

/*
 * Component Test: Non-fatal Player Error Updates Test
 * Test Objective:
 *  Test that after non-fatal errors on the gstreamer pipeline via GST_MESSAGE_WARNING are handled succcesfully.
 *
 * Sequence Diagrams:
 *  Non-fatal Errors - https://wiki.rdkcentral.com/display/ASP/Rialto+Playback+Design
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: MediaPipelineClient
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
 *  Step 5: Write 1 audio frame
 *   Gstreamer Stub notifies, that it needs audio data
 *   Expect that server notifies the client that it needs 3 frames of audio data.
 *   Write 1 frame of audio data to the shared buffer.
 *   Send HaveData message
 *   Expect that server notifies the client that it needs 3 frames of audio data.
 *
 *  Step 6: Write 1 video frame
 *   Gstreamer Stub notifies, that it needs video data
 *   Expect that server notifies the client that it needs 3 frames of video data.
 *   Write 1 frame of video data to the shared buffer.
 *   Send HaveData message
 *   Expect that server notifies the client that it needs 3 frames of video data.
 *
 *  Step 7: Notify buffered and Paused
 *   Expect that server notifies the client that the Network state has changed to BUFFERED.
 *   Gstreamer Stub notifies, that pipeline state is in PAUSED state
 *   Expect that server notifies the client that the Network state has changed to PAUSED.
 *
 *  Step 8: Play
 *   Play the content.
 *   Expect that gstreamer pipeline is in playing state
 *   Expect that server notifies the client that the Playback state has changed to PLAYING.
 *
 *  Step 9: Audio generic warning
 *   GST_MESSAGE_WARNING should be received from audio source.
 *   Error is set to GST_CORE_ERROR.
 *   Expect no notification to the client.
 *
 *  Step 10: Video generic warning
 *   GST_MESSAGE_WARNING should be received from video source.
 *   Error is set to GST_CORE_ERROR.
 *   Expect no notification to the client.
 *
 *  Step 11: Audio decrypt warning
 *   GST_MESSAGE_WARNING should be received from audio source.
 *   Error is set to GST_STREAM_ERROR & GST_STREAM_ERROR_DECRYPT.
 *   Expect that server notifies the client of an audio PlaybackError::DECRYPTION.
 *
 *  Step 12: Video decrypt warning
 *   GST_MESSAGE_WARNING should be received from video source.
 *   Error is set to GST_STREAM_ERROR & GST_STREAM_ERROR_DECRYPT.
 *   Expect that server notifies the client of an video PlaybackError::DECRYPTION.
 *
 *  Step 13: End of audio stream
 *   Send audio haveData with one frame and EOS status
 *   Expect that Gstreamer is notified about end of stream
 *
 *  Step 14: End of video stream
 *   Send video haveData with one frame and EOS status
 *   Expect that Gstreamer is notified about end of stream
 *
 *  Step 15: Notify end of stream
 *   Simulate, that gst_message_eos is received by Rialto Server
 *   Expect that server notifies the client that the Network state has changed to END_OF_STREAM.
 *
 *  Step 16: Remove sources
 *   Remove the audio source.
 *   Expect that audio source is removed.
 *   Remove the video source.
 *   Expect that video source is removed.
 *
 *  Step 17: Stop
 *   Stop the playback.
 *   Expect that stop propagated to the gstreamer pipeline.
 *   Expect that server notifies the client that the Playback state has changed to STOPPED.
 *
 *  Step 18: Destroy media session
 *   Send DestroySessionRequest.
 *   Expect that the session is destroyed on the server.
 *
 * Test Teardown:
 *  Memory region created for the shared buffer is unmapped.
 *  Server is terminated.
 *
 * Expected Results:
 *  PlaybackError information from GStreamer is forwarded to Rialto Client
 *
 * Code:
 */
TEST_F(NonFatalPlayerErrorUpdatesTest, warningMessage)
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

    // Step 9: Audio generic warning
    willHandleWarningMessage(GST_CORE_ERROR, GST_CORE_ERROR_FAILED);
    gstWarning(kAudioDecryptorName.c_str());

    // Step 10: Video generic warning
    willHandleWarningMessage(GST_CORE_ERROR, GST_CORE_ERROR_FAILED);
    gstWarning(kVideoDecryptorName.c_str());

    // Step 11: Audio decrypt warning
    willHandleWarningMessage(GST_STREAM_ERROR, GST_STREAM_ERROR_DECRYPT);
    notifyPlaybackError(m_audioSourceId, PlaybackError::DECRYPTION, kAudioDecryptorName.c_str());

    // Step 12: Video decrypt warning
    willHandleWarningMessage(GST_STREAM_ERROR, GST_STREAM_ERROR_DECRYPT);
    notifyPlaybackError(m_videoSourceId, PlaybackError::DECRYPTION, kVideoDecryptorName.c_str());

    // Step 13: End of audio stream
    // Step 14: End of video stream
    willEos(&m_audioAppSrc);
    eosAudio(kFramesToPush);
    willEos(&m_videoAppSrc);
    eosVideo(kFramesToPush);

    // Step 15: Notify end of stream
    gstNotifyEos();

    // Step 16: Remove sources
    removeSource(m_audioSourceId);
    removeSource(m_videoSourceId);

    // Step 17: Stop
    willStop();
    stop();

    // Step 18: Destroy media session
    gstPlayerWillBeDestructed();
    destroySession();
}
} // namespace firebolt::rialto::server::ct
