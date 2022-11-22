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

#include "SharedMemoryBufferTestsFixture.h"

TEST_F(SharedMemoryBufferTests, shouldMapSession)
{
    constexpr int session1{0};
    initialize();
    mapPartitionShouldSucceed(session1);
}

TEST_F(SharedMemoryBufferTests, shouldSkipToMapTheSameSessionTwice)
{
    constexpr int session1{0};
    initialize();
    mapPartitionShouldSucceed(session1);
    mapPartitionShouldSucceed(session1);
}

TEST_F(SharedMemoryBufferTests, shouldFailToMapTwoSessions)
{
    // maxSessions is set to 1
    constexpr int session1{0}, session2{1};
    initialize();
    mapPartitionShouldSucceed(session1);
    mapPartitionShouldFail(session2);
}

TEST_F(SharedMemoryBufferTests, shouldUnmapSession)
{
    constexpr int session1{0};
    initialize();
    mapPartitionShouldSucceed(session1);
    unmapPartitionShouldSucceed(session1);
}

TEST_F(SharedMemoryBufferTests, shouldMapNewSessionAfterUnmapingTheOldOne)
{
    // maxSessions is set to 1
    constexpr int session1{0}, session2{1};
    initialize();
    mapPartitionShouldSucceed(session1);
    unmapPartitionShouldSucceed(session1);
    mapPartitionShouldSucceed(session2);
}

TEST_F(SharedMemoryBufferTests, shouldFailToUnmapNotExistingSession)
{
    constexpr int session1{0};
    initialize();
    unmapPartitionShouldFail(session1);
}

TEST_F(SharedMemoryBufferTests, shouldReturnMaxAudioDataLen)
{
    constexpr int session1{0};
    initialize();
    mapPartitionShouldSucceed(session1);
    shouldReturnMaxAudioDataLen(session1);
}

TEST_F(SharedMemoryBufferTests, shouldNotReturnMaxAudioDataLenForNotMappedSession)
{
    constexpr int session1{0};
    initialize();
    shouldFailToReturnMaxAudioDataLen(session1);
}

TEST_F(SharedMemoryBufferTests, shouldReturnMaxVideoDataLen)
{
    constexpr int session1{0};
    initialize();
    mapPartitionShouldSucceed(session1);
    shouldReturnMaxVideoDataLen(session1);
}

TEST_F(SharedMemoryBufferTests, shouldNotReturnMaxVideoDataLenForNotMappedSession)
{
    constexpr int session1{0};
    initialize();
    shouldFailToReturnMaxVideoDataLen(session1);
}

TEST_F(SharedMemoryBufferTests, shouldClearAudioData)
{
    constexpr int session1{0};
    initialize();
    mapPartitionShouldSucceed(session1);
    shouldClearAudioData(session1);
}

TEST_F(SharedMemoryBufferTests, shouldNotClearAudioDataForNotMappedSession)
{
    constexpr int session1{0};
    initialize();
    shouldFailToClearAudioData(session1);
}

TEST_F(SharedMemoryBufferTests, shouldClearVideoData)
{
    constexpr int session1{0};
    initialize();
    mapPartitionShouldSucceed(session1);
    shouldClearVideoData(session1);
}

TEST_F(SharedMemoryBufferTests, shouldNotClearVideoDataForNotMappedSession)
{
    constexpr int session1{0};
    initialize();
    shouldFailToClearVideoData(session1);
}

TEST_F(SharedMemoryBufferTests, shouldFailToReturnVideoDataOffset)
{
    constexpr int session1{0};
    initialize();
    shouldFailToReturnVideoDataOffset(session1);
}

TEST_F(SharedMemoryBufferTests, shouldFailToReturnAudioDataOffset)
{
    constexpr int session1{0};
    initialize();
    shouldFailToReturnAudioDataOffset(session1);
}

TEST_F(SharedMemoryBufferTests, shouldReturnVideoDataOffsetForOneSession)
{
    constexpr int session1{0};
    // Video buffer for first mapped session is at the beginning of shm
    constexpr std::uint32_t expectedOffset{0};
    initialize();
    mapPartitionShouldSucceed(session1);
    shouldReturnVideoDataOffset(session1, expectedOffset);
}

