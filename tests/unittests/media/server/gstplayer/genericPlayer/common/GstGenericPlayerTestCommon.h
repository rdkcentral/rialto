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

#ifndef GST_GENERIC_PLAYER_TEST_COMMON_H_
#define GST_GENERIC_PLAYER_TEST_COMMON_H_

#include "DecryptionServiceMock.h"
#include "GenericPlayerTaskFactoryMock.h"
#include "GlibWrapperMock.h"
#include "GstDispatcherThreadFactoryMock.h"
#include "GstDispatcherThreadMock.h"
#include "GstGenericPlayer.h"
#include "GstGenericPlayerClientMock.h"
#include "GstProtectionMetadataHelperFactoryMock.h"
#include "GstProtectionMetadataHelperMock.h"
#include "GstSrcFactoryMock.h"
#include "GstSrcMock.h"
#include "GstWrapperMock.h"
#include "RdkGstreamerUtilsWrapperMock.h"
#include "TimerFactoryMock.h"
#include "WorkerThreadFactoryMock.h"
#include "WorkerThreadMock.h"
#include <gtest/gtest.h>
#include <memory>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;
using namespace firebolt::rialto::wrappers;

using ::testing::StrictMock;

class GstGenericPlayerTestCommon : public ::testing::Test
{
public:
    GstGenericPlayerTestCommon() = default;
    ~GstGenericPlayerTestCommon() override = default;

    void triggerSetupSource(GstElement *element);
    void triggerSetupElement(GstElement *element);
    void triggerDeepElementAdded(GstElement *element);

    StrictMock<GstGenericPlayerClientMock> m_gstPlayerClient;
    std::shared_ptr<StrictMock<GstWrapperMock>> m_gstWrapperMock{std::make_shared<StrictMock<GstWrapperMock>>()};
    std::shared_ptr<StrictMock<GlibWrapperMock>> m_glibWrapperMock{std::make_shared<StrictMock<GlibWrapperMock>>()};
    std::shared_ptr<StrictMock<GstSrcFactoryMock>> m_gstSrcFactoryMock{std::make_shared<StrictMock<GstSrcFactoryMock>>()};
    std::shared_ptr<StrictMock<GstSrcMock>> m_gstSrcMock{std::make_shared<StrictMock<GstSrcMock>>()};
    std::shared_ptr<StrictMock<TimerFactoryMock>> m_timerFactoryMock{std::make_shared<StrictMock<TimerFactoryMock>>()};
    std::unique_ptr<IGenericPlayerTaskFactory> m_taskFactory{std::make_unique<StrictMock<GenericPlayerTaskFactoryMock>>()};
    StrictMock<GenericPlayerTaskFactoryMock> &m_taskFactoryMock{
        dynamic_cast<StrictMock<GenericPlayerTaskFactoryMock> &>(*m_taskFactory)};
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
    StrictMock<DecryptionServiceMock> m_decryptionServiceMock;
    std::shared_ptr<StrictMock<GstProtectionMetadataHelperFactoryMock>> m_gstProtectionMetadataFactoryMock{
        std::make_shared<StrictMock<GstProtectionMetadataHelperFactoryMock>>()};
    std::unique_ptr<StrictMock<GstProtectionMetadataHelperMock>> m_gstProtectionMetadataWrapper{
        std::make_unique<StrictMock<GstProtectionMetadataHelperMock>>()};
    StrictMock<GstProtectionMetadataHelperMock> *m_gstProtectionMetadataWrapperMock{m_gstProtectionMetadataWrapper.get()};

public:
    void setPipelineState(const GstState &state);

protected:
    void gstPlayerWillBeCreated();
    void gstPlayerWillBeDestroyed();
    void expectShutdown();
    void expectStop();
    void executeTaskWhenEnqueued();
    void initFactories();
    void expectMakePlaybin();
    void expectSetFlags();
    void expectSetFlagsWithNativeAudio();
    void expectSetSignalCallbacks();
    void expectSetUri();
    void expectCheckPlaySink();
    void expectSetMessageCallback();

private:
    GstElement m_pipeline{};
    GFlagsClass m_flagsClass{};
    GstElement m_playsink{};
    GstBus m_bus{};
    GstBin m_bin{};
    GType m_gstPlayFlagsType = static_cast<GType>(123);
    GFlagsValue m_audioFlag{1, "audio", "audio"};
    GFlagsValue m_videoFlag{2, "video", "video"};
    GFlagsValue m_nativeVideoFlag{3, "native-video", "native-video"};
    GFlagsValue m_nativeAudioFlag{4, "native-audio", "native-audio"};
    gpointer m_setupSourceUserData;
    GCallback m_setupSourceFunc;
    gulong m_setupSourceSignalId{0};
    gpointer m_setupElementUserData;
    GCallback m_setupElementFunc;
    gulong m_setupElementSignalId{1};
    gpointer m_deepElementAddedUserData;
    GCallback m_deepElementAddedFunc;
    gulong m_deepElementAddedSignalId{2};
    GstObject m_sinkFactory{}; // GstElementFactory is an opaque data structure
};

#endif // GST_GENERIC_PLAYER_TEST_COMMON_H_
