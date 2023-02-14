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

#include "tasks/generic/DeepElementAdded.h"
#include "GenericPlayerContext.h"
#include "GlibWrapperMock.h"
#include "GstGenericPlayerPrivateMock.h"
#include "GstWrapperMock.h"
#include "Matchers.h"
#include <gst/gst.h>
#include <gtest/gtest.h>
#include <memory>

using testing::_;
using testing::Invoke;
using testing::Return;
using testing::StrictMock;

class DeepElementAddedTest : public testing::Test
{
protected:
    firebolt::rialto::server::GenericPlayerContext m_context{};
    std::shared_ptr<firebolt::rialto::server::GlibWrapperMock> m_glibWrapper{
        std::make_shared<StrictMock<firebolt::rialto::server::GlibWrapperMock>>()};
    std::shared_ptr<firebolt::rialto::server::GstWrapperMock> m_gstWrapper{
        std::make_shared<StrictMock<firebolt::rialto::server::GstWrapperMock>>()};
    StrictMock<firebolt::rialto::server::GstGenericPlayerPrivateMock> m_gstPlayer;
    GstBin m_pipeline{};
    GstBin m_bin{};
    GstElement m_element{};
};

TEST_F(DeepElementAddedTest, shouldNotRegisterCallbackWhenPtrsAreNotEqual)
{
    GstObject obj1{}, obj2{};
    EXPECT_CALL(*m_gstWrapper, gstObjectParent(&m_element)).WillOnce(Return(&obj1));
    EXPECT_CALL(*m_gstWrapper, gstObjectCast(&m_bin)).WillOnce(Return(&obj2));
    EXPECT_CALL(*m_glibWrapper, gFree(nullptr));
    firebolt::rialto::server::tasks::generic::DeepElementAdded task{m_context,     m_gstPlayer, m_gstWrapper,
                                                                    m_glibWrapper, &m_pipeline, &m_bin,
                                                                    &m_element};
}

TEST_F(DeepElementAddedTest, shouldNotRegisterCallbackWhenElementIsNull)
{
    GstObject obj{};
    EXPECT_CALL(*m_gstWrapper, gstObjectParent(&m_element)).WillOnce(Return(&obj));
    EXPECT_CALL(*m_gstWrapper, gstObjectCast(&m_bin)).WillOnce(Return(&obj));
    EXPECT_CALL(*m_gstWrapper, gstElementGetName(&m_element)).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_glibWrapper, gFree(nullptr));
    firebolt::rialto::server::tasks::generic::DeepElementAdded task{m_context,     m_gstPlayer, m_gstWrapper,
                                                                    m_glibWrapper, &m_pipeline, &m_bin,
                                                                    &m_element};
}

TEST_F(DeepElementAddedTest, shouldNotRegisterCallbackWhenElementNameIsNotTypefind)
{
    GstObject obj{};
    gchar elementName[]{"sink"};
    EXPECT_CALL(*m_gstWrapper, gstObjectParent(&m_element)).WillOnce(Return(&obj));
    EXPECT_CALL(*m_gstWrapper, gstObjectCast(&m_bin)).WillOnce(Return(&obj));
    EXPECT_CALL(*m_gstWrapper, gstElementGetName(&m_element)).WillOnce(Return(elementName));
    EXPECT_CALL(*m_glibWrapper, gStrrstr(elementName, CharStrMatcher("typefind"))).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_glibWrapper, gFree(elementName));
    firebolt::rialto::server::tasks::generic::DeepElementAdded task{m_context,     m_gstPlayer, m_gstWrapper,
                                                                    m_glibWrapper, &m_pipeline, &m_bin,
                                                                    &m_element};
}

TEST_F(DeepElementAddedTest, shouldRegisterCallbackForTypefindElement)
{
    GstObject obj{};
    gchar elementName[]{"typefind"};
    gulong signalId{1};
    EXPECT_CALL(*m_gstWrapper, gstObjectParent(&m_element)).WillOnce(Return(&obj));
    EXPECT_CALL(*m_gstWrapper, gstObjectCast(&m_bin)).WillOnce(Return(&obj));
    EXPECT_CALL(*m_gstWrapper, gstElementGetName(&m_element)).WillOnce(Return(elementName));
    EXPECT_CALL(*m_glibWrapper, gStrrstr(elementName, CharStrMatcher("typefind"))).WillOnce(Return(elementName));
    EXPECT_CALL(*m_glibWrapper, gSignalConnect(G_OBJECT(&m_element), CharStrMatcher("have-type"), _, &m_gstPlayer))
        .WillOnce(Return(signalId));
    EXPECT_CALL(*m_glibWrapper, gFree(elementName));
    firebolt::rialto::server::tasks::generic::DeepElementAdded task{m_context,     m_gstPlayer, m_gstWrapper,
                                                                    m_glibWrapper, &m_pipeline, &m_bin,
                                                                    &m_element};
}

