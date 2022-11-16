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

#include "GstPlayerTestCommon.h"
#include "Matchers.h"
#include "PlayerTaskMock.h"
#include <memory>
#include <utility>

using ::testing::_;
using ::testing::ByMove;
using ::testing::Invoke;
using ::testing::Return;

void GstPlayerTestCommon::gstPlayerWillBeCreated()
{
    EXPECT_CALL(*m_gstSrcFactoryMock, getGstSrc()).WillOnce(Return(m_gstSrcMock));
    EXPECT_CALL(*m_gstSrcMock, initSrc());
    EXPECT_CALL(m_workerThreadFactoryMock, createWorkerThread()).WillOnce(Return(ByMove(std::move(workerThread))));
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
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_pipeline, CharStrMatcher("flags")));
    EXPECT_CALL(*m_glibWrapperMock,
                gSignalConnect(&m_pipeline, CharStrMatcher("source-setup"), NotNullMatcher(), NotNullMatcher()))
        .WillOnce(Invoke(
            [this](gpointer instance, const gchar *detailed_signal, GCallback c_handler, gpointer data)
            {
                m_setupSourceFunc = c_handler;
                m_setupSourceUserData = data;
            }));
    EXPECT_CALL(*m_glibWrapperMock,
                gSignalConnect(&m_pipeline, CharStrMatcher("element-setup"), NotNullMatcher(), NotNullMatcher()))
        .WillOnce(Invoke(
            [this](gpointer instance, const gchar *detailed_signal, GCallback c_handler, gpointer data)
            {
                m_setupElementFunc = c_handler;
                m_setupElementUserData = data;
            }));
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_pipeline, CharStrMatcher("uri")));
    EXPECT_CALL(*m_gstWrapperMock, gstBinGetByName(GST_BIN(&m_pipeline), CharStrMatcher("playsink")))
        .WillOnce(Return(&m_playsink));
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_playsink, CharStrMatcher("send-event-mode")));
    EXPECT_CALL(*m_glibWrapperMock, gObjectUnref(&m_playsink));
    EXPECT_CALL(m_gstDispatcherThreadFactoryMock, createGstDispatcherThread(_, _, _, _, _))
        .WillOnce(Return(ByMove(std::move(gstDispatcherThread))));
    executeTaskWhenEnqueued();
}

void GstPlayerTestCommon::gstPlayerWillBeDestroyed()
{
    std::unique_ptr<IPlayerTask> stopTask{std::make_unique<StrictMock<PlayerTaskMock>>()};
    std::unique_ptr<IPlayerTask> shutdownTask{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*stopTask), execute());
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*shutdownTask), execute());
    EXPECT_CALL(m_taskFactoryMock, createShutdown(_)).WillOnce(Return(ByMove(std::move(shutdownTask))));
    EXPECT_CALL(m_workerThreadMock, join());
    EXPECT_CALL(m_taskFactoryMock, createStop(_, _)).WillOnce(Return(ByMove(std::move(stopTask))));
    EXPECT_CALL(*m_gstWrapperMock, gstPipelineGetBus(GST_PIPELINE(&m_pipeline))).WillOnce(Return(&m_bus));
    EXPECT_CALL(*m_gstWrapperMock, gstBusSetSyncHandler(&m_bus, nullptr, nullptr, nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_bus));
    EXPECT_CALL(*m_glibWrapperMock, gObjectUnref(&m_pipeline));
}

void GstPlayerTestCommon::executeTaskWhenEnqueued()
{
    // It's hard to match std::unique_ptr<IPlayerTask> &&, so we will just execute task, when it's enqueued to check
    // if proper task was enqueued (EXPECT_CALL(task, execute())) has to be added for each task, which is expected to
    // be enqueued)
    EXPECT_CALL(m_workerThreadMock, enqueueTask(_))
        .WillRepeatedly(Invoke([](std::unique_ptr<IPlayerTask> &&task) { task->execute(); }));
}

void GstPlayerTestCommon::triggerSetupSource(GstElement *element)
{
    ASSERT_TRUE(m_setupSourceFunc);
    ((void (*)(GstElement *, GstElement *, GstPlayer *))m_setupSourceFunc)(&m_pipeline, element,
                                                                           reinterpret_cast<GstPlayer *>(
                                                                               m_setupSourceUserData));
}

void GstPlayerTestCommon::triggerSetupElement(GstElement *element)
{
    ASSERT_TRUE(m_setupElementFunc);
    ((void (*)(GstElement *, GstElement *, GstPlayer *))m_setupElementFunc)(&m_pipeline, element,
                                                                            reinterpret_cast<GstPlayer *>(
                                                                                m_setupElementUserData));
}

void GstPlayerTestCommon::setPipelineState(const GstState &state)
{
    GST_STATE(&m_pipeline) = state;
}
