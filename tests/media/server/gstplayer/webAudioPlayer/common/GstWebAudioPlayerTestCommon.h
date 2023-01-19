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

#ifndef GST_WEB_AUDIO_PLAYER_TEST_COMMON_H_
#define GST_WEB_AUDIO_PLAYER_TEST_COMMON_H_

#include "GlibWrapperMock.h"
#include "GstDispatcherThreadFactoryMock.h"
#include "GstDispatcherThreadMock.h"
#include "GstSrcFactoryMock.h"
#include "GstSrcMock.h"
#include "GstWebAudioPlayer.h"
#include "GstWebAudioPlayerClientMock.h"
#include "GstWrapperMock.h"
#include "WebAudioPlayerTaskFactoryMock.h"
#include "WorkerThreadFactoryMock.h"
#include "WorkerThreadMock.h"
#include <gtest/gtest.h>
#include <memory>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;

using ::testing::_;
using ::testing::ByMove;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::StrictMock;

class GstWebAudioPlayerTestCommon : public ::testing::Test
{
public:
    GstWebAudioPlayerTestCommon() = default;
    ~GstWebAudioPlayerTestCommon() override = default;

    StrictMock<GstWebAudioPlayerClientMock> m_gstPlayerClient;
    std::shared_ptr<StrictMock<GstWrapperMock>> m_gstWrapperMock{std::make_shared<StrictMock<GstWrapperMock>>()};
    std::shared_ptr<StrictMock<GlibWrapperMock>> m_glibWrapperMock{std::make_shared<StrictMock<GlibWrapperMock>>()};
    std::shared_ptr<StrictMock<GstSrcFactoryMock>> m_gstSrcFactoryMock{std::make_shared<StrictMock<GstSrcFactoryMock>>()};
    std::shared_ptr<StrictMock<GstSrcMock>> m_gstSrcMock{std::make_shared<StrictMock<GstSrcMock>>()};
    std::unique_ptr<IWebAudioPlayerTaskFactory> m_taskFactory{
        std::make_unique<StrictMock<WebAudioPlayerTaskFactoryMock>>()};
    StrictMock<WebAudioPlayerTaskFactoryMock> &m_taskFactoryMock{
        dynamic_cast<StrictMock<WebAudioPlayerTaskFactoryMock> &>(*m_taskFactory)};
    std::unique_ptr<IWorkerThreadFactory> workerThreadFactory{std::make_unique<StrictMock<WorkerThreadFactoryMock>>()};
    StrictMock<WorkerThreadFactoryMock> &m_workerThreadFactoryMock{
        dynamic_cast<StrictMock<WorkerThreadFactoryMock> &>(*workerThreadFactory)};
    std::unique_ptr<IWorkerThread> workerThread{std::make_unique<StrictMock<WorkerThreadMock>>()};
    StrictMock<WorkerThreadMock> &m_workerThreadMock{dynamic_cast<StrictMock<WorkerThreadMock> &>(*workerThread)};
    std::unique_ptr<IGstDispatcherThreadFactory> gstDispatcherThreadFactory{
        std::make_unique<StrictMock<GstDispatcherThreadFactoryMock>>()};
    StrictMock<GstDispatcherThreadFactoryMock> &m_gstDispatcherThreadFactoryMock{
        dynamic_cast<StrictMock<GstDispatcherThreadFactoryMock> &>(*gstDispatcherThreadFactory)};
    std::unique_ptr<IGstDispatcherThread> gstDispatcherThread{std::make_unique<StrictMock<GstDispatcherThreadMock>>()};
    StrictMock<GstDispatcherThreadMock> &m_gstDispatcherThreadMock{
        dynamic_cast<StrictMock<GstDispatcherThreadMock> &>(*gstDispatcherThread)};

protected:
    void gstPlayerWillBeCreatedForLlama();
    void gstPlayerWillBeCreatedForXiOne();
    void gstPlayerWillBeCreatedForGenericPlatform();
    void gstPlayerWillBeDestroyed();
    void executeTaskWhenEnqueued();
    void expectInitRialtoSrc();
    void expectInitThreads();
    void expectCreatePipeline();
    void expectInitAppSrc();
    void expectAddElementsAmlhalaSink();
    void expectAddElementsRtkAudioSink();
    void expectAddElementsAutoAudioSink();
    void expectMakeAmlhalaSink();
    void expectInitAmlhalaSink();
    void expectInitAmlhalaSinkFailure();
    void expectMakeRtkAudioSink();
    void expectInitRtkAudioSink();
    void expectInitRtkAudioSinkFailure();
    void expectMakeAutoAudioSink();
    void expectInitAutoAudioSink();
    void expectInitAutoAudioSinkFailure();
    void expectTermPipeline();
    void expectResetWorkerThread();
    void expectTaskStop();

    GstElement m_pipeline{};
    GstElement m_appSrc{};
    GstRegistry m_reg{};
    GstObject m_feature{};
    GstElement m_sink{};
    GstBus m_bus{};
};

#endif // GST_WEB_AUDIO_PLAYER_TEST_COMMON_H_
