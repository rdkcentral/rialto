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

#include "ClientControllerMock.h"
#include "IMediaPipelineIpcClient.h"
#include "MediaFrameWriterFactoryMock.h"
#include "MediaPipeline.h"
#include "MediaPipelineClientMock.h"
#include "MediaPipelineIpcFactoryMock.h"
#include "MediaPipelineIpcMock.h"
#include <gtest/gtest.h>
#include <memory>

using namespace firebolt::rialto;
using namespace firebolt::rialto::common;
using namespace firebolt::rialto::client;

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::ByMove;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Ref;
using ::testing::Return;
using ::testing::SaveArg;
using ::testing::SetArgReferee;
using ::testing::StrictMock;

MATCHER(IsNull, "")
{
    return arg == nullptr;
}

class MediaPipelineTestBase : public ::testing::Test
{
protected:
    IMediaPipelineIpcClient *m_mediaPipelineCallback;

    // Strict Mocks
    std::shared_ptr<StrictMock<MediaPipelineClientMock>> m_mediaPipelineClientMock;
    std::shared_ptr<StrictMock<MediaPipelineIpcFactoryMock>> m_mediaPipelineIpcFactoryMock;
    StrictMock<MediaPipelineIpcMock> *m_mediaPipelineIpcMock = nullptr;
    std::shared_ptr<StrictMock<MediaFrameWriterFactoryMock>> m_mediaFrameWriterFactoryMock;
    std::shared_ptr<StrictMock<ClientControllerMock>> m_clientControllerMock;
    std::unique_ptr<StrictMock<MediaPipelineIpcMock>> mediaPipelineIpcMock;

    // MediaPipeline object
    std::shared_ptr<MediaPipeline> m_mediaPipeline;

    void SetUp();
    void TearDown();
    void createMediaPipeline();
    void destroyMediaPipeline();
    void setPlaybackState(PlaybackState state);
    void setNetworkState(NetworkState state);
    void needData(int32_t sourceId, size_t frameCount, uint32_t requestId,
                  const std::shared_ptr<MediaPlayerShmInfo> &shmInfo);
};

#endif // MEDIA_PIPELINE_TEST_BASE_H_
