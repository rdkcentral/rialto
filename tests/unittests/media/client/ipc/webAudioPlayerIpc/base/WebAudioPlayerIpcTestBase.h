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

#ifndef WEB_AUDIO_PLAYER_IPC_TEST_BASE_H_
#define WEB_AUDIO_PLAYER_IPC_TEST_BASE_H_

#include "EventThreadFactoryMock.h"
#include "EventThreadMock.h"
#include "IpcModuleBase.h"
#include "WebAudioPlayerIpc.h"
#include "WebAudioPlayerIpcClientMock.h"
#include <gtest/gtest.h>
#include <memory>
#include <string>

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

class WebAudioPlayerIpcTestBase : public IpcModuleBase, public ::testing::Test
{
protected:
    const std::string m_audioMimeType{"audio/x-raw"};
    const uint32_t m_priority{5};
    std::shared_ptr<WebAudioConfig> m_config = std::make_shared<WebAudioConfig>();

    // WebAudioPlayerIpc object
    std::unique_ptr<IWebAudioPlayerIpc> m_webAudioPlayerIpc;

    // Strict Mocks
    StrictMock<WebAudioPlayerIpcClientMock> *m_clientMock = nullptr;
    std::shared_ptr<StrictMock<EventThreadFactoryMock>> m_eventThreadFactoryMock;
    std::unique_ptr<StrictMock<EventThreadMock>> m_eventThread;
    StrictMock<EventThreadMock> *m_eventThreadMock;

    // Common variables
    int32_t m_webAaudioPlayerHandle = 123;

    enum class EventTags
    {
        notifyStateEvent = 0
    };

    // Callbacks
    std::function<void(const std::shared_ptr<google::protobuf::Message> &msg)> m_notifyStateCb;

    void SetUp();
    void TearDown();
    void createWebAudioPlayerIpc();
    void expectSubscribeEvents();
    void expectUnsubscribeEvents();
    void destroyWebAudioPlayerIpc();

public:
    void setCreateWebAudioPlayerResponse(google::protobuf::Message *response);
};

#endif // WEB_AUDIO_PLAYER_IPC_TEST_BASE_H_
