/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Sky UK
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

#ifndef FIREBOLT_RIALTO_CLIENT_CT_WEB_AUDIO_PLAYER_TEST_METHODS_H_
#define FIREBOLT_RIALTO_CLIENT_CT_WEB_AUDIO_PLAYER_TEST_METHODS_H_

#include "IWebAudioPlayer.h"
#include "WebAudioPlayerModuleMock.h"
#include "WebAudioPlayerClientMock.h"
#include "ServerStub.h"
#include <gtest/gtest.h>
#include <map>
#include <memory>
#include <string>
#include <vector>

using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::InvokeWithoutArgs;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::StrictMock;
using ::testing::WithArgs;

using namespace firebolt::rialto;

namespace firebolt::rialto::client::ct
{
class WebAudioPlayerTestMethods
{
public:
    WebAudioPlayerTestMethods();
    virtual ~WebAudioPlayerTestMethods();

protected:
    // Strict Mocks
    std::shared_ptr<StrictMock<WebAudioPlayerModuleMock>> m_webAudioPlayerModuleMock;
    std::shared_ptr<StrictMock<WebAudioPlayerClientMock>> m_webAudioPlayerClientMock;

    // Objects
    std::shared_ptr<IWebAudioPlayerFactory> m_webAudioPlayerFactory;
    std::shared_ptr<IWebAudioPlayer> m_webAudioPlayer;

    // Expect methods
    void shouldCreateWebAudioPlayer();


    // Api methods
    void createWebAudioPlayer();

};
} // namespace firebolt::rialto::client::ct

#endif // FIREBOLT_RIALTO_CLIENT_CT_WEB_AUDIO_PLAYER_TEST_METHODS_H_
