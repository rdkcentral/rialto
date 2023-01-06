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

#ifndef SHARED_MEMORY_BUFFER_TESTS_FIXTURE_H_
#define SHARED_MEMORY_BUFFER_TESTS_FIXTURE_H_

#include "SharedMemoryBuffer.h"
#include <gtest/gtest.h>
#include <memory>

class SharedMemoryBufferTests : public testing::Test
{
public:
    SharedMemoryBufferTests() = default;
    virtual ~SharedMemoryBufferTests() = default;

    void initialize(int maxPlaybacks = 1, int maxWebAudioPlayers = 2);

    void mapPartitionShouldSucceed(int sessionId);
    void mapPartitionShouldFail(int sessionId);
    void unmapPartitionShouldSucceed(int sessionId);
    void unmapPartitionShouldFail(int sessionId);
    void shouldReturnMaxAudioDataLen(int sessionId);
    void shouldFailToReturnMaxAudioDataLen(int sessionId);
    void shouldReturnMaxVideoDataLen(int sessionId);
    void shouldFailToReturnMaxVideoDataLen(int sessionId);
    void shouldReturnVideoDataOffset(int sessionId, std::uint32_t expectedOffset);
    void shouldFailToReturnVideoDataOffset(int sessionId);
    void shouldReturnAudioDataOffset(int sessionId, std::uint32_t expectedOffset);
    void shouldFailToReturnAudioDataOffset(int sessionId);
    void shouldClearAudioData(int sessionId);
    void shouldFailToClearAudioData(int sessionId);
    void shouldClearVideoData(int sessionId);
    void shouldFailToClearVideoData(int sessionId);
    uint8_t *shouldGetDataPtr(int sessionId, const firebolt::rialto::MediaSourceType &mediaSourceType);
    void shouldFailToGetDataPtr(int sessionId, const firebolt::rialto::MediaSourceType &mediaSourceType);
    void shouldGetFd();
    void shouldGetSize();
    void shouldGetBuffer();

private:
    std::shared_ptr<firebolt::rialto::server::ISharedMemoryBuffer> m_sut;
};

#endif // SHARED_MEMORY_BUFFER_TESTS_FIXTURE_H_
