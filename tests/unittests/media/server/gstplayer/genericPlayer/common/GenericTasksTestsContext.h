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

#ifndef GENERIC_TASKS_TESTS_CONTEXT_H_
#define GENERIC_TASKS_TESTS_CONTEXT_H_

#include "DataReaderMock.h"
#include "DecryptionServiceMock.h"
#include "GenericPlayerContext.h"
#include "GlibWrapperMock.h"
#include "GstGenericPlayerClientMock.h"
#include "GstGenericPlayerPrivateMock.h"
#include "GstSrcMock.h"
#include "GstTextTrackSinkFactoryMock.h"
#include "GstWrapperMock.h"
#include "RdkGstreamerUtilsWrapperMock.h"
#include <memory>

/**
 * @brief GenericTasksTests context
 *
 * Stores all objects and non-const variables so that constuction and destruction can be managed.
 */
class GenericTasksTestsContext
{
public:
    firebolt::rialto::server::GenericPlayerContext m_context;

    // Mocks
    StrictMock<firebolt::rialto::server::GstGenericPlayerPrivateMock> m_gstPlayer;
    StrictMock<firebolt::rialto::server::GstGenericPlayerClientMock> m_gstPlayerClient;
    std::shared_ptr<firebolt::rialto::wrappers::GlibWrapperMock> m_glibWrapper{
        std::make_shared<StrictMock<firebolt::rialto::wrappers::GlibWrapperMock>>()};
    std::shared_ptr<firebolt::rialto::wrappers::GstWrapperMock> m_gstWrapper{
        std::make_shared<StrictMock<firebolt::rialto::wrappers::GstWrapperMock>>()};
    std::shared_ptr<firebolt::rialto::wrappers::RdkGstreamerUtilsWrapperMock> m_rdkGstreamerUtilsWrapper{
        std::make_shared<StrictMock<firebolt::rialto::wrappers::RdkGstreamerUtilsWrapperMock>>()};
    std::shared_ptr<StrictMock<firebolt::rialto::server::DecryptionServiceMock>> m_decryptionServiceMock{
        std::make_shared<StrictMock<firebolt::rialto::server::DecryptionServiceMock>>()};
    std::shared_ptr<StrictMock<firebolt::rialto::server::GstSrcMock>> m_gstSrc{
        std::make_shared<StrictMock<firebolt::rialto::server::GstSrcMock>>()};
    std::shared_ptr<StrictMock<firebolt::rialto::server::DataReaderMock>> m_dataReader{
        std::make_shared<StrictMock<firebolt::rialto::server::DataReaderMock>>()};
    std::shared_ptr<firebolt::rialto::server::GstTextTrackSinkFactoryMock> m_gstTextTrackSinkFactoryMock{
        std::make_shared<StrictMock<firebolt::rialto::server::GstTextTrackSinkFactoryMock>>()};

    // Gstreamer members
    GstElement *m_element{};
    GstElementFactory *m_elementFactory{};
    GstElement m_pipeline{};
    GstBuffer m_audioBuffer{};
    GstBuffer m_videoBuffer{};
    GstCaps m_gstCaps1{};
    GstCaps m_gstCaps2{};
    GstElement m_appSrcAudio{};
    GstElement m_appSrcVideo{};
    GstElement m_appSrcSubtitle{};
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
    GstEvent m_event2{};
    GstIterator m_iterator{};
    GstElement m_childElement{};
    GstQuery m_query{};
    GstElement m_textTrackSink{};

    // Glib members
    guint m_signals[1]{123};
    GCallback m_audioUnderflowCallback;
    GCallback m_videoUnderflowCallback;
    GCallback m_childAddedCallback;
    GCallback m_childRemovedCallback;
    gchar m_capsStr{};
    gchar m_videoStr[7]{"video/"};
    gchar m_audioStr[7]{"audio/"};
    gchar m_typefindElementName[9]{"typefind"};
    gchar m_parseElementName[6]{"parse"};
    gchar m_decoderElementName[10]{"decodebin"};
    gchar m_audioSinkElementName[10]{"audiosink"};
    gchar m_elementName[5]{"sink"};
    gchar m_binElementName[5]{"bin"};
    gchar m_xEac3Str[13]{"audio/x-eac3"};
    gpointer m_videoUserData{};
    gpointer m_audioUserData{};
    GValue m_value{};
    GParamSpec m_paramSpec{};
    GObject m_gObj{};

    // Standard members
    bool m_underflowEnabled{false};
    firebolt::rialto::server::StreamInfo m_streamInfoAudio{&m_appSrcAudio, true};
    firebolt::rialto::server::StreamInfo m_streamInfoVideo{&m_appSrcVideo, true};
};

#endif // GENERIC_TASKS_TESTS_CONTEXT_H_
