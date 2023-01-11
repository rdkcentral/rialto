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

#include "GstWebAudioPlayerTestCommon.h"
#include "WebAudioPlayerTaskMock.h"
#include "Matchers.h"
#include <memory>
#include <utility>

using ::testing::_;
using ::testing::ByMove;
using ::testing::Invoke;
using ::testing::Return;

void GstWebAudioPlayerTestCommon::gstPlayerWillBeCreatedForLlama()
{
    expectCreatePipeline();
    expectInitAppSrc();
    expectAddElementsAmlhalaSink();

    EXPECT_CALL(*m_gstSrcMock, initSrc());
    EXPECT_CALL(m_workerThreadFactoryMock, createWorkerThread()).WillOnce(Return(ByMove(std::move(workerThread))));
    executeTaskWhenEnqueued();
}

void GstWebAudioPlayerTestCommon::gstPlayerWillBeCreatedForXiOne()
{
    expectCreatePipeline();
    expectInitAppSrc();
    expectAddElementsRtkAudioSink();

    EXPECT_CALL(*m_gstSrcMock, initSrc());
    EXPECT_CALL(m_workerThreadFactoryMock, createWorkerThread()).WillOnce(Return(ByMove(std::move(workerThread))));
    executeTaskWhenEnqueued();
}

void GstWebAudioPlayerTestCommon::gstPlayerWillBeCreatedForGenericPlatform()
{
    expectCreatePipeline();
    expectInitAppSrc();
    expectAddElementsAutoAudioSink();

    EXPECT_CALL(*m_gstSrcMock, initSrc());
    EXPECT_CALL(m_workerThreadFactoryMock, createWorkerThread()).WillOnce(Return(ByMove(std::move(workerThread))));
    executeTaskWhenEnqueued();
}

void GstWebAudioPlayerTestCommon::gstPlayerWillBeDestroyed()
{
    std::unique_ptr<IPlayerTask> stopTask{std::make_unique<StrictMock<WebAudioPlayerTaskMock>>()};
    std::unique_ptr<IPlayerTask> shutdownTask{std::make_unique<StrictMock<WebAudioPlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<WebAudioPlayerTaskMock> &>(*stopTask), execute());
    EXPECT_CALL(dynamic_cast<StrictMock<WebAudioPlayerTaskMock> &>(*shutdownTask), execute());
    EXPECT_CALL(m_taskFactoryMock, createShutdown(_)).WillOnce(Return(ByMove(std::move(shutdownTask))));
    EXPECT_CALL(m_workerThreadMock, join());
    EXPECT_CALL(m_taskFactoryMock, createStop(_, _)).WillOnce(Return(ByMove(std::move(stopTask))));
    EXPECT_CALL(*m_gstWrapperMock, gstPipelineGetBus(GST_PIPELINE(&m_pipeline))).WillOnce(Return(&m_bus));
    EXPECT_CALL(*m_gstWrapperMock, gstBusSetSyncHandler(&m_bus, nullptr, nullptr, nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_bus));
    EXPECT_CALL(*m_glibWrapperMock, gObjectUnref(&m_pipeline));
}

void GstWebAudioPlayerTestCommon::executeTaskWhenEnqueued()
{
    // It's hard to match std::unique_ptr<IPlayerTask> &&, so we will just execute task, when it's enqueued to check
    // if proper task was enqueued (EXPECT_CALL(task, execute())) has to be added for each task, which is expected to
    // be enqueued)
    EXPECT_CALL(m_workerThreadMock, enqueueTask(_))
        .WillRepeatedly(Invoke([](std::unique_ptr<IPlayerTask> &&task) { task->execute(); }));
}

void GstWebAudioPlayerTestCommon::expectCreatePipeline()
{
    EXPECT_CALL(*m_gstWrapperMock, gstPipelineNew(CharStrMatcher("webaudiopipeline")))
        .WillOnce(Return(&m_pipeline));
}

void GstWebAudioPlayerTestCommon::expectInitAppSrc()
{
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(CharStrMatcher("appsrc"), CharStrMatcher("audsrc")))
        .WillOnce(Return(&m_appSrc));
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetMaxBytes(GST_APP_SRC(&m_appSrc), 10 * 1024));
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(G_OBJECT(&m_appSrc), CharStrMatcher("format")));
}

