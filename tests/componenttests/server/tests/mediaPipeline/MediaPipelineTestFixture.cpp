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

#include "MediaPipelineTestFixture.h"
#include "ActionTraits.h"
#include "ConfigureAction.h"
#include "Constants.h"
#include "ExpectMessage.h"
#include "Matchers.h"
#include "MediaCommon.h"
#include "MessageBuilders.h"

using testing::_;
using testing::DoAll;
using testing::Invoke;
using testing::Return;
using testing::SaveArg;
using testing::SaveArgPointee;
using testing::SetArgPointee;

namespace
{
const std::string kNullStateName{"NULL"};
const std::string kPausedStateName{"PAUSED"};
constexpr GType kGstPlayFlagsType{static_cast<GType>(123)};
} // namespace

namespace firebolt::rialto::server::ct
{
MediaPipelineTest::MediaPipelineTest()
{
    willConfigureSocket();
    configureSutInActiveState();
    connectClient();
}

void MediaPipelineTest::gstPlayerWillBeCreated()
{
    m_gstreamerStub.setupPipeline();
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryFind(CharStrMatcher("rialtosrc"))).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstElementRegister(0, CharStrMatcher("rialtosrc"), _, _));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(CharStrMatcher("playbin"), _)).WillOnce(Return(&m_pipeline));
    EXPECT_CALL(*m_glibWrapperMock, gTypeFromName(CharStrMatcher("GstPlayFlags")))
        .Times(3)
        .WillRepeatedly(Return(kGstPlayFlagsType));
    EXPECT_CALL(*m_glibWrapperMock, gTypeClassRef(kGstPlayFlagsType)).Times(3).WillRepeatedly(Return(&m_flagsClass));

    EXPECT_CALL(*m_glibWrapperMock, gFlagsGetValueByNick(&m_flagsClass, CharStrMatcher("audio")))
        .WillOnce(Return(&m_audioFlag));
    EXPECT_CALL(*m_glibWrapperMock, gFlagsGetValueByNick(&m_flagsClass, CharStrMatcher("video")))
        .WillOnce(Return(&m_videoFlag));
    EXPECT_CALL(*m_glibWrapperMock, gFlagsGetValueByNick(&m_flagsClass, CharStrMatcher("native-video")))
        .WillOnce(Return(&m_nativeVideoFlag));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryFind(CharStrMatcher("brcmaudiosink"))).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_pipeline, CharStrMatcher("flags")));
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_pipeline, CharStrMatcher("uri")));
    EXPECT_CALL(*m_gstWrapperMock, gstBinGetByName(GST_BIN(&m_pipeline), CharStrMatcher("playsink")))
        .WillOnce(Return(&m_playsink));
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_playsink, CharStrMatcher("send-event-mode")));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_playsink));
}

void MediaPipelineTest::gstPlayerWillBeDestructed()
{
    EXPECT_CALL(*m_gstWrapperMock, gstMessageParseStateChanged(_, _, _, _))
        .WillRepeatedly(DoAll(SetArgPointee<1>(GST_STATE_NULL), SetArgPointee<2>(GST_STATE_NULL),
                              SetArgPointee<3>(GST_STATE_NULL)));
    EXPECT_CALL(*m_gstWrapperMock, gstElementStateGetName(GST_STATE_NULL)).WillRepeatedly(Return(kNullStateName.c_str()));
    EXPECT_CALL(*m_gstWrapperMock, gstDebugBinToDotFileWithTs(GST_BIN(&m_pipeline), _, _));
    m_gstreamerStub.sendStateChanged(GST_STATE_NULL, GST_STATE_NULL, GST_STATE_NULL);
    EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(&m_pipeline, GST_STATE_NULL))
        .WillOnce(Return(GST_STATE_CHANGE_SUCCESS));
    EXPECT_CALL(*m_gstWrapperMock, gstPipelineGetBus(GST_PIPELINE(&m_pipeline))).WillOnce(Return(&m_bus));
    EXPECT_CALL(*m_gstWrapperMock, gstBusSetSyncHandler(&m_bus, nullptr, nullptr, nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_bus)).Times(2);
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_pipeline));
}

