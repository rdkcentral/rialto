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

#include "WebAudioPlayerTestMethods.h"
#include "WebAudioPlayerProtoRequestMatchers.h"
#include <memory>
#include <string>
#include <vector>

namespace
{
const std::string kAudioMimeType{"audio/x-raw"};
const uint32_t kPriority{5};
std::shared_ptr<const WebAudioConfig> kConfig = std::make_shared<const WebAudioConfig>();
const int32_t kValue{1};
} // namespace

namespace firebolt::rialto::client::ct
{
WebAudioPlayerTestMethods::WebAudioPlayerTestMethods()
    : m_webAudioPlayerModuleMock{std::make_shared<StrictMock<WebAudioPlayerModuleMock>>()},
      m_webAudioPlayerClientMock{std::make_shared<StrictMock<WebAudioPlayerClientMock>>()}
{
}

void WebAudioPlayerTestMethods::shouldCreateWebAudioPlayer()
{
    EXPECT_CALL(*m_webAudioPlayerModuleMock,
                createWebAudioPlayer(_, createWebAudioPlayerRequestMatcher(kAudioMimeType, kPriority, kConfig),
                              _, _))
        .WillOnce
            (DoAll(SetArgPointee<2>(m_webAudioPlayerModuleMock->createWebAudioPlayerResponse(kValue)),
                (WithArgs<0, 3>(Invoke(&(*m_webAudioPlayerModuleMock), &WebAudioPlayerModuleMock::defaultReturn)))));
}

// void WebAudioPlayerTestMethods::shouldCreateWebAudioPlayer()
// {
//     EXPECT_CALL(*m_webAudioPlayerModuleMock,
//                 createWebAudioPlayer(_, createWebAudioPlayerRequestMatcher(kAudioMimeType, kPriority, kConfig),
//                               _, _))
//         .WillOnce(WithArgs<0, 3>(Invoke(&(*m_webAudioPlayerModuleMock), &WebAudioPlayerModuleMock::defaultReturn)));
// }
void WebAudioPlayerTestMethods::createWebAudioPlayer()
{
    m_webAudioPlayerFactory = firebolt::rialto::IWebAudioPlayerFactory::createFactory();
    EXPECT_NO_THROW(
        m_webAudioPlayer = m_webAudioPlayerFactory->createWebAudioPlayer(m_webAudioPlayerClientMock, kAudioMimeType, kPriority, kConfig));
    EXPECT_EQ(m_webAudioPlayer, nullptr);

}


WebAudioPlayerTestMethods::~WebAudioPlayerTestMethods() {}

} // firebolt::rialto::client::ct