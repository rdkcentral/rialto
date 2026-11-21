/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
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
#include "ExpectMessage.h"
#include "MediaPipelineTest.h"
#include "MessageBuilders.h"

#include <cstdint>
#include <cstdlib>

using testing::_;
using testing::Invoke;
using testing::Return;
using testing::StrEq;

namespace
{
constexpr const char *kCaptureHalLibraryEnv{"RIALTO_CAPTURE_HAL_LIBRARY"};
constexpr unsigned kFramesToPush{1};
constexpr int kFrameCountInPlayingState{3};
constexpr int kPositionInPaused{10};
constexpr double kSeekPlaybackRate{1.0};
const std::string kVideoSinkTypeName{"VideoSink"};
} // namespace

namespace firebolt::rialto::server::ct
{
/**
 * Attach and detach results for each pipeline state of proposal 3.5, against the real server pipeline.
 */
class VideoFrameCaptureAttachTest : public MediaPipelineTest
{
public:
    VideoFrameCaptureAttachTest()
    {
        // the capture HAL is loaded on the first create
        setenv(kCaptureHalLibraryEnv, FAKE_CAPTURE_HAL_PATH, 1);
    }

    ~VideoFrameCaptureAttachTest() override { unsetenv(kCaptureHalLibraryEnv); }

    std::uint32_t createCapture()
    {
        CreateVideoFrameCaptureRequest request;
        request.set_api(GRAPHICS_API_TYPE_GL);
        std::uint32_t captureId{0};
        ConfigureAction<CreateVideoFrameCapture>(m_clientStub)
            .send(request)
            .expectSuccess()
            .matchResponse([&](const CreateVideoFrameCaptureResponse &response) { captureId = response.capture_id(); });
        EXPECT_NE(captureId, 0U);
        return captureId;
    }

    void attach(std::uint32_t captureId, bool expectSuccess)
    {
        AttachVideoFrameCaptureRequest request;
        request.set_session_id(m_sessionId);
        request.set_capture_id(captureId);
        auto action = ConfigureAction<AttachVideoFrameCapture>(m_clientStub);
        action.send(request);
        expectSuccess ? action.expectSuccess() : action.expectFailure();
    }

    void detach(bool expectSuccess)
    {
        DetachVideoFrameCaptureRequest request;
        request.set_session_id(m_sessionId);
        auto action = ConfigureAction<DetachVideoFrameCapture>(m_clientStub);
        action.send(request);
        expectSuccess ? action.expectSuccess() : action.expectFailure();
    }

    // In a state after commit: a new capture is refused, and detach succeeds only in STOPPED.
    void checkCommittedState(bool detachAllowed)
    {
        attach(m_otherCaptureId, false);
        if (!detachAllowed)
            detach(false);
    }

    void loadAndCommit()
    {
        gstPlayerWillBeCreated();
        load();
        attach(m_captureId, true); // attached before commit, sources attached after it
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
    }

    void pauseWithPreroll()
    {
        willPause();
        pause();
        {
            ExpectMessage<firebolt::rialto::NetworkStateChangeEvent> expectedNetworkStateChange{m_clientStub};
            pushAudioData(kFramesToPush, kPrerollNumFrames);
            pushVideoData(kFramesToPush, kPrerollNumFrames);
            ASSERT_TRUE(expectedNetworkStateChange.getMessage());
        }
        willNotifyPaused();
        notifyPaused();
    }

    void destroyCapture(std::uint32_t captureId)
    {
        DestroyVideoFrameCaptureRequest request;
        request.set_capture_id(captureId);
        ConfigureAction<DestroyVideoFrameCapture>(m_clientStub).send(request).expectSuccess();
    }

    void createAndLoadSession()
    {
        createSession();
        gstPlayerWillBeCreated();
        load();
    }

    void destroyLoadedSession()
    {
        gstPlayerWillBeDestructed();
        destroySession();
    }

