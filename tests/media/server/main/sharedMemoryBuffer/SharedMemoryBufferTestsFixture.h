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

    void initialize(int maxPlaybacks = 1);

    void mapPartitionShouldSucceed(int sessionId);
    void mapPartitionShouldFail(int sessionId);
    void unmapPartitionShouldSucceed(int sessionId);
    void unmapPartitionShouldFail(int sessionId);
    void shouldReturnAudioBufferLen(int sessionId);
    void shouldFailToReturnAudioBufferLen(int sessionId);
    void shouldReturnVideoBufferLen(int sessionId);
    void shouldFailToReturnVideoBufferLen(int sessionId);
    void shouldReturnVideoBufferOffset(int sessionId, std::uint32_t expectedOffset);
    void shouldFailToReturnVideoBufferOffset(int sessionId);
    void shouldReturnAudioBufferOffset(int sessionId, std::uint32_t expectedOffset);
    void shouldFailToReturnAudioBufferOffset(int sessionId);
    void shouldClearAudioBuffer(int sessionId);
    void shouldFailToClearAudioBuffer(int sessionId);
    void shouldClearVideoBuffer(int sessionId);
    void shouldFailToClearVideoBuffer(int sessionId);
    uint8_t *shouldGetBuffer(int sessionId, const firebolt::rialto::MediaSourceType &mediaSourceType);
    void shouldFailToGetBuffer(int sessionId, const firebolt::rialto::MediaSourceType &mediaSourceType);
    void shouldGetFd();
    void shouldGetSize();

private:
    std::shared_ptr<firebolt::rialto::server::ISharedMemoryBuffer> m_sut;
};

#endif // SHARED_MEMORY_BUFFER_TESTS_FIXTURE_H_
