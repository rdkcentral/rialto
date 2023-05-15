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

#ifndef WEB_AUDIO_PLAYER_TEST_BASE_H_
#define WEB_AUDIO_PLAYER_TEST_BASE_H_

#include "ClientControllerMock.h"
#include "IWebAudioPlayerIpcClient.h"
#include "WebAudioPlayer.h"
#include "WebAudioPlayerClientMock.h"
#include "WebAudioPlayerIpcFactoryMock.h"
#include "WebAudioPlayerIpcMock.h"
#include <gtest/gtest.h>
#include <memory>
#include <string>

using namespace firebolt::rialto;
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

class WebAudioPlayerTestBase : public ::testing::Test
{
protected:
    const std::string m_audioMimeType{"audio/x-raw"};
    const uint32_t m_priority{5};
    WebAudioConfig m_config{};

    IWebAudioPlayerIpcClient *m_webAudioPlayerCallback;

    // Strict Mocks
    std::shared_ptr<StrictMock<WebAudioPlayerClientMock>> m_webAudioPlayerClientMock;
    std::shared_ptr<StrictMock<WebAudioPlayerIpcFactoryMock>> m_webAudioPlayerIpcFactoryMock;
    StrictMock<WebAudioPlayerIpcMock> *m_webAudioPlayerIpcMock = nullptr;
    StrictMock<ClientControllerMock> m_clientControllerMock;

    std::unique_ptr<WebAudioPlayer> m_webAudioPlayer;

    void SetUp();
    void TearDown();
    void createWebAudioPlayer();
    void destroyWebAudioPlayer();
};

#endif // WEB_AUDIO_PLAYER_TEST_BASE_H_
