/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Sky UK
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

#include "GstGenericPlayerTestCommon.h"
#include "Matchers.h"
#include "PlayerTaskMock.h"
#include <memory>
#include <string>
#include <utility>

using ::testing::_;
using ::testing::ByMove;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::StrEq;

void GstGenericPlayerTestCommon::gstPlayerWillBeCreated()
{
    EXPECT_CALL(m_gstInitialiserMock, waitForInitialisation());
    initFactories();
    expectMakePlaybin();
    expectSetFlags();
    expectSetSignalCallbacks();
    expectSetUri();
    expectCheckPlaySink();
    expectSetMessageCallback();

    EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(&m_pipeline, GST_STATE_READY))
        .WillOnce(Return(GST_STATE_CHANGE_SUCCESS));
    EXPECT_CALL(*m_gstSrcMock, initSrc());
    EXPECT_CALL(m_workerThreadFactoryMock, createWorkerThread()).WillOnce(Return(ByMove(std::move(workerThread))));
    EXPECT_CALL(*m_gstProtectionMetadataFactoryMock, createProtectionMetadataWrapper(_))
        .WillOnce(Return(ByMove(std::move(m_gstProtectionMetadataWrapper))));
    executeTaskWhenEnqueued();
}

void GstGenericPlayerTestCommon::gstPlayerWillBeDestroyed()
{
    expectShutdown();
    expectStop();
    EXPECT_CALL(*m_gstWrapperMock, gstPipelineGetBus(GST_PIPELINE(&m_pipeline))).WillOnce(Return(&m_bus));
    EXPECT_CALL(*m_gstWrapperMock, gstBusSetSyncHandler(&m_bus, nullptr, nullptr, nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_bus));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_pipeline));
}

void GstGenericPlayerTestCommon::expectShutdown()
{
    std::unique_ptr<IPlayerTask> shutdownTask{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*shutdownTask), execute());
    EXPECT_CALL(m_taskFactoryMock, createShutdown(_)).WillOnce(Return(ByMove(std::move(shutdownTask))));
    EXPECT_CALL(m_workerThreadMock, join());
}

void GstGenericPlayerTestCommon::expectStop()
{
    std::unique_ptr<IPlayerTask> stopTask{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*stopTask), execute());
    EXPECT_CALL(m_taskFactoryMock, createStop(_, _)).WillOnce(Return(ByMove(std::move(stopTask))));
}

void GstGenericPlayerTestCommon::executeTaskWhenEnqueued()
{
    // It's hard to match std::unique_ptr<IPlayerTask> &&, so we will just execute task, when it's enqueued to check
    // if proper task was enqueued (EXPECT_CALL(task, execute())) has to be added for each task, which is expected to
    // be enqueued)
    EXPECT_CALL(m_workerThreadMock, enqueueTask(_))
        .WillRepeatedly(Invoke([](std::unique_ptr<IPlayerTask> &&task) { task->execute(); }));
}

void GstGenericPlayerTestCommon::triggerSetupSource(GstElement *element)
{
    ASSERT_TRUE(m_setupSourceFunc);
    reinterpret_cast<void (*)(GstElement *, GstElement *, GstGenericPlayer *)>(
        m_setupSourceFunc)(&m_pipeline, element, reinterpret_cast<GstGenericPlayer *>(m_setupSourceUserData));
}

void GstGenericPlayerTestCommon::triggerSetupElement(GstElement *element)
{
    ASSERT_TRUE(m_setupElementFunc);
    reinterpret_cast<void (*)(GstElement *, GstElement *, GstGenericPlayer *)>(
        m_setupElementFunc)(&m_pipeline, element, reinterpret_cast<GstGenericPlayer *>(m_setupElementUserData));
}

void GstGenericPlayerTestCommon::triggerDeepElementAdded(GstElement *element)
{
    ASSERT_TRUE(m_deepElementAddedFunc);
    reinterpret_cast<void (*)(GstBin *pipeline, GstBin *bin, GstElement *element, GstGenericPlayer *self)>(
        m_deepElementAddedFunc)(&m_bin, &m_bin, element, reinterpret_cast<GstGenericPlayer *>(m_setupElementUserData));
}

void GstGenericPlayerTestCommon::setPipelineState(const GstState &state)
{
    GST_STATE(&m_pipeline) = state;
}

void GstGenericPlayerTestCommon::initFactories()
{
    EXPECT_CALL(*m_gstSrcFactoryMock, getGstSrc()).WillOnce(Return(m_gstSrcMock));
}

void GstGenericPlayerTestCommon::expectMakePlaybin()
{
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(StrEq("playbin"), _)).WillOnce(Return(&m_pipeline));
}

