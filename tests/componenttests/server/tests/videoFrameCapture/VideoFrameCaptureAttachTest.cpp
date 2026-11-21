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

#include "VideoFrameCaptureFixture.h"

namespace firebolt::rialto::server::ct
{
/**
 * Attach and detach results for each pipeline state of proposal 3.5, and the capture commit (R7), against the real
 * server pipeline.
 */
class VideoFrameCaptureAttachTest : public VideoFrameCaptureFixture
{
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
    captureSinkWillBeConfigured();
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

    captureSinkWillBeDropped();
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

    captureSinkWillBeDropped();
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

/*
 * Component Test: Capture commit success and failures (R7)
 * Test Objective:
 *  Check allSourcesAttached() with a capture against the real server, the fake capture HAL and a stand-in admission:
 *  success, each failure, and that no HAL session is left after any failure.
 *
 * Code:
 */
TEST_F(VideoFrameCaptureAttachTest, shouldFailCommitWithoutVideoSource)
{
    m_captureId = createCapture();
    createSession();
    prepareCommit(false);
    commitShouldFail();
    EXPECT_EQ(liveHalSessions(), 0);
    gstPlayerWillBeDestructed();
    destroySession();
}

TEST_F(VideoFrameCaptureAttachTest, shouldFailCommitWithAnotherVideoSinkOverride)
{
    m_captureId = createCapture();
    createSession();
    prepareCommit(true);
    setenv("RIALTO_PLAYBIN_VIDEO_SINK", "autovideosink", 1); // after the player applied its own sinks
    commitShouldFail();
    unsetenv("RIALTO_PLAYBIN_VIDEO_SINK");
    EXPECT_EQ(liveHalSessions(), 0);
    gstPlayerWillBeDestructed();
    destroySession();
}

TEST_F(VideoFrameCaptureAttachTest, shouldRollBackWhenTheSinkRejectsItsConfiguration)
{
    m_captureId = createCapture();
    createSession();
    prepareCommit(true);
    captureSinkWillBeSetUpUntilReady();
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_pipeline, StrEq("video-sink"))).Times(2); // assign, clear
    EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(m_captureSink, GST_STATE_READY))
        .WillOnce(Return(GST_STATE_CHANGE_FAILURE));
    captureSinkWillBeDiscarded();
    commitShouldFail();
    EXPECT_EQ(liveHalSessions(), 0); // deleted after the sink was at NULL
    gstPlayerWillBeDestructed();
    destroySession();
}

TEST_F(VideoFrameCaptureAttachTest, shouldRollBackWhenAdmissionRefusesAndCommitOnRetry)
{
    m_captureId = createCapture();
    createSession();
    prepareCommit(true);

    // admission refuses: the configured sink is discarded and the session deleted
    *m_admissionAccepts = false;
    captureSinkWillBeSetUpUntilReady();
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_pipeline, StrEq("video-sink"))).Times(2).RetiresOnSaturation();
    EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(m_captureSink, GST_STATE_READY))
        .WillOnce(Return(GST_STATE_CHANGE_SUCCESS))
        .RetiresOnSaturation();
    EXPECT_CALL(*m_gstWrapperMock, gstElementGetState(m_captureSink, _, _, _))
        .WillOnce(DoAll(SetArgPointee<1>(GST_STATE_READY), Return(GST_STATE_CHANGE_SUCCESS)))
        .RetiresOnSaturation();
    captureSinkWillBeDiscarded();
    commitShouldFail();
    EXPECT_EQ(liveHalSessions(), 0);

    // retry with the capture still attached: a new session, a new sink set-up, and the commit succeeds
    *m_admissionAccepts = true;
    willSetupAndAddSource(&m_audioAppSrc);
    willSetupAndAddSource(&m_videoAppSrc);
    willFinishSetupAndAddSource();
    captureSinkWillBeConfigured();
    indicateAllSourcesAttached({&m_audioAppSrc, &m_videoAppSrc});
    EXPECT_EQ(liveHalSessions(), 1);

    captureSinkWillBeDropped();
    gstPlayerWillBeDestructed();
    destroySession();
    EXPECT_EQ(liveHalSessions(), 0); // retired with its pipeline, sink at NULL, no frame held: deleted (R10)
    destroyCapture(m_captureId);
}

TEST_F(VideoFrameCaptureAttachTest, shouldUseOneSinkForCaptureAndSecondaryVideo)
{
    m_captureId = createCapture();
    // a secondary-video pipeline: small maximum size
    ConfigureAction<CreateSession>(m_clientStub)
        .send(createCreateSessionRequest(640, 360))
        .expectSuccess()
        .matchResponse([&](const ::firebolt::rialto::CreateSessionResponse &resp) { m_sessionId = resp.session_id(); });

    // the shipping secondary-video sink and erm context at load
    GstElement *shippingSink = gst_element_factory_make("fakesink", nullptr);
    gstPlayerWillBeCreated();
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryFind(StrEq("westerossink")))
        .WillOnce(Return(m_sinkFactory))
        .RetiresOnSaturation();
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryCreate(m_sinkFactory, nullptr))
        .WillOnce(Return(shippingSink))
        .RetiresOnSaturation();
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(GST_OBJECT(m_sinkFactory))).RetiresOnSaturation();
    EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, StrEq("res-usage")))
        .WillOnce(Return(&m_paramSpec))
        .RetiresOnSaturation();
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(shippingSink, StrEq("res-usage")));
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_pipeline, StrEq("video-sink"))).RetiresOnSaturation();
    GstContext *context = gst_context_new("erm", false);
    GstStructure structure{};
    EXPECT_CALL(*m_gstWrapperMock, gstContextNew(StrEq("erm"), false)).WillOnce(Return(context));
    EXPECT_CALL(*m_gstWrapperMock, gstContextWritableStructure(context)).WillOnce(Return(&structure));
    EXPECT_CALL(*m_gstWrapperMock, gstStructureSetUintStub(&structure, StrEq("res-usage"), G_TYPE_UINT, 0x0u));
    EXPECT_CALL(*m_gstWrapperMock, gstElementSetContext(_, context));
    EXPECT_CALL(*m_gstWrapperMock, gstContextUnref(context));
    load();
    attach(m_captureId, true);
    audioSourceWillBeAttached();
    attachAudioSource();
    videoSourceWillBeAttached();
    attachVideoSource();
    sourceWillBeSetup();
    setupSource();

    // the capture sink replaces it, carrying both res-usage 0 and the capture session
    captureSinkWillBeSetUpUntilReady("640x360");
    EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, StrEq("res-usage"))).WillOnce(Return(&m_paramSpec));
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(m_captureSink, StrEq("res-usage")));
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_pipeline, StrEq("video-sink")));
    EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(m_captureSink, GST_STATE_READY))
        .WillOnce(Return(GST_STATE_CHANGE_SUCCESS));
    EXPECT_CALL(*m_gstWrapperMock, gstElementGetState(m_captureSink, _, _, _))
        .WillOnce(DoAll(SetArgPointee<1>(GST_STATE_READY), Return(GST_STATE_CHANGE_SUCCESS)));
    willSetupAndAddSource(&m_audioAppSrc);
    willSetupAndAddSource(&m_videoAppSrc);
    willFinishSetupAndAddSource();
    indicateAllSourcesAttached({&m_audioAppSrc, &m_videoAppSrc});

    captureSinkWillBeDropped();
    gstPlayerWillBeDestructed();
    destroySession();
    gst_context_unref(context);
    gst_object_unref(shippingSink);
}
} // namespace firebolt::rialto::server::ct
