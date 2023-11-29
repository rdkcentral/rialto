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

#include "MediaPipelineTestMethods.h"
#include "MediaPipelineMatchers.h"
#include "MediaPipelineProtoUtils.h"
#include <memory>

namespace
{
    constexpr VideoRequirements kVideoRequirements{123, 456};
    constexpr int32_t kSessionId{10};
    constexpr MediaType kMediaType = MediaType::MSE;
    const std::string kMimeType = "mime";
    const std::string kUrl = "mse://1";
} // namespace

MediaPipelineTestMethods::MediaPipelineTestMethods()
    : m_mediaPipelineClientMock{std::make_shared<StrictMock<MediaPipelineClientMock>>()},
      m_mediaPipelineModuleMock{std::make_shared<StrictMock<MediaPipelineModuleMock>>()}
{
}

MediaPipelineTestMethods::~MediaPipelineTestMethods()
{
}

void MediaPipelineTestMethods::shouldCreateMediaSession()
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, createSession(_, createSessionRequestMatcher(kVideoRequirements.maxWidth, kVideoRequirements.maxHeight), _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_mediaPipelineModuleMock->createSessionResponse(kSessionId)),
                        WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn))));
}

void MediaPipelineTestMethods::createMediaPipeline()
{
    m_mediaPipelineFactory = firebolt::rialto::IMediaPipelineFactory::createFactory();
    m_mediaPipeline = m_mediaPipelineFactory->createMediaPipeline(m_mediaPipelineClientMock, kVideoRequirements);
    EXPECT_NE(m_mediaPipeline, nullptr);
}

void MediaPipelineTestMethods::shouldLoad()
{
    EXPECT_CALL(*m_mediaPipelineModuleMock, load(_, loadRequestMatcher(kSessionId, convertMediaType(kMediaType), kMimeType, kUrl), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_mediaPipelineModuleMock), &MediaPipelineModuleMock::defaultReturn)));
}

void MediaPipelineTestMethods::load()
{
    EXPECT_EQ(m_mediaPipeline->load(kMediaType, kMimeType, kUrl), true);
}