void GstGenericPlayerTestCommon::expectSetFlags()
{
    EXPECT_CALL(*m_glibWrapperMock, gTypeFromName(StrEq("GstPlayFlags"))).Times(4).WillRepeatedly(Return(m_gstPlayFlagsType));
    EXPECT_CALL(*m_glibWrapperMock, gTypeClassRef(m_gstPlayFlagsType)).Times(4).WillRepeatedly(Return(&m_flagsClass));

    EXPECT_CALL(*m_glibWrapperMock, gFlagsGetValueByNick(&m_flagsClass, StrEq("audio"))).WillOnce(Return(&m_audioFlag));
    EXPECT_CALL(*m_glibWrapperMock, gFlagsGetValueByNick(&m_flagsClass, StrEq("video"))).WillOnce(Return(&m_videoFlag));
    EXPECT_CALL(*m_glibWrapperMock, gFlagsGetValueByNick(&m_flagsClass, StrEq("text"))).WillOnce(Return(&m_subtitleFlag));
    EXPECT_CALL(*m_glibWrapperMock, gFlagsGetValueByNick(&m_flagsClass, StrEq("native-video")))
        .WillOnce(Return(&m_nativeVideoFlag));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryFind(StrEq("brcmaudiosink"))).WillOnce(Return(nullptr));

    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_pipeline, StrEq("flags")));
}

void GstGenericPlayerTestCommon::expectSetFlagsWithNativeAudio()
{
    EXPECT_CALL(*m_glibWrapperMock, gTypeFromName(StrEq("GstPlayFlags"))).Times(5).WillRepeatedly(Return(m_gstPlayFlagsType));
    EXPECT_CALL(*m_glibWrapperMock, gTypeClassRef(m_gstPlayFlagsType)).Times(5).WillRepeatedly(Return(&m_flagsClass));

    EXPECT_CALL(*m_glibWrapperMock, gFlagsGetValueByNick(&m_flagsClass, StrEq("audio"))).WillOnce(Return(&m_audioFlag));
    EXPECT_CALL(*m_glibWrapperMock, gFlagsGetValueByNick(&m_flagsClass, StrEq("video"))).WillOnce(Return(&m_videoFlag));
    EXPECT_CALL(*m_glibWrapperMock, gFlagsGetValueByNick(&m_flagsClass, StrEq("text"))).WillOnce(Return(&m_subtitleFlag));
    EXPECT_CALL(*m_glibWrapperMock, gFlagsGetValueByNick(&m_flagsClass, StrEq("native-video")))
        .WillOnce(Return(&m_nativeVideoFlag));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryFind(StrEq("brcmaudiosink")))
        .WillOnce(Return(reinterpret_cast<GstElementFactory *>(&m_sinkFactory)));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(reinterpret_cast<GstElementFactory *>(&m_sinkFactory)));
    EXPECT_CALL(*m_glibWrapperMock, gFlagsGetValueByNick(&m_flagsClass, StrEq("native-audio")))
        .WillOnce(Return(&m_nativeAudioFlag));

    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_pipeline, StrEq("flags")));
}

void GstGenericPlayerTestCommon::expectSetSignalCallbacks()
{
    EXPECT_CALL(*m_glibWrapperMock,
                gSignalConnect(&m_pipeline, StrEq("source-setup"), NotNullMatcher(), NotNullMatcher()))
        .WillOnce(Invoke(
            [this](gpointer instance, const gchar *detailed_signal, GCallback c_handler, gpointer data)
            {
                m_setupSourceFunc = c_handler;
                m_setupSourceUserData = data;
                return m_setupSourceSignalId;
            }));
    EXPECT_CALL(*m_glibWrapperMock,
                gSignalConnect(&m_pipeline, StrEq("element-setup"), NotNullMatcher(), NotNullMatcher()))
        .WillOnce(Invoke(
            [this](gpointer instance, const gchar *detailed_signal, GCallback c_handler, gpointer data)
            {
                m_setupElementFunc = c_handler;
                m_setupElementUserData = data;
                return m_setupElementSignalId;
            }));
    EXPECT_CALL(*m_glibWrapperMock,
                gSignalConnect(&m_pipeline, StrEq("deep-element-added"), NotNullMatcher(), NotNullMatcher()))
        .WillOnce(Invoke(
            [this](gpointer instance, const gchar *detailed_signal, GCallback c_handler, gpointer data)
            {
                m_deepElementAddedFunc = c_handler;
                m_deepElementAddedUserData = data;
                return m_deepElementAddedSignalId;
            }));
}

void GstGenericPlayerTestCommon::expectSetUri()
{
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_pipeline, StrEq("uri")));
}

void GstGenericPlayerTestCommon::expectCheckPlaySink()
{
    EXPECT_CALL(*m_gstWrapperMock, gstBinGetByName(GST_BIN(&m_pipeline), StrEq("playsink"))).WillOnce(Return(&m_playsink));
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_playsink, StrEq("send-event-mode")));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_playsink));
}