TEST_F(DeepElementAddedTest, shouldUpdatePlaybackGroupWhenCallbackIsCalled)
{
    GstObject obj{};
    gchar elementName[]{"typefind"};
    gulong signalId{1};
    GstCaps caps{};
    EXPECT_CALL(*m_gstWrapper, gstObjectParent(&m_element)).WillOnce(Return(&obj));
    EXPECT_CALL(*m_gstWrapper, gstObjectCast(&m_bin)).WillOnce(Return(&obj));
    EXPECT_CALL(*m_gstWrapper, gstElementGetName(&m_element)).WillOnce(Return(elementName));
    EXPECT_CALL(*m_glibWrapper, gStrrstr(elementName, CharStrMatcher("typefind"))).WillOnce(Return(elementName));
    EXPECT_CALL(*m_glibWrapper, gSignalConnect(G_OBJECT(&m_element), CharStrMatcher("have-type"), _, &m_gstPlayer))
        .WillOnce(Invoke(
            [&](gpointer instance, const gchar *detailed_signal, GCallback c_handler, gpointer data)
            {
                ((void (*)(GstElement *, guint, const GstCaps *, gpointer))c_handler)(&m_element, 0, &caps, &m_gstPlayer);
                return signalId;
            }));
    EXPECT_CALL(m_gstPlayer, updatePlaybackGroup(&m_element, &caps));
    EXPECT_CALL(*m_glibWrapper, gFree(elementName));
    firebolt::rialto::server::tasks::generic::DeepElementAdded task{m_context,     m_gstPlayer, m_gstWrapper,
                                                                    m_glibWrapper, &m_pipeline, &m_bin,
                                                                    &m_element};
}

TEST_F(DeepElementAddedTest, shouldAddSignalIdOfRegisteredCallbackToPlayerContext)
{
    GstObject obj{};
    gchar elementName[]{"typefind"};
    gulong signalId{1};
    EXPECT_CALL(*m_gstWrapper, gstObjectParent(&m_element)).WillOnce(Return(&obj));
    EXPECT_CALL(*m_gstWrapper, gstObjectCast(&m_bin)).WillRepeatedly(Return(&obj));
    EXPECT_CALL(*m_gstWrapper, gstObjectCast(nullptr)).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapper, gstElementGetName(&m_element)).WillOnce(Return(elementName));
    EXPECT_CALL(*m_glibWrapper, gStrrstr(elementName, CharStrMatcher("typefind"))).WillOnce(Return(elementName));
    EXPECT_CALL(*m_glibWrapper, gStrrstr(elementName, CharStrMatcher("audiosink"))).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_glibWrapper, gSignalConnect(G_OBJECT(&m_element), CharStrMatcher("have-type"), _, &m_gstPlayer))
        .WillOnce(Return(signalId));
    EXPECT_CALL(*m_glibWrapper, gFree(elementName));
    firebolt::rialto::server::tasks::generic::DeepElementAdded task{m_context,     m_gstPlayer, m_gstWrapper,
                                                                    m_glibWrapper, &m_pipeline, &m_bin,
                                                                    &m_element};
    task.execute();
    EXPECT_EQ(m_context.playbackGroup.m_gstPipeline, GST_ELEMENT(&m_pipeline));
    EXPECT_EQ(m_context.playbackGroup.m_curAudioTypefind, &m_element);
    ASSERT_NE(m_context.connectedSignals.find(&m_element), m_context.connectedSignals.end());
    EXPECT_EQ(m_context.connectedSignals[&m_element], signalId);
    EXPECT_EQ(m_context.playbackGroup.m_curAudioParse, nullptr);
    EXPECT_EQ(m_context.playbackGroup.m_curAudioDecoder, nullptr);
    EXPECT_EQ(m_context.playbackGroup.m_curAudioPlaysinkBin, nullptr);
}

TEST_F(DeepElementAddedTest, shouldAssignPipelineOnlyWhenElementNameIsNull)
{
    GstObject obj1{}, obj2{};
    EXPECT_CALL(*m_gstWrapper, gstObjectParent(&m_element)).WillOnce(Return(&obj1));
    EXPECT_CALL(*m_gstWrapper, gstObjectCast(&m_bin)).WillOnce(Return(&obj2));
    EXPECT_CALL(*m_glibWrapper, gFree(nullptr));
    firebolt::rialto::server::tasks::generic::DeepElementAdded task{m_context,     m_gstPlayer, m_gstWrapper,
                                                                    m_glibWrapper, &m_pipeline, &m_bin,
                                                                    &m_element};
    task.execute();
    EXPECT_EQ(m_context.playbackGroup.m_curAudioTypefind, nullptr);
    EXPECT_TRUE(m_context.connectedSignals.empty());
    EXPECT_EQ(m_context.playbackGroup.m_gstPipeline, GST_ELEMENT(&m_pipeline));
    EXPECT_EQ(m_context.playbackGroup.m_curAudioParse, nullptr);
    EXPECT_EQ(m_context.playbackGroup.m_curAudioDecoder, nullptr);
    EXPECT_EQ(m_context.playbackGroup.m_curAudioPlaysinkBin, nullptr);
}

TEST_F(DeepElementAddedTest, shouldDetectParseElement)
{
    GstObject obj{};
    gchar elementName[]{"parse"};
    GstElement audioDecodeBin;
    m_context.playbackGroup.m_curAudioDecodeBin = &audioDecodeBin;
    EXPECT_CALL(*m_gstWrapper, gstObjectParent(&m_element)).WillOnce(Return(&obj));
    EXPECT_CALL(*m_gstWrapper, gstObjectCast(&m_bin)).WillRepeatedly(Return(&obj));
    EXPECT_CALL(*m_gstWrapper, gstElementGetName(&m_element)).WillOnce(Return(elementName));
    EXPECT_CALL(*m_glibWrapper, gStrrstr(elementName, CharStrMatcher("typefind"))).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapper, gstObjectCast(&audioDecodeBin)).WillOnce(Return(&obj));
    EXPECT_CALL(*m_glibWrapper, gStrrstr(elementName, CharStrMatcher("parse"))).WillOnce(Return(elementName));
    EXPECT_CALL(*m_glibWrapper, gFree(elementName));
    firebolt::rialto::server::tasks::generic::DeepElementAdded task{m_context,     m_gstPlayer, m_gstWrapper,
                                                                    m_glibWrapper, &m_pipeline, &m_bin,
                                                                    &m_element};
    task.execute();
    EXPECT_EQ(m_context.playbackGroup.m_curAudioTypefind, nullptr);
    EXPECT_TRUE(m_context.connectedSignals.empty());
    EXPECT_EQ(m_context.playbackGroup.m_gstPipeline, GST_ELEMENT(&m_pipeline));
    EXPECT_EQ(m_context.playbackGroup.m_curAudioParse, &m_element);
    EXPECT_EQ(m_context.playbackGroup.m_curAudioDecoder, nullptr);
    EXPECT_EQ(m_context.playbackGroup.m_curAudioPlaysinkBin, nullptr);
}

TEST_F(DeepElementAddedTest, shouldDetectDecElement)
{
    GstObject obj{};
    gchar elementName[]{"dec"};
    GstElement audioDecodeBin;
    m_context.playbackGroup.m_curAudioDecodeBin = &audioDecodeBin;
    EXPECT_CALL(*m_gstWrapper, gstObjectParent(&m_element)).WillOnce(Return(&obj));
    EXPECT_CALL(*m_gstWrapper, gstObjectCast(&m_bin)).WillRepeatedly(Return(&obj));
    EXPECT_CALL(*m_gstWrapper, gstElementGetName(&m_element)).WillOnce(Return(elementName));
    EXPECT_CALL(*m_glibWrapper, gStrrstr(elementName, CharStrMatcher("typefind"))).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapper, gstObjectCast(&audioDecodeBin)).WillOnce(Return(&obj));
    EXPECT_CALL(*m_glibWrapper, gStrrstr(elementName, CharStrMatcher("parse"))).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_glibWrapper, gStrrstr(elementName, CharStrMatcher("dec"))).WillOnce(Return(elementName));
    EXPECT_CALL(*m_glibWrapper, gFree(elementName));
    firebolt::rialto::server::tasks::generic::DeepElementAdded task{m_context,     m_gstPlayer, m_gstWrapper,
                                                                    m_glibWrapper, &m_pipeline, &m_bin,
                                                                    &m_element};
    task.execute();
    EXPECT_EQ(m_context.playbackGroup.m_curAudioTypefind, nullptr);
    EXPECT_TRUE(m_context.connectedSignals.empty());
    EXPECT_EQ(m_context.playbackGroup.m_gstPipeline, GST_ELEMENT(&m_pipeline));
    EXPECT_EQ(m_context.playbackGroup.m_curAudioParse, nullptr);
    EXPECT_EQ(m_context.playbackGroup.m_curAudioDecoder, &m_element);
    EXPECT_EQ(m_context.playbackGroup.m_curAudioPlaysinkBin, nullptr);
}

TEST_F(DeepElementAddedTest, shouldDoNothingForNotHandledElementName)
{
    GstObject obj{};
    gchar elementName[]{"playbin"};
    EXPECT_CALL(*m_gstWrapper, gstObjectParent(&m_element)).WillOnce(Return(&obj));
    EXPECT_CALL(*m_gstWrapper, gstObjectCast(&m_bin)).WillRepeatedly(Return(&obj));
    EXPECT_CALL(*m_gstWrapper, gstElementGetName(&m_element)).WillOnce(Return(elementName));
    EXPECT_CALL(*m_glibWrapper, gStrrstr(elementName, CharStrMatcher("typefind"))).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapper, gstObjectCast(nullptr)).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_glibWrapper, gStrrstr(elementName, CharStrMatcher("audiosink"))).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_glibWrapper, gFree(elementName));
    firebolt::rialto::server::tasks::generic::DeepElementAdded task{m_context,     m_gstPlayer, m_gstWrapper,
                                                                    m_glibWrapper, &m_pipeline, &m_bin,
                                                                    &m_element};
    task.execute();
    EXPECT_EQ(m_context.playbackGroup.m_curAudioTypefind, nullptr);
    EXPECT_TRUE(m_context.connectedSignals.empty());
    EXPECT_EQ(m_context.playbackGroup.m_gstPipeline, GST_ELEMENT(&m_pipeline));
    EXPECT_EQ(m_context.playbackGroup.m_curAudioParse, nullptr);
    EXPECT_EQ(m_context.playbackGroup.m_curAudioDecoder, nullptr);
    EXPECT_EQ(m_context.playbackGroup.m_curAudioPlaysinkBin, nullptr);
}

TEST_F(DeepElementAddedTest, shouldDoNothingWhenAudiosinkParentNameIsNull)
{
    GstObject obj{};
    gchar elementName[]{"audiosink"};
    GstElement audioSinkParent{};
    EXPECT_CALL(*m_gstWrapper, gstObjectParent(&m_element)).WillOnce(Return(&obj));
    EXPECT_CALL(*m_gstWrapper, gstObjectCast(&m_bin)).WillRepeatedly(Return(&obj));
    EXPECT_CALL(*m_gstWrapper, gstElementGetName(&m_element)).WillOnce(Return(elementName));
    EXPECT_CALL(*m_glibWrapper, gStrrstr(elementName, CharStrMatcher("typefind"))).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapper, gstObjectCast(nullptr)).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_glibWrapper, gStrrstr(elementName, CharStrMatcher("audiosink"))).WillOnce(Return(elementName));
    EXPECT_CALL(*m_gstWrapper, gstElementGetParent(&m_element)).WillOnce(Return(GST_OBJECT(&audioSinkParent)));
    EXPECT_CALL(*m_gstWrapper, gstElementGetName(&audioSinkParent)).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_glibWrapper, gFree(nullptr));
    EXPECT_CALL(*m_glibWrapper, gFree(elementName));
    firebolt::rialto::server::tasks::generic::DeepElementAdded task{m_context,     m_gstPlayer, m_gstWrapper,
                                                                    m_glibWrapper, &m_pipeline, &m_bin,
                                                                    &m_element};
    task.execute();
    EXPECT_EQ(m_context.playbackGroup.m_curAudioTypefind, nullptr);
    EXPECT_TRUE(m_context.connectedSignals.empty());
    EXPECT_EQ(m_context.playbackGroup.m_gstPipeline, GST_ELEMENT(&m_pipeline));
    EXPECT_EQ(m_context.playbackGroup.m_curAudioParse, nullptr);
    EXPECT_EQ(m_context.playbackGroup.m_curAudioDecoder, nullptr);
    EXPECT_EQ(m_context.playbackGroup.m_curAudioPlaysinkBin, nullptr);
}

