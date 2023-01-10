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

constexpr std::uint32_t m_audioBufferLen{1 * 1024 * 1024}; // 1MB
constexpr std::uint32_t m_videoBufferLen{7 * 1024 * 1024}; // 7MB
constexpr std::uint32_t m_webAudioBufferLen{10 * 1024};    // 10KB

class SharedMemoryBufferTests : public testing::Test
{
public:
    SharedMemoryBufferTests() = default;
    virtual ~SharedMemoryBufferTests() = default;

    void initialize(int maxPlaybacks = 1, int maxWebAudioPlayers = 1);

    void mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType playbackType, int id);
    void mapPartitionShouldFail(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType playbackType, int id);
    void unmapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType playbackType,
                                     int id);
    void unmapPartitionShouldFail(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType playbackType, int id);
    void shouldReturnMaxGenericAudioDataLen(int id);
    void shouldFailToReturnMaxAudioDataLen(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType playbackType,
                                           int id);
    void shouldReturnMaxGenericVideoDataLen(int id);
    void shouldFailToReturnMaxVideoDataLen(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType playbackType,
                                           int id);
    void shouldReturnMaxWebAudioDataLen(int id);
    void shouldReturnVideoDataOffset(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType playbackType,
                                     int id, std::uint32_t expectedOffset);
    void shouldFailToReturnVideoDataOffset(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType playbackType,
                                           int id);
    void shouldReturnAudioDataOffset(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType playbackType,
                                     int id, std::uint32_t expectedOffset);
    void shouldFailToReturnAudioDataOffset(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType playbackType,
                                           int id);
    void shouldClearAudioData(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType playbackType, int id);
    void shouldFailToClearAudioData(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType playbackType,
                                    int id);
    void shouldClearVideoData(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType playbackType, int id);
    void shouldFailToClearVideoData(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType playbackType,
                                    int id);
    uint8_t *shouldGetDataPtr(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType playbackType, int id,
                              const firebolt::rialto::MediaSourceType &mediaSourceType);
    void shouldFailToGetDataPtr(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType playbackType, int id,
                                const firebolt::rialto::MediaSourceType &mediaSourceType);
    void shouldGetFd();
    void shouldGetSize();
    void shouldGetBuffer();

private:
    std::shared_ptr<firebolt::rialto::server::ISharedMemoryBuffer> m_sut;
};

#endif // SHARED_MEMORY_BUFFER_TESTS_FIXTURE_H_