    std::uint32_t m_captureId{0};
    std::uint32_t m_otherCaptureId{0};
};

/*
 * Component Test: Attach and detach through a whole playback
 * Test Objective:
 *  Check the attach and detach result in every state a normal playback passes through.
 *
 * Test Steps:
 *  Step 1: Created, not loaded: attach and detach fail.
 *  Step 2: Loaded: attach, detach, and attach again succeed; a second capture is refused.
 *  Step 3: Committed (IDLE), PAUSED, after a seek (SEEK_DONE), PLAYING and END_OF_STREAM: a new attach and the
 *          detach fail, and the capture stays attached.
 *  Step 4: STOPPED: a new attach fails and the detach succeeds.
 *
 * Expected Results:
 *  The results follow proposal 3.5.
 *
 * Code:
 */
TEST_F(VideoFrameCaptureAttachTest, shouldFollowTheStateTableThroughPlayback)
{
    m_captureId = createCapture();
    m_otherCaptureId = createCapture();

    // Step 1: Created, not loaded
    createSession();
    attach(m_captureId, false);
    detach(false);

    // Step 2: Loaded
    gstPlayerWillBeCreated();
    load();
    attach(m_captureId, true);
    attach(m_otherCaptureId, false); // one capture per pipeline
    detach(true);
    detach(false); // nothing attached

    // Step 3: committed states
    audioSourceWillBeAttached();
    attachAudioSource();
    attach(m_captureId, true); // source order does not matter
    videoSourceWillBeAttached();
    attachVideoSource();
    sourceWillBeSetup();
    setupSource();
    willSetupAndAddSource(&m_audioAppSrc);
    willSetupAndAddSource(&m_videoAppSrc);
    willFinishSetupAndAddSource();
    indicateAllSourcesAttached({&m_audioAppSrc, &m_videoAppSrc});
    checkCommittedState(false); // IDLE

    pauseWithPreroll();
    checkCommittedState(false); // PAUSED

    EXPECT_CALL(*m_gstWrapperMock, gstElementSeek(&m_pipeline, kSeekPlaybackRate, GST_FORMAT_TIME,
                                                  static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH), GST_SEEK_TYPE_SET,
                                                  kPositionInPaused, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE))
        .WillOnce(Return(TRUE));
    {
        // SEEKING and SEEK_DONE are both reported during the call; the request runs after SEEK_DONE
        ExpectMessage<PlaybackStateChangeEvent> expectedSeekDone{m_clientStub};
        expectedSeekDone.setFilter([](const PlaybackStateChangeEvent &event)
                                   { return event.state() == PlaybackStateChangeEvent_PlaybackState_SEEK_DONE; });
        ExpectMessage<NeedMediaDataEvent> expectedAudioNeedData{m_clientStub};
        ExpectMessage<NeedMediaDataEvent> expectedVideoNeedData{m_clientStub};
        expectedAudioNeedData.setFilter([&](const NeedMediaDataEvent &event)
                                        { return event.source_id() == m_audioSourceId; });
        expectedVideoNeedData.setFilter([&](const NeedMediaDataEvent &event)
                                        { return event.source_id() == m_videoSourceId; });
        ConfigureAction<SetPosition>(m_clientStub)
            .send(createSetPositionRequest(m_sessionId, kPositionInPaused))
            .expectSuccess();
        ASSERT_TRUE(expectedSeekDone.getMessage());
        m_lastAudioNeedData = expectedAudioNeedData.getMessage();
        m_lastVideoNeedData = expectedVideoNeedData.getMessage();
        ASSERT_TRUE(m_lastAudioNeedData);
        ASSERT_TRUE(m_lastVideoNeedData);
    }
    checkCommittedState(false); // SEEK_DONE

    willPlay();
    play();
    pushAudioData(kFramesToPush, kFrameCountInPlayingState);
    pushVideoData(kFramesToPush, kFrameCountInPlayingState);
    checkCommittedState(false); // PLAYING

    willEos(&m_audioAppSrc);
    eosAudio(kFramesToPush);
    willEos(&m_videoAppSrc);
    eosVideo(kFramesToPush);
    gstNotifyEos();
    checkCommittedState(false); // END_OF_STREAM

    // Step 4: STOPPED
    removeSource(m_audioSourceId);
    removeSource(m_videoSourceId);
    willStop();
    stop();
    checkCommittedState(true);
    detach(true);

    gstPlayerWillBeDestructed();
    destroySession();
}

/*
 * Component Test: Attach and detach after a playback failure
 * Test Objective:
 *  Check that attach and detach both fail in FAILURE.
 *
 * Test Steps:
 *  Step 1: Attach before commit, commit, and let pause fail so the pipeline reports FAILURE.
 *  Step 2: A new attach fails and the detach fails.
 *
 * Expected Results:
 *  The capture stays attached in FAILURE, as proposal 3.5 requires.
 *
 * Code:
 */