TEST_F(DeepElementAddedTest, shouldDoNothingWhenAudiosinkParentIsNotBin)
{
    GstObject obj{};
    gchar elementName[]{"audiosink"};
    GstElement audioSinkParent{};
    gchar audioSinkParentName[]{"audiosink_parent"};
    EXPECT_CALL(*m_gstWrapper, gstObjectParent(&m_element)).WillOnce(Return(&obj));
    EXPECT_CALL(*m_gstWrapper, gstObjectCast(&m_bin)).WillRepeatedly(Return(&obj));
    EXPECT_CALL(*m_gstWrapper, gstElementGetName(&m_element)).WillOnce(Return(elementName));
    EXPECT_CALL(*m_glibWrapper, gStrrstr(elementName, CharStrMatcher("typefind"))).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapper, gstObjectCast(nullptr)).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_glibWrapper, gStrrstr(elementName, CharStrMatcher("audiosink"))).WillOnce(Return(elementName));
    EXPECT_CALL(*m_gstWrapper, gstElementGetParent(&m_element)).WillOnce(Return(GST_OBJECT(&audioSinkParent)));
    EXPECT_CALL(*m_gstWrapper, gstElementGetName(&audioSinkParent)).WillOnce(Return(audioSinkParentName));
    EXPECT_CALL(*m_glibWrapper, gStrrstr(audioSinkParentName, CharStrMatcher("bin"))).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_glibWrapper, gFree(audioSinkParentName));
    EXPECT_CALL(*m_glibWrapper, gFree(elementName));
    firebolt::rialto::server::tasks::generic::DeepElementAdded task{m_context,     m_gstPlayer, m_gstWrapper,
                                                                    m_glibWrapper, &m_pipeline, &m_bin,
                                                                    &m_element};
    task.execute();
    EXPECT_EQ(m_context.playbackGroup.m_curAudioTypefind, nullptr);
    EXPECT_TRUE(m_context.connectedSignals.empty());
    EXPECT_EQ(m_context.playbackGroup.m_gstPipeline, GST_ELEMENT(&m_pipeline));
    EXPECT_EQ(m_context.playbackGroup.m_curAudioParse, nullptr);
    EXPECT_EQ(m_context.playbackGroup.m_curAudioDecoder, nullptr);
    EXPECT_EQ(m_context.playbackGroup.m_curAudioPlaysinkBin, nullptr);
}

TEST_F(DeepElementAddedTest, shouldFindAudioSinkBin)
{
    GstObject obj{};
    gchar elementName[]{"audiosink"};
    GstElement audioSinkParent{};
    gchar audioSinkParentName[]{"audiosinkbin"};
    EXPECT_CALL(*m_gstWrapper, gstObjectParent(&m_element)).WillOnce(Return(&obj));
    EXPECT_CALL(*m_gstWrapper, gstObjectCast(&m_bin)).WillRepeatedly(Return(&obj));
    EXPECT_CALL(*m_gstWrapper, gstElementGetName(&m_element)).WillOnce(Return(elementName));
    EXPECT_CALL(*m_glibWrapper, gStrrstr(elementName, CharStrMatcher("typefind"))).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapper, gstObjectCast(nullptr)).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_glibWrapper, gStrrstr(elementName, CharStrMatcher("audiosink"))).WillOnce(Return(elementName));
    EXPECT_CALL(*m_gstWrapper, gstElementGetParent(&m_element)).WillOnce(Return(GST_OBJECT(&audioSinkParent)));
    EXPECT_CALL(*m_gstWrapper, gstElementGetName(&audioSinkParent)).WillOnce(Return(audioSinkParentName));
    EXPECT_CALL(*m_glibWrapper, gStrrstr(audioSinkParentName, CharStrMatcher("bin"))).WillOnce(Return(audioSinkParentName));
    EXPECT_CALL(*m_glibWrapper, gFree(audioSinkParentName));
    EXPECT_CALL(*m_glibWrapper, gFree(elementName));
    firebolt::rialto::server::tasks::generic::DeepElementAdded task{m_context,     m_gstPlayer, m_gstWrapper,
                                                                    m_glibWrapper, &m_pipeline, &m_bin,
                                                                    &m_element};
    task.execute();
    EXPECT_EQ(m_context.playbackGroup.m_curAudioTypefind, nullptr);
    EXPECT_TRUE(m_context.connectedSignals.empty());
    EXPECT_EQ(m_context.playbackGroup.m_gstPipeline, GST_ELEMENT(&m_pipeline));
    EXPECT_EQ(m_context.playbackGroup.m_curAudioParse, nullptr);
    EXPECT_EQ(m_context.playbackGroup.m_curAudioDecoder, nullptr);
    EXPECT_EQ(m_context.playbackGroup.m_curAudioPlaysinkBin, &audioSinkParent);
}