void MediaPipelineTest::audioSourceWillBeAttached()
{
    EXPECT_CALL(*m_gstWrapperMock, gstCapsNewEmptySimple(CharStrMatcher("audio/mpeg"))).WillOnce(Return(&m_audioCaps));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleStringStub(&m_audioCaps, CharStrMatcher("alignment"), G_TYPE_STRING,
                                                              CharStrMatcher("nal")));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleStringStub(&m_audioCaps, CharStrMatcher("stream-format"),
                                                              G_TYPE_STRING, CharStrMatcher("raw")));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&m_audioCaps, CharStrMatcher("mpegversion"), G_TYPE_INT, 4));
    EXPECT_CALL(*m_gstWrapperMock,
                gstCapsSetSimpleIntStub(&m_audioCaps, CharStrMatcher("channels"), G_TYPE_INT, kNumOfChannels));
    EXPECT_CALL(*m_gstWrapperMock,
                gstCapsSetSimpleIntStub(&m_audioCaps, CharStrMatcher("rate"), G_TYPE_INT, kSampleRate));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsToString(&m_audioCaps)).WillOnce(Return(&m_audioCapsStr));
    EXPECT_CALL(*m_glibWrapperMock, gFree(&m_audioCapsStr));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(CharStrMatcher("appsrc"), CharStrMatcher("audsrc")))
        .WillOnce(Return(GST_ELEMENT(&m_audioAppSrc)));
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetCaps(&m_audioAppSrc, &m_audioCaps));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&m_audioCaps));
}

void MediaPipelineTest::videoSourceWillBeAttached()
{
    EXPECT_CALL(*m_gstWrapperMock, gstCapsNewEmptySimple(CharStrMatcher("video/x-h264"))).WillOnce(Return(&m_videoCaps));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleStringStub(&m_videoCaps, CharStrMatcher("alignment"), G_TYPE_STRING,
                                                              CharStrMatcher("nal")));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleStringStub(&m_videoCaps, CharStrMatcher("stream-format"),
                                                              G_TYPE_STRING, CharStrMatcher("raw")));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&m_videoCaps, CharStrMatcher("width"), G_TYPE_INT, kWidth));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&m_videoCaps, CharStrMatcher("height"), G_TYPE_INT, kHeight));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsToString(&m_videoCaps)).WillOnce(Return(&m_videoCapsStr));
    EXPECT_CALL(*m_glibWrapperMock, gFree(&m_videoCapsStr));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(CharStrMatcher("appsrc"), CharStrMatcher("vidsrc")))
        .WillOnce(Return(GST_ELEMENT(&m_videoAppSrc)));
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetCaps(&m_videoAppSrc, &m_videoCaps));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&m_videoCaps));
}

void MediaPipelineTest::sourceWillBeSetup()
{
    EXPECT_CALL(*m_gstWrapperMock, gstObjectRef(&m_rialtoSource));
    // Source will be unreferenced when GstPlayer is destructed.
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_rialtoSource));
}

void MediaPipelineTest::willSetupAndAddSource(GstAppSrc *appSrc)
{
    const guint64 kMaxBytes = ((appSrc == &m_audioAppSrc) ? (512 * 1024) : (8 * 1024 * 1024));
    m_gstreamerStub.setupAppSrcCallbacks(appSrc);
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(GST_ELEMENT(appSrc), CharStrMatcher("block")));
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(GST_ELEMENT(appSrc), CharStrMatcher("format")));
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(GST_ELEMENT(appSrc), CharStrMatcher("stream-type")));
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(GST_ELEMENT(appSrc), CharStrMatcher("min-percent")));
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetMaxBytes(appSrc, kMaxBytes));
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetStreamType(appSrc, GST_APP_STREAM_TYPE_SEEKABLE));
    EXPECT_CALL(*m_glibWrapperMock, gStrdupPrintfStub(_)).WillOnce(Return(m_sourceName.data())).RetiresOnSaturation();
    EXPECT_CALL(*m_gstWrapperMock, gstBinAdd(GST_BIN(&m_rialtoSource), GST_ELEMENT(appSrc)));
    EXPECT_CALL(*m_gstWrapperMock, gstElementGetStaticPad(GST_ELEMENT(appSrc), CharStrMatcher("src")))
        .WillOnce(Return(&m_pad));
    EXPECT_CALL(*m_gstWrapperMock, gstGhostPadNew(CharStrMatcher(m_sourceName.data()), &m_pad))
        .WillOnce(Return(&m_ghostPad))
        .RetiresOnSaturation();
    EXPECT_CALL(*m_gstWrapperMock, gstPadSetQueryFunction(&m_ghostPad, NotNullMatcher())).RetiresOnSaturation();
    EXPECT_CALL(*m_gstWrapperMock, gstPadSetActive(&m_ghostPad, TRUE)).RetiresOnSaturation();
    EXPECT_CALL(*m_gstWrapperMock, gstElementAddPad(GST_ELEMENT(&m_rialtoSource), &m_ghostPad)).RetiresOnSaturation();
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_pad)).RetiresOnSaturation();
    EXPECT_CALL(*m_gstWrapperMock, gstElementSyncStateWithParent(GST_ELEMENT(appSrc)));
    EXPECT_CALL(*m_glibWrapperMock, gFree(CharStrMatcher(m_sourceName.data()))).RetiresOnSaturation();
}

