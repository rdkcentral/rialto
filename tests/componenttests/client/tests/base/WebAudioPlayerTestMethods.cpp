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
#include "CommonConstants.h"
// #include "WebAudioPlayerRequestMatcher.h"
#include "MediaSegments.h"
#include "MetadataProtoMatchers.h"
#include "MetadataProtoUtils.h"
#include "metadata.pb.h"
#include <memory>
#include <string>
#include <vector>

// namespace
// {
// // const std::string kAudioMimeType{"audio/mp4"};
// // const uint32_t kPriority{1};
// } // namespace

namespace firebolt::rialto::client::ct
{
WebAudioPlayerTestMethods::WebAudioPlayerTestMethods()
    : m_webAudioPlayerModuleMock{std::make_shared<StrictMock<WebAudioPlayerModuleMock>>()}
{
}

void WebAudioPlayerTestMethods::shouldCreateWebAudioPlayer()
{
    // EXPECT_CALL(*m_webAudioPlayerModuleMock,
    //             createAudioPlayer(_, webAudioPlayerRequestMatcher(kAudioMimeType, kPriority),
    //                           _, _))
    //     .WillOnce(WithArgs<0, 3>(Invoke(&(*m_webAudioPlayerModuleMock), &WebAudioPlayerModuleMock::defaultReturn)));
}

void WebAudioPlayerTestMethods::createWebAudioPlayer()
{
    // createWebAudioPlayer()
}


WebAudioPlayerTestMethods::~WebAudioPlayerTestMethods() {}

} // firebolt::rialto::client::ct