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
#include "GstPlayerFactoryMock.h"
#include "GstPlayerMock.h"
#include "IGstPlayerClient.h"
#include "MainThreadFactoryMock.h"
#include "MainThreadMock.h"
#include "MediaPipelineClientMock.h"
#include "MediaPipelineServerInternal.h"
#include "SharedMemoryBufferMock.h"
#include "MediaSourceUtil.h"
#include <gtest/gtest.h>
#include <memory>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;
using namespace firebolt::rialto::server::mock;

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::StrictMock;

namespace
{
constexpr int sessionId{1};
} // namespace

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
    std::shared_ptr<StrictMock<GstPlayerFactoryMock>> m_gstPlayerFactoryMock;
    std::unique_ptr<StrictMock<GstPlayerMock>> m_gstPlayer;
    StrictMock<GstPlayerMock> *m_gstPlayerMock;
    std::shared_ptr<StrictMock<SharedMemoryBufferMock>> m_sharedMemoryBufferMock;
    std::unique_ptr<StrictMock<DataReaderFactoryMock>> m_dataReaderFactory;
    StrictMock<DataReaderFactoryMock> *m_dataReaderFactoryMock;
    std::unique_ptr<StrictMock<ActiveRequestsMock>> m_activeRequests;
    StrictMock<ActiveRequestsMock> *m_activeRequestsMock;
    std::shared_ptr<StrictMock<MainThreadFactoryMock>> m_mainThreadFactoryMock;
    std::shared_ptr<StrictMock<MainThreadMock>> m_mainThreadMock;
    StrictMock<DecryptionServiceMock> m_decryptionServiceMock;

    // Common variables
    const int m_kSessionId{1};
    const int32_t m_kMainThreadClientId = {5};
    VideoRequirements m_videoReq = {};

    void createMediaPipeline();
    void destroyMediaPipeline();
    void mainThreadWillEnqueueTask();
    void mainThreadWillEnqueueTaskAndWait();
    void loadGstPlayer();
};

#endif // MEDIA_PIPELINE_TEST_BASE_H_
