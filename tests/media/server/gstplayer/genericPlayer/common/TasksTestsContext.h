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

#ifndef TASKS_TESTS_CONTEXT_H_
#define TASKS_TESTS_CONTEXT_H_

#include "DataReaderMock.h"
#include "DecryptionServiceMock.h"
#include "GenericPlayerContext.h"
#include "GlibWrapperMock.h"
#include "GstGenericPlayerPrivateMock.h"
#include "GstGenericPlayerClientMock.h"
#include "GstWrapperMock.h"
#include "GstSrcMock.h"
#include "RdkGstreamerUtilsWrapperMock.h"
#include <memory>

/**
 * @brief TasksTests context
 *
 * Stores all objects and non-const variables so that constuction and destruction can be managed.
 */
class TasksTestsContext
{
public:
    firebolt::rialto::server::GenericPlayerContext m_context;

    // Mocks
    StrictMock<firebolt::rialto::server::GstGenericPlayerPrivateMock> m_gstPlayer;
    StrictMock<firebolt::rialto::server::GstGenericPlayerClientMock> m_gstPlayerClient;
    std::shared_ptr<firebolt::rialto::server::GlibWrapperMock> m_glibWrapper{
        std::make_shared<StrictMock<firebolt::rialto::server::GlibWrapperMock>>()};
    std::shared_ptr<firebolt::rialto::server::GstWrapperMock> m_gstWrapper{
        std::make_shared<StrictMock<firebolt::rialto::server::GstWrapperMock>>()};
    std::shared_ptr<firebolt::rialto::server::RdkGstreamerUtilsWrapperMock> m_rdkGstreamerUtilsWrapper{
        std::make_shared<StrictMock<firebolt::rialto::server::RdkGstreamerUtilsWrapperMock>>()};
    std::shared_ptr<StrictMock<firebolt::rialto::server::DecryptionServiceMock>> m_decryptionServiceMock{
        std::make_shared<StrictMock<firebolt::rialto::server::DecryptionServiceMock>>()};
    std::shared_ptr<StrictMock<firebolt::rialto::server::GstSrcMock>> m_gstSrc{
        std::make_shared<StrictMock<firebolt::rialto::server::GstSrcMock>>()};
    std::shared_ptr<StrictMock<firebolt::rialto::server::DataReaderMock>> m_dataReader{
        std::make_shared<StrictMock<firebolt::rialto::server::DataReaderMock>>()};

    // Gstreamer members
    GstElement m_element{};
    GstElementFactory *m_elementFactory{};
    GstElement m_pipeline{};
    GstBuffer m_audioBuffer{};
    GstBuffer m_videoBuffer{};
    GstCaps m_gstCaps1{};
    GstCaps m_gstCaps2{};
    GstElement m_appSrcAudio{};
    GstElement m_appSrcVideo{};
    GstBin m_bin{};
    GstObject m_obj1{};
    GstObject m_obj2{};
    GstElement m_audioDecodeBin{};
    GstElement m_audioParentSink{};
    GstAppSrcCallbacks m_audioCallbacks{};
    GstAppSrcCallbacks m_videoCallbacks{};
    GstStructure m_structure{};
    GstEvent m_event{};
    GstSegment m_segment{};
    GParamSpec m_paramSpec{};
    GstEvent m_event2{};

    // Glib members
    guint m_signals[1]{123};
    GCallback m_audioUnderflowCallback;
    GCallback m_videoUnderflowCallback;
    gchar m_capsStr{};
    gchar m_videoStr[7]{"video/"};
    gchar m_audioStr[7]{"audio/"};
    gchar m_typefindElementName[9]{"typefind"};
    gchar m_parseElementName[6]{"parse"};
    gchar m_decoderElementName[10]{"decodebin"};
    gchar m_audioSinkElementName[10]{"audiosink"};
    gchar m_elementName[5]{"sink"};
    gchar m_binElementName[5]{"bin"};
    gpointer m_videoUserData{};
    gpointer m_audioUserData{};

    // Standard members
    bool m_underflowFlag{false};
    bool m_underflowEnabled{false};
    firebolt::rialto::server::StreamInfo m_streamInfoAudio{&m_appSrcAudio, true};
    firebolt::rialto::server::StreamInfo m_streamInfoVideo{&m_appSrcVideo, true};
};

#endif // TASKS_TESTS_CONTEXT_H_
