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

#ifndef MEDIA_PIPELINE_TEST_BASE_H_
#define MEDIA_PIPELINE_TEST_BASE_H_

#include "ActiveRequestsMock.h"
#include "DataReaderFactoryMock.h"
#include "DataReaderMock.h"
#include "DecryptionServiceMock.h"
#include "GstGenericPlayerFactoryMock.h"
#include "GstGenericPlayerMock.h"
#include "IGstGenericPlayerClient.h"
#include "MainThreadFactoryMock.h"
#include "MainThreadMock.h"
#include "MediaPipelineClientMock.h"
#include "MediaPipelineServerInternal.h"
#include "MediaSourceUtil.h"
#include "SharedMemoryBufferMock.h"
#include "TimerFactoryMock.h"
#include "TimerMock.h"
#include <gtest/gtest.h>
#include <memory>
#include <string>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;
using namespace firebolt::rialto::server::mock;

using ::testing::_;
using ::testing::A;
using ::testing::ByMove;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Ref;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::SaveArg;
using ::testing::StrictMock;

class MediaPipelineTestBase : public ::testing::Test
{
public:
    MediaPipelineTestBase();
    virtual ~MediaPipelineTestBase();

protected:
    // MediaPipelineServerInternal object
    std::unique_ptr<MediaPipelineServerInternal> m_mediaPipeline;

    // Strict Mocks
    std::shared_ptr<StrictMock<MediaPipelineClientMock>> m_mediaPipelineClientMock;
    std::shared_ptr<StrictMock<GstGenericPlayerFactoryMock>> m_gstPlayerFactoryMock;
    std::unique_ptr<StrictMock<GstGenericPlayerMock>> m_gstPlayer;
    StrictMock<GstGenericPlayerMock> *m_gstPlayerMock;
    std::shared_ptr<StrictMock<SharedMemoryBufferMock>> m_sharedMemoryBufferMock;
    std::unique_ptr<StrictMock<DataReaderFactoryMock>> m_dataReaderFactory;
    StrictMock<DataReaderFactoryMock> *m_dataReaderFactoryMock;
    std::unique_ptr<StrictMock<ActiveRequestsMock>> m_activeRequests;
    StrictMock<ActiveRequestsMock> *m_activeRequestsMock;
    std::shared_ptr<StrictMock<MainThreadFactoryMock>> m_mainThreadFactoryMock;
    std::shared_ptr<StrictMock<MainThreadMock>> m_mainThreadMock;
    std::shared_ptr<StrictMock<TimerFactoryMock>> m_timerFactoryMock;
    std::unique_ptr<StrictMock<TimerMock>> m_timerMock;
    StrictMock<DecryptionServiceMock> m_decryptionServiceMock;
    IGstGenericPlayerClient *m_gstPlayerCallback;

    // Common variables
    const int m_kSessionId{1};
    const int32_t m_kMainThreadClientId = {5};
    VideoRequirements m_videoReq = {123, 456};
    const bool m_kEnableInstantRateChangeSeek{true};

    void createMediaPipeline();
    void destroyMediaPipeline();
    void mainThreadWillEnqueueTask();
    void mainThreadWillEnqueueTaskAndWait();
    void loadGstPlayer();
    int attachSource(MediaSourceType sourceType, const std::string &mimeType);
    void setEos(MediaSourceType sourceType);
    void expectNotifyNeedData(MediaSourceType sourceType, int sourceId, int numFrames);
    void expectNotifyNeedDataEos(MediaSourceType sourceType);
};

#endif // MEDIA_PIPELINE_TEST_BASE_H_