void GstWebAudioPlayerTestCommon::expectAddElementsAmlhalaSink()
{
    EXPECT_CALL(*m_gstWrapperMock, gstRegistryGet())
        .WillOnce(Return(&m_reg));
    EXPECT_CALL(*m_gstWrapperMock, gstRegistryLookupFeature(&m_reg, CharStrMatcher("amlhalasink")))
        .WillOnce(Return(&m_feature));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(CharStrMatcher("amlhalasink"), CharStrMatcher("webaudiosink")))
        .WillOnce(Return(&m_sink));
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(G_OBJECT(&m_sink), CharStrMatcher("direct-mode")));
    EXPECT_CALL(*m_gstWrapperMock, gstBinAddManyStub(GST_BIN(&m_pipeline), &m_appSrc));
    EXPECT_CALL(*m_gstWrapperMock, gstBinAddManyStub(GST_BIN(&m_pipeline), &m_sink));
    EXPECT_CALL(*m_gstWrapperMock, gstElementLinkMany(&m_appSrc, &m_sink));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_feature));
}

void GstWebAudioPlayerTestCommon::expectAddElementsRtkAudioSink()
{
    EXPECT_CALL(*m_gstWrapperMock, gstRegistryGet())
        .WillOnce(Return(&m_reg));
    EXPECT_CALL(*m_gstWrapperMock, gstRegistryLookupFeature(&m_reg, CharStrMatcher("amlhalasink")))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstRegistryLookupFeature(&m_reg, CharStrMatcher("rtkaudiosink")))
        .WillOnce(Return(&m_feature));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(CharStrMatcher("rtkaudiosink"), CharStrMatcher("webaudiosink")))
        .WillOnce(Return(&m_sink));
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(G_OBJECT(&m_sink), CharStrMatcher("media-tunnel")));
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(G_OBJECT(&m_sink), CharStrMatcher("audio-service")));

    GstElement convert{};
    GstElement resample{};
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(CharStrMatcher("audioconvert"),_));
        .WillOnce(Return(&convert));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(CharStrMatcher("audioresample"),_));
        .WillOnce(Return(&resample));

    EXPECT_CALL(*m_gstWrapperMock, gstBinAddManyStub(GST_BIN(&m_pipeline), &m_appSrc));
    EXPECT_CALL(*m_gstWrapperMock, gstBinAddManyStub(GST_BIN(&m_pipeline), &convert));
    EXPECT_CALL(*m_gstWrapperMock, gstBinAddManyStub(GST_BIN(&m_pipeline), &resample));
    EXPECT_CALL(*m_gstWrapperMock, gstBinAddManyStub(GST_BIN(&m_pipeline), &sink));
    EXPECT_CALL(*m_gstWrapperMock, gstElementLinkMany(&m_appSrc, &convert));
    EXPECT_CALL(*m_gstWrapperMock, gstElementLinkMany(&convert, &resample));
    EXPECT_CALL(*m_gstWrapperMock, gstElementLinkMany(&resample, &sink));

    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_feature));
}

void GstWebAudioPlayerTestCommon::expectAddElementsAutoAudioSink()
{
    EXPECT_CALL(*m_gstWrapperMock, gstRegistryGet())
        .WillOnce(Return(&m_reg));
    EXPECT_CALL(*m_gstWrapperMock, gstRegistryLookupFeature(&m_reg, CharStrMatcher("amlhalasink")))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstRegistryLookupFeature(&m_reg, CharStrMatcher("rtkaudiosink")))
        .WillOnce(Return(nullptr));

    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(CharStrMatcher("autoaudiosink"), CharStrMatcher("webaudiosink")))
        .WillOnce(Return(&m_sink));
    EXPECT_CALL(*m_gstWrapperMock, gstBinAddManyStub(GST_BIN(&m_pipeline), &m_appSrc));
    EXPECT_CALL(*m_gstWrapperMock, gstBinAddManyStub(GST_BIN(&m_pipeline), &m_sink));
    EXPECT_CALL(*m_gstWrapperMock, gstElementLinkMany(&m_appSrc, &m_sink));
}
