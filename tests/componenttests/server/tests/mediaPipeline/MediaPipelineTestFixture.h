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

#ifndef FIREBOLT_RIALTO_SERVER_CT_MEDIA_PIPELINE_TEST_FIXTURE_H_
#define FIREBOLT_RIALTO_SERVER_CT_MEDIA_PIPELINE_TEST_FIXTURE_H_

#include "GstSrc.h"
#include "GstreamerStub.h"
#include "RialtoServerComponentTest.h"

namespace firebolt::rialto::server::ct
{
class MediaPipelineTest : public RialtoServerComponentTest
{
public:
    MediaPipelineTest();
    ~MediaPipelineTest() override = default;

    void gstPlayerWillBeCreated();
    void gstPlayerWillBeDestructed();
    void audioSourceWillBeAttached();
    void videoSourceWillBeAttached();
    void sourceWillBeSetup();
    void willSetupAndAddSource(GstAppSrc *appSrc);
    void willFinishSetupAndAddSource();
    void willPause();

    void createSession();
    void load();
    void attachAudioSource();
    void attachVideoSource();
    void setupSource();
    void indicateAllSourcesAttached();
    void pause();

    int m_sessionId{-1};
    int m_audioSourceId{-1};
    int m_videoSourceId{-1};
    GstElement m_pipeline{};
    GstBus m_bus{};
    GstElement m_playsink{};
    GstBin m_rialtoSrcBin = {};
    GstRialtoSrcPrivate m_rialtoSrcPriv = {};
    GstRialtoSrc m_rialtoSource = {m_rialtoSrcBin, &m_rialtoSrcPriv};
    GstreamerStub m_gstreamerStub{m_glibWrapperMock, m_gstWrapperMock, &m_pipeline, &m_bus, GST_ELEMENT(&m_rialtoSource)};
    GstAppSrc m_audioAppSrc{};
    GstAppSrc m_videoAppSrc{};
    GFlagsClass m_flagsClass{};
    GFlagsValue m_audioFlag{1, "audio", "audio"};
    GFlagsValue m_videoFlag{2, "video", "video"};
    GFlagsValue m_nativeVideoFlag{3, "native-video", "native-video"};
    GstCaps m_audioCaps{};
    GstCaps m_videoCaps{};
    gchar m_audioCapsStr{};
    gchar m_videoCapsStr{};
    std::string m_sourceName{"src_0"};
    GstPad m_pad{};
    GstPad m_ghostPad{};
};
} // namespace firebolt::rialto::server::ct

#endif // FIREBOLT_RIALTO_SERVER_CT_MEDIA_PIPELINE_TEST_FIXTURE_H_