TEST_F(VideoFrameCaptureAttachTest, shouldRefuseAttachAndDetachInFailure)
{
    m_captureId = createCapture();
    m_otherCaptureId = createCapture();
    createSession();

    // Step 1: Commit, then fail
    loadAndCommit();
    EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(&m_pipeline, GST_STATE_PAUSED))
        .WillOnce(Return(GST_STATE_CHANGE_FAILURE));
    {
        ExpectMessage<firebolt::rialto::PlaybackStateChangeEvent> expectedFailure{m_clientStub};
        ConfigureAction<Pause>(m_clientStub).send(createPauseRequest(m_sessionId)).expectSuccess();
        auto received{expectedFailure.getMessage()};
        ASSERT_TRUE(received);
        EXPECT_EQ(received->state(), ::firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_FAILURE);
    }

    // Step 2: FAILURE
    checkCommittedState(false);

    gstPlayerWillBeDestructed();
    destroySession();
}

/*
 * Component Test: Reuse after detach and after pipeline destruction
 * Test Objective:
 *  Check that a capture can be attached to a new pipeline once it was detached, or its pipeline destroyed.
 *
 * Test Steps:
 *  Step 1: Attach the capture to a loaded pipeline and detach it; destroy the pipeline; attach it to a new one.
 *  Step 2: Destroy that pipeline while the capture is attached; attach it to a third pipeline.
 *
 * Expected Results:
 *  Each attach succeeds.
 *
 * Code:
 */
TEST_F(VideoFrameCaptureAttachTest, shouldReuseCaptureAfterDetachAndAfterPipelineDestruction)
{
    m_captureId = createCapture();

    // Step 1: Reuse after detach
    createAndLoadSession();
    attach(m_captureId, true);
    detach(true);
    destroyLoadedSession();
    createAndLoadSession();
    attach(m_captureId, true);

    // Step 2: Reuse after pipeline destruction
    destroyLoadedSession();
    createAndLoadSession();
    attach(m_captureId, true);
    destroyLoadedSession();
}

/*
 * Component Test: Destroying an attached capture
 * Test Objective:
 *  Check that destroying the registration of an attached capture detaches it.
 *
 * Test Steps:
 *  Step 1: Attach a capture to a loaded pipeline and destroy the capture.
 *  Step 2: Attach a new capture to the same pipeline.
 *
 * Expected Results:
 *  The new capture is attached.
 *
 * Code:
 */
TEST_F(VideoFrameCaptureAttachTest, shouldDetachDestroyedCapture)
{
    m_captureId = createCapture();
    createAndLoadSession();

    // Step 1: Destroy the attached capture
    attach(m_captureId, true);
    destroyCapture(m_captureId);

    // Step 2: The pipeline takes a new capture
    attach(createCapture(), true);
    destroyLoadedSession();
}

/*
 * Component Test: setVideoWindow while a capture is attached
 * Test Objective:
 *  Check that setVideoWindow fails while a capture is attached and works as before after detach.
 *
 * Test Steps:
 *  Step 1: Attach a capture to a loaded pipeline; setVideoWindow fails.
 *  Step 2: Detach; setVideoWindow sets the sink rectangle.
 *
 * Expected Results:
 *  The video window is refused only while capturing.
 *
 * Code:
 */
TEST_F(VideoFrameCaptureAttachTest, shouldRefuseVideoWindowWhileCaptureIsAttached)
{
    m_captureId = createCapture();
    createAndLoadSession();

    // Step 1: Refused while attached
    attach(m_captureId, true);
    ConfigureAction<SetVideoWindow>(m_clientStub).send(createSetVideoWindowRequest(m_sessionId)).expectFailure();

    // Step 2: As before after detach
    detach(true);
    GstElementFactory *elementFactory = gst_element_factory_find("fakesrc");
    GstElement *videoSink = gst_element_factory_create(elementFactory, nullptr);
    gst_object_unref(elementFactory);
    GParamSpec paramSpec{};
    EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, StrEq("video-sink"), _))
        .WillOnce(Invoke([&](gpointer, const gchar *, void *element)
                         { *reinterpret_cast<GstElement **>(element) = videoSink; }));
    EXPECT_CALL(*m_glibWrapperMock, gTypeName(G_OBJECT_TYPE(videoSink))).WillOnce(Return(kVideoSinkTypeName.c_str()));
    EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, StrEq("rectangle"))).WillOnce(Return(&paramSpec));
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(videoSink, StrEq("rectangle")));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(videoSink));
    ConfigureAction<SetVideoWindow>(m_clientStub).send(createSetVideoWindowRequest(m_sessionId)).expectSuccess();

    destroyLoadedSession();
    gst_object_unref(videoSink);
}
} // namespace firebolt::rialto::server::ct
