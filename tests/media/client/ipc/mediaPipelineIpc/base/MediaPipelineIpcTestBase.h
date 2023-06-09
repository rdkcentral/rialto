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

#ifndef MEDIA_PIPELINE_IPC_TEST_BASE_H_
#define MEDIA_PIPELINE_IPC_TEST_BASE_H_

#include "EventThreadFactoryMock.h"
#include "EventThreadMock.h"
#include "IpcModuleBase.h"
#include "MediaPipelineIpc.h"
#include "MediaPipelineIpcClientMock.h"
#include <gtest/gtest.h>
#include <memory>

using namespace firebolt::rialto;
using namespace firebolt::rialto::ipc;
using namespace firebolt::rialto::client;
using namespace firebolt::rialto::common;

using ::testing::_;
using ::testing::ByMove;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::Sequence;
using ::testing::StrictMock;
using ::testing::WithArgs;

class MediaPipelineIpcTestBase : public IpcModuleBase, public ::testing::Test
{
protected:
    // MediaPipelineIpc object
    std::unique_ptr<IMediaPipelineIpc> m_mediaPipelineIpc;

    // Strict Mocks
    StrictMock<MediaPipelineIpcClientMock> *m_clientMock = nullptr;
    std::shared_ptr<StrictMock<EventThreadFactoryMock>> m_eventThreadFactoryMock;
    std::unique_ptr<StrictMock<EventThreadMock>> m_eventThread;
    StrictMock<EventThreadMock> *m_eventThreadMock;

    // Common variables
    int32_t m_sessionId = 123;
    firebolt::rialto::CreateSessionResponse m_createSessionResponse;

    enum class EventTags
    {
        PlaybackStateChangeEvent = 0,
        PositionChangeEvent,
        NetworkStateChangeEvent,
        NeedMediaDataEvent,
        QosEvent,
        BufferUnderflowEvent
    };

    // Callbacks
    std::function<void(const std::shared_ptr<google::protobuf::Message> &msg)> m_playbackStateCb;
    std::function<void(const std::shared_ptr<google::protobuf::Message> &msg)> m_networkStateCb;
    std::function<void(const std::shared_ptr<google::protobuf::Message> &msg)> m_needDataCb;
    std::function<void(const std::shared_ptr<google::protobuf::Message> &msg)> m_positionChangeCb;
    std::function<void(const std::shared_ptr<google::protobuf::Message> &msg)> m_qosCb;
    std::function<void(const std::shared_ptr<google::protobuf::Message> &msg)> m_bufferUnderflowCb;

    void SetUp();
    void TearDown();
    void createMediaPipelineIpc();
    void expectSubscribeEvents();
    void expectUnsubscribeEvents();
    void destroyMediaPipelineIpc();

public:
    void setCreateSessionResponse(google::protobuf::Message *response);
};

#endif // MEDIA_PIPELINE_IPC_TEST_BASE_H_