TEST_F(SharedMemoryBufferTests, shouldReturnAudioDataOffsetForOneSession)
{
    constexpr int session1{0};
    // Audio buffer for first mapped session is after video buffer - 7MB
    constexpr std::uint32_t expectedOffset{7 * 1024 * 1024};
    initialize();
    mapPartitionShouldSucceed(session1);
    shouldReturnAudioDataOffset(session1, expectedOffset);
}

TEST_F(SharedMemoryBufferTests, shouldReturnVideoDataOffsetForTwoSessions)
{
    constexpr int maxPlaybacks{2};
    constexpr int session1{0}, session2{1};
    // Video buffer for second mapped session is after video and audio buffer of 1st session - 7MB + 1MB = 8MB
    constexpr std::uint32_t expectedOffset{8 * 1024 * 1024};
    initialize(maxPlaybacks);
    mapPartitionShouldSucceed(session1);
    mapPartitionShouldSucceed(session2);
    shouldReturnVideoDataOffset(session2, expectedOffset);
}

TEST_F(SharedMemoryBufferTests, shouldReturnAudioDataOffsetForTwoSessions)
{
    constexpr int maxPlaybacks{2};
    constexpr int session1{0}, session2{1};
    // Video buffer for second mapped session is after video and audio buffer of 1st session
    // and video buffer of 2nd session - 7MB + 1MB + 7MB = 15MB
    constexpr std::uint32_t expectedOffset{15 * 1024 * 1024};
    initialize(maxPlaybacks);
    mapPartitionShouldSucceed(session1);
    mapPartitionShouldSucceed(session2);
    shouldReturnAudioDataOffset(session2, expectedOffset);
}

TEST_F(SharedMemoryBufferTests, shouldFailToGetDataPtrForUnmappedSession)
{
    constexpr int session1{0};
    initialize();
    shouldFailToGetDataPtr(session1, firebolt::rialto::MediaSourceType::VIDEO);
    shouldFailToGetDataPtr(session1, firebolt::rialto::MediaSourceType::AUDIO);
}

TEST_F(SharedMemoryBufferTests, shouldFailToGetDataPtrForUnknownSourceType)
{
    constexpr int session1{0};
    initialize();
    mapPartitionShouldSucceed(session1);
    shouldFailToGetBufDataPtr(session1, firebolt::rialto::MediaSourceType::UNKNOWN);
}

TEST_F(SharedMemoryBufferTests, shouldGetDataPtr)
{
    constexpr int maxPlaybacks{2};
    constexpr int session1{0}, session2{1};
    initialize(maxPlaybacks);
    mapPartitionShouldSucceed(session1);
    mapPartitionShouldSucceed(session2);
    uint8_t *session1Video = shouldGetDataPtr(session1, firebolt::rialto::MediaSourceType::VIDEO);
    uint8_t *session1Audio = shouldGetDataPtr(session1, firebolt::rialto::MediaSourceType::AUDIO);
    uint8_t *session2Video = shouldGetDataPtr(session2, firebolt::rialto::MediaSourceType::VIDEO);
    uint8_t *session2Audio = shouldGetDataPtr(session2, firebolt::rialto::MediaSourceType::AUDIO);
    EXPECT_NE(nullptr, session1Video);
    EXPECT_NE(nullptr, session1Audio);
    EXPECT_NE(nullptr, session2Video);
    EXPECT_NE(nullptr, session2Audio);
    EXPECT_EQ((session1Audio - session1Video),
              (7 * 1024 * 1024)); // Difference between S1 audio & S1 video should be 7 MB
    EXPECT_EQ((session2Video - session1Audio),
              (1 * 1024 * 1024)); // Difference between S2 video & S1 audio should be 1 MB
    EXPECT_EQ((session2Video - session1Video),
              (8 * 1024 * 1024)); // Difference between S2 video & S1 video should be 8 MB
    EXPECT_EQ((session2Audio - session2Video),
              (7 * 1024 * 1024)); // Difference between S2 video & S2 audio should be 7 MB
}

TEST_F(SharedMemoryBufferTests, shouldGetFd)
{
    initialize();
    shouldGetFd();
}

TEST_F(SharedMemoryBufferTests, shouldGetSize)
{
    initialize();
    shouldGetSize();
}

TEST_F(SharedMemoryBufferTests, shouldGetBuffer)
{
    initialize();
    shouldGetBuffer();
}