void MediaPipelineTest::willFinishSetupAndAddSource()
{
    EXPECT_CALL(*m_gstWrapperMock, gstElementNoMorePads(GST_ELEMENT(&m_rialtoSource)));
}

void MediaPipelineTest::willPause()
{
    EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(&m_pipeline, GST_STATE_PAUSED))
        .WillOnce(Return(GST_STATE_CHANGE_SUCCESS));
    EXPECT_CALL(*m_gstWrapperMock, gstMessageParseStateChanged(_, _, _, _))
        .WillRepeatedly(DoAll(SetArgPointee<1>(GST_STATE_NULL), SetArgPointee<2>(GST_STATE_PAUSED),
                              SetArgPointee<3>(GST_STATE_NULL)));
    EXPECT_CALL(*m_gstWrapperMock, gstElementStateGetName(GST_STATE_NULL)).WillRepeatedly(Return(kNullStateName.c_str()));
    EXPECT_CALL(*m_gstWrapperMock, gstElementStateGetName(GST_STATE_PAUSED))
        .WillRepeatedly(Return(kPausedStateName.c_str()));
    EXPECT_CALL(*m_gstWrapperMock, gstDebugBinToDotFileWithTs(GST_BIN(&m_pipeline), _, _));
}

void MediaPipelineTest::createSession()
{
    // Use matchResponse to store session id
    auto request{createCreateSessionRequest()};
    ConfigureAction<CreateSession>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse([&](const ::firebolt::rialto::CreateSessionResponse &resp) { m_sessionId = resp.session_id(); });
}

void MediaPipelineTest::load()
{
    ExpectMessage<firebolt::rialto::NetworkStateChangeEvent> expectedNetworkStateChange{m_clientStub};

    auto request = createLoadRequest(m_sessionId);
    ConfigureAction<Load>(m_clientStub).send(request).expectSuccess();

    auto receivedNetworkStateChange = expectedNetworkStateChange.getMessage();
    ASSERT_TRUE(receivedNetworkStateChange);
    EXPECT_EQ(receivedNetworkStateChange->session_id(), m_sessionId);
    EXPECT_EQ(receivedNetworkStateChange->state(), ::firebolt::rialto::NetworkStateChangeEvent_NetworkState_BUFFERING);
}

void MediaPipelineTest::attachAudioSource()
{
    auto attachAudioSourceReq{createAttachAudioSourceRequest(m_sessionId)};
    ConfigureAction<AttachSource>(m_clientStub)
        .send(attachAudioSourceReq)
        .expectSuccess()
        .matchResponse([&](const auto &resp) { m_audioSourceId = resp.source_id(); });
}

void MediaPipelineTest::attachVideoSource()
{
    auto attachVideoSourceReq{createAttachVideoSourceRequest(m_sessionId)};
    ConfigureAction<AttachSource>(m_clientStub)
        .send(attachVideoSourceReq)
        .expectSuccess()
        .matchResponse([&](const auto &resp) { m_videoSourceId = resp.source_id(); });
}

void MediaPipelineTest::setupSource()
{
    m_gstreamerStub.setupRialtoSource();
}

void MediaPipelineTest::indicateAllSourcesAttached()
{
    ExpectMessage<firebolt::rialto::PlaybackStateChangeEvent> expectedPlaybackStateChange(m_clientStub);

    auto allSourcesAttachedReq{createAllSourcesAttachedRequest(m_sessionId)};
    ConfigureAction<AllSourcesAttached>(m_clientStub).send(allSourcesAttachedReq).expectSuccess();

    auto receivedPlaybackStateChange{expectedPlaybackStateChange.getMessage()};
    ASSERT_TRUE(receivedPlaybackStateChange);
    EXPECT_EQ(receivedPlaybackStateChange->session_id(), m_sessionId);
    EXPECT_EQ(receivedPlaybackStateChange->state(), ::firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_IDLE);
}

void MediaPipelineTest::pause()
{
    auto pauseReq{createPauseRequest(m_sessionId)};
    ConfigureAction<Pause>(m_clientStub).send(pauseReq).expectSuccess();

    ExpectMessage<firebolt::rialto::PlaybackStateChangeEvent> expectedPlaybackStateChange{m_clientStub};

    m_gstreamerStub.sendStateChanged(GST_STATE_NULL, GST_STATE_PAUSED, GST_STATE_NULL);

    auto receivedPlaybackStateChange{expectedPlaybackStateChange.getMessage()};
    ASSERT_TRUE(receivedPlaybackStateChange);
    EXPECT_EQ(receivedPlaybackStateChange->session_id(), m_sessionId);
    EXPECT_EQ(receivedPlaybackStateChange->state(), ::firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_PAUSED);
}
} // namespace firebolt::rialto::server::ct
