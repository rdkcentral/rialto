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

#include "WebAudioPlayerTestBase.h"
#include <memory>
#include <utility>

void WebAudioPlayerTestBase::SetUp() // NOLINT(build/function_format)
{
    // Create StrictMocks
    m_webAudioPlayerClientMock = std::make_shared<StrictMock<WebAudioPlayerClientMock>>();
    m_webAudioPlayerIpcFactoryMock = std::make_shared<StrictMock<WebAudioPlayerIpcFactoryMock>>();

    // Init pcm config
    m_config.pcm.rate = 1;
    m_config.pcm.channels = 2;
    m_config.pcm.sampleSize = 16;
    m_config.pcm.isBigEndian = false;
    m_config.pcm.isSigned = false;
    m_config.pcm.isFloat = false;
}

void WebAudioPlayerTestBase::TearDown() // NOLINT(build/function_format)
{
    // Destroy StrictMocks
    m_webAudioPlayerIpcMock = nullptr;
    m_webAudioPlayerIpcFactoryMock.reset();
    m_webAudioPlayerClientMock.reset();
}

void WebAudioPlayerTestBase::createWebAudioPlayer()
{
    webAudioPlayerIpcMock = std::make_unique<StrictMock<WebAudioPlayerIpcMock>>();

    // Save a raw pointer to the unique object for use when testing mocks
    // Object shall be freed by the holder of the unique ptr on destruction
    m_webAudioPlayerIpcMock = webAudioPlayerIpcMock.get();

    EXPECT_CALL(m_clientControllerMock, registerClient(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(ApplicationState::RUNNING), Return(true)));

    EXPECT_CALL(*m_webAudioPlayerIpcFactoryMock, createWebAudioPlayerIpc(_, _, _, _))
        .WillOnce(Return(ByMove(std::move(webAudioPlayerIpcMock))));

    EXPECT_NO_THROW(m_webAudioPlayer = std::make_unique<WebAudioPlayer>(m_webAudioPlayerClientMock, m_audioMimeType,
                                                                        m_priority, &m_config,
                                                                        m_webAudioPlayerIpcFactoryMock,
                                                                        m_clientControllerMock));

    // Save a raw pointer here same as above
    m_webAudioPlayerCallback = m_webAudioPlayer.get();
    EXPECT_NE(m_webAudioPlayer, nullptr);
}

void WebAudioPlayerTestBase::destroyWebAudioPlayer()
{
    EXPECT_CALL(m_clientControllerMock, unregisterClient(_)).WillOnce(Return(true));
    m_webAudioPlayer.reset();
    m_webAudioPlayerCallback = nullptr;
}
