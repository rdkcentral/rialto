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

#ifndef WEB_AUDIO_PLAYER_TEST_BASE_H_
#define WEB_AUDIO_PLAYER_TEST_BASE_H_

#include "GstWebAudioPlayerFactoryMock.h"
#include "GstWebAudioPlayerMock.h"
#include "IGstWebAudioPlayerClient.h"
#include "MainThreadFactoryMock.h"
#include "MainThreadMock.h"
#include "SharedMemoryBufferMock.h"
#include "WebAudioPlayerClientMock.h"
#include "WebAudioPlayerServerInternal.h"
#include <gtest/gtest.h>
#include <memory>
#include <string>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;
using namespace firebolt::rialto::server::mock;

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::StrictMock;

class WebAudioPlayerTestBase : public ::testing::Test
{
public:
    WebAudioPlayerTestBase();
    virtual ~WebAudioPlayerTestBase();

protected:
    // WebAudioPlayerServerInternal object
    std::unique_ptr<WebAudioPlayerServerInternal> m_webAudioPlayer;

    // Strict Mocks
    std::shared_ptr<StrictMock<WebAudioPlayerClientMock>> m_webAudioPlayerClientMock;
    std::shared_ptr<StrictMock<SharedMemoryBufferMock>> m_sharedMemoryBufferMock;
    std::shared_ptr<StrictMock<MainThreadFactoryMock>> m_mainThreadFactoryMock;
    std::shared_ptr<StrictMock<MainThreadMock>> m_mainThreadMock;
    std::shared_ptr<StrictMock<GstWebAudioPlayerFactoryMock>> m_gstPlayerFactoryMock;
    std::unique_ptr<StrictMock<GstWebAudioPlayerMock>> m_gstPlayer;
    StrictMock<GstWebAudioPlayerMock> *m_gstPlayerMock;

    // Common variables
    const int m_webAudioPlayerHandle{1};
    const std::string m_audioMimeType{"audio/x-raw"};
    const uint32_t m_priority{5};
    const WebAudioConfig m_config{};
    const int32_t m_kMainThreadClientId{65};

    void createWebAudioPlayer();
    void destroyWebAudioPlayer();
    void mainThreadWillEnqueueTask();
    void mainThreadWillEnqueueTaskAndWait();
};

#endif // WEB_AUDIO_PLAYER_TEST_BASE_H_
