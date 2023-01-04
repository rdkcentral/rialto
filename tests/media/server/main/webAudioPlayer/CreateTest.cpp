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

#include "WebAudioPlayerServerInternal.h"
#include "WebAudioPlayerClientMock.h"
#include "SharedMemoryBufferMock.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;

using ::testing::Return;
using ::testing::StrictMock;

class RialtoServerCreateWebAudioPlayerTest : public ::testing::Test
{
protected:
    const std::string m_audioMimeType{"audio/x-raw"};
    const uint32_t m_priority{5};
    const WebAudioConfig m_config{};

    std::shared_ptr<StrictMock<WebAudioPlayerClientMock>> m_webAudioPlayerClientMock;
    std::shared_ptr<StrictMock<SharedMemoryBufferMock>> m_sharedMemoryBufferMock;

    RialtoServerCreateWebAudioPlayerTest()
        : m_webAudioPlayerClientMock{std::make_shared<StrictMock<WebAudioPlayerClientMock>>()},
          m_sharedMemoryBufferMock{std::make_shared<StrictMock<SharedMemoryBufferMock>>()}
    {
    }
};

/**
 * Test that a WebAudioPlayer object can be created successfully.
 */
TEST_F(RialtoServerCreateWebAudioPlayerTest, Create)
{
    std::unique_ptr<IWebAudioPlayer> webAudioPlayer;

    EXPECT_NO_THROW(webAudioPlayer = std::make_unique<WebAudioPlayerServerInternal>(m_webAudioPlayerClientMock, m_audioMimeType,
                                                                      m_priority, &m_config, m_sharedMemoryBufferMock));
    EXPECT_NE(webAudioPlayer, nullptr);
}
