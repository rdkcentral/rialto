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
#include "ExpectMessage.h"
#include "Matchers.h"
#include "MediaCommon.h"
#include "MessageBuilders.h"

using testing::_;
using testing::Invoke;
using testing::Return;

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
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryFind(CharStrMatcher("rialtosrc"))).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstElementRegister(0, CharStrMatcher("rialtosrc"), _, _));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(CharStrMatcher("playbin"), _)).WillOnce(Return(&m_pipeline));
    EXPECT_CALL(*m_glibWrapperMock, gTypeFromName(CharStrMatcher("GstPlayFlags")))
        .Times(3)
        .WillRepeatedly(Return(m_gstPlayFlagsType));
    EXPECT_CALL(*m_glibWrapperMock, gTypeClassRef(m_gstPlayFlagsType)).Times(3).WillRepeatedly(Return(&m_flagsClass));

    EXPECT_CALL(*m_glibWrapperMock, gFlagsGetValueByNick(&m_flagsClass, CharStrMatcher("audio")))
        .WillOnce(Return(&m_audioFlag));
    EXPECT_CALL(*m_glibWrapperMock, gFlagsGetValueByNick(&m_flagsClass, CharStrMatcher("video")))
        .WillOnce(Return(&m_videoFlag));
    EXPECT_CALL(*m_glibWrapperMock, gFlagsGetValueByNick(&m_flagsClass, CharStrMatcher("native-video")))
        .WillOnce(Return(&m_nativeVideoFlag));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryFind(CharStrMatcher("brcmaudiosink"))).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_pipeline, CharStrMatcher("flags")));
    EXPECT_CALL(*m_glibWrapperMock,
                gSignalConnect(&m_pipeline, CharStrMatcher("source-setup"), NotNullMatcher(), NotNullMatcher()))
        .WillOnce(Invoke(
            [this](gpointer instance, const gchar *detailed_signal, GCallback c_handler, gpointer data)
            {
                m_setupSourceFunc = c_handler;
                m_setupSourceUserData = data;
                return m_setupSourceSignalId;
            }));
    EXPECT_CALL(*m_glibWrapperMock,
                gSignalConnect(&m_pipeline, CharStrMatcher("element-setup"), NotNullMatcher(), NotNullMatcher()))
        .WillOnce(Invoke(
            [this](gpointer instance, const gchar *detailed_signal, GCallback c_handler, gpointer data)
            {
                m_setupElementFunc = c_handler;
                m_setupElementUserData = data;
                return m_setupElementSignalId;
            }));
    EXPECT_CALL(*m_glibWrapperMock,
                gSignalConnect(&m_pipeline, CharStrMatcher("deep-element-added"), NotNullMatcher(), NotNullMatcher()))
        .WillOnce(Invoke(
            [this](gpointer instance, const gchar *detailed_signal, GCallback c_handler, gpointer data)
            {
                m_deepElementAddedFunc = c_handler;
                m_deepElementAddedUserData = data;
                return m_deepElementAddedSignalId;
            }));
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_pipeline, CharStrMatcher("uri")));
    EXPECT_CALL(*m_gstWrapperMock, gstBinGetByName(GST_BIN(&m_pipeline), CharStrMatcher("playsink")))
        .WillOnce(Return(&m_playsink));
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_playsink, CharStrMatcher("send-event-mode")));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_playsink));
    EXPECT_CALL(*m_gstWrapperMock, gstPipelineGetBus(GST_PIPELINE(&m_pipeline))).WillOnce(Return(nullptr));
}

void MediaPipelineTest::gstPlayerWillBeDestructed()
{
    EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(&m_pipeline, GST_STATE_NULL));
    EXPECT_CALL(*m_gstWrapperMock, gstPipelineGetBus(GST_PIPELINE(&m_pipeline))).WillOnce(Return(&m_bus));
    EXPECT_CALL(*m_gstWrapperMock, gstBusSetSyncHandler(&m_bus, nullptr, nullptr, nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_bus));
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
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&m_audioCaps, CharStrMatcher("channels"), G_TYPE_INT, 2));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&m_audioCaps, CharStrMatcher("rate"), G_TYPE_INT, 48000));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsToString(&m_audioCaps)).WillOnce(Return(&m_capsStr));
    EXPECT_CALL(*m_glibWrapperMock, gFree(&m_capsStr));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(CharStrMatcher("appsrc"), CharStrMatcher("audsrc")))
        .WillOnce(Return(GST_ELEMENT(&m_audioAppSrc)));
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetCaps(&m_audioAppSrc, &m_audioCaps));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&m_audioCaps));
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
} // namespace firebolt::rialto::server::ct