void GstGenericPlayerTestCommon::expectSetMessageCallback()
{
    EXPECT_CALL(m_gstDispatcherThreadFactoryMock, createGstDispatcherThread(_, _, _, _))
        .WillOnce(Return(ByMove(std::move(gstDispatcherThread))));
}

void GstGenericPlayerTestCommon::expectGetDecoder(GstElement *element)
{
    EXPECT_CALL(*m_gstWrapperMock, gstBinIterateRecurse(GST_BIN(&m_pipeline))).WillOnce(Return(&m_it));
    EXPECT_CALL(*m_gstWrapperMock, gstIteratorNext(&m_it, _)).WillOnce(Return(GST_ITERATOR_OK));
    EXPECT_CALL(*m_glibWrapperMock, gValueGetObject(_)).WillOnce(Return(element));
    EXPECT_CALL(*m_gstWrapperMock, gstElementGetFactory(element)).WillOnce(Return(m_factory));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListIsType(m_factory, (GST_ELEMENT_FACTORY_TYPE_DECODER |
                                                                           GST_ELEMENT_FACTORY_TYPE_MEDIA_AUDIO)))
        .WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectRef(element)).WillOnce(Return(element));
    EXPECT_CALL(*m_glibWrapperMock, gValueUnset(_));
    EXPECT_CALL(*m_gstWrapperMock, gstIteratorFree(&m_it));
}

void GstGenericPlayerTestCommon::expectGetVideoDecoder(GstElement *element)
{
    EXPECT_CALL(*m_gstWrapperMock, gstBinIterateRecurse(GST_BIN(&m_pipeline))).WillOnce(Return(&m_it));
    EXPECT_CALL(*m_gstWrapperMock, gstIteratorNext(&m_it, _)).WillOnce(Return(GST_ITERATOR_OK));
    EXPECT_CALL(*m_glibWrapperMock, gValueGetObject(_)).WillOnce(Return(element));
    EXPECT_CALL(*m_gstWrapperMock, gstElementGetFactory(element)).WillOnce(Return(m_factory));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListIsType(m_factory, (GST_ELEMENT_FACTORY_TYPE_DECODER |
                                                                           GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO)))
        .WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectRef(element)).WillOnce(Return(element));
    EXPECT_CALL(*m_glibWrapperMock, gValueUnset(_));
    EXPECT_CALL(*m_gstWrapperMock, gstIteratorFree(&m_it));
}

void GstGenericPlayerTestCommon::expectGetVideoParser(GstElement *element)
{
    EXPECT_CALL(*m_gstWrapperMock, gstBinIterateRecurse(GST_BIN(&m_pipeline))).WillOnce(Return(&m_it));
    EXPECT_CALL(*m_gstWrapperMock, gstIteratorNext(&m_it, _)).WillOnce(Return(GST_ITERATOR_OK));
    EXPECT_CALL(*m_glibWrapperMock, gValueGetObject(_)).WillOnce(Return(element));
    EXPECT_CALL(*m_gstWrapperMock, gstElementGetFactory(element)).WillOnce(Return(m_factory));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryListIsType(m_factory, (GST_ELEMENT_FACTORY_TYPE_PARSER |
                                                                           GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO)))
        .WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectRef(element)).WillOnce(Return(element));
    EXPECT_CALL(*m_glibWrapperMock, gValueUnset(_));
    EXPECT_CALL(*m_gstWrapperMock, gstIteratorFree(&m_it));
}

void GstGenericPlayerTestCommon::expectGetSink(const std::string &sinkName, GstElement *elementObj)
{
    EXPECT_CALL(*m_glibWrapperMock, gObjectGetStub(_, StrEq(sinkName.c_str()), _))
        .WillOnce(Invoke(
            [elementObj](gpointer object, const gchar *first_property_name, void *element)
            {
                GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                *elementPtr = elementObj;
            }));
    EXPECT_CALL(*m_glibWrapperMock, gTypeName(G_OBJECT_TYPE(elementObj))).WillOnce(Return(kElementTypeName.c_str()));
}

void GstGenericPlayerTestCommon::expectNoDecoder()
{
    EXPECT_CALL(*m_gstWrapperMock, gstBinIterateRecurse(GST_BIN(&m_pipeline))).WillOnce(Return(&m_it));
    EXPECT_CALL(*m_gstWrapperMock, gstIteratorNext(&m_it, _)).WillOnce(Return(GST_ITERATOR_DONE));
    EXPECT_CALL(*m_glibWrapperMock, gValueUnset(_));
    EXPECT_CALL(*m_gstWrapperMock, gstIteratorFree(&m_it));
}

void GstGenericPlayerTestCommon::expectNoParser()
{
    expectNoDecoder();
}
