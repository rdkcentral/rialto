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

TEST_F(SharedMemoryBufferTests, shouldReturnAudioBufferLen)
{
    constexpr int session1{0};
    initialize();
    mapPartitionShouldSucceed(session1);
    shouldReturnAudioBufferLen(session1);
}

TEST_F(SharedMemoryBufferTests, shouldNotReturnAudioBufferLenForNotMappedSession)
{
    constexpr int session1{0};
    initialize();
    shouldFailToReturnAudioBufferLen(session1);
}

TEST_F(SharedMemoryBufferTests, shouldReturnVideoBufferLen)
{
    constexpr int session1{0};
    initialize();
    mapPartitionShouldSucceed(session1);
    shouldReturnVideoBufferLen(session1);
}

TEST_F(SharedMemoryBufferTests, shouldNotReturnVideoBufferLenForNotMappedSession)
{
    constexpr int session1{0};
    initialize();
    shouldFailToReturnVideoBufferLen(session1);
}

TEST_F(SharedMemoryBufferTests, shouldClearAudioBuffer)
{
    constexpr int session1{0};
    initialize();
    mapPartitionShouldSucceed(session1);
    shouldClearAudioBuffer(session1);
}

TEST_F(SharedMemoryBufferTests, shouldNotClearAudioBufferForNotMappedSession)
{
    constexpr int session1{0};
    initialize();
    shouldFailToClearAudioBuffer(session1);
}

TEST_F(SharedMemoryBufferTests, shouldClearVideoBuffer)
{
    constexpr int session1{0};
    initialize();
    mapPartitionShouldSucceed(session1);
    shouldClearVideoBuffer(session1);
}

TEST_F(SharedMemoryBufferTests, shouldNotClearVideoBufferForNotMappedSession)
{
    constexpr int session1{0};
    initialize();
    shouldFailToClearVideoBuffer(session1);
}

TEST_F(SharedMemoryBufferTests, shouldFailToReturnVideoBufferOffset)
{
    constexpr int session1{0};
    initialize();
    shouldFailToReturnVideoBufferOffset(session1);
}

TEST_F(SharedMemoryBufferTests, shouldFailToReturnAudioBufferOffset)
{
    constexpr int session1{0};
    initialize();
    shouldFailToReturnAudioBufferOffset(session1);
}

TEST_F(SharedMemoryBufferTests, shouldReturnVideoBufferOffsetForOneSession)
{
    constexpr int session1{0};
    // Video buffer for first mapped session is at the beginning of shm
    constexpr std::uint32_t expectedOffset{0};
    initialize();
    mapPartitionShouldSucceed(session1);
    shouldReturnVideoBufferOffset(session1, expectedOffset);
}

TEST_F(SharedMemoryBufferTests, shouldReturnAudioBufferOffsetForOneSession)
{
    constexpr int session1{0};
    // Audio buffer for first mapped session is after video buffer - 7MB
    constexpr std::uint32_t expectedOffset{7 * 1024 * 1024};
    initialize();
    mapPartitionShouldSucceed(session1);
    shouldReturnAudioBufferOffset(session1, expectedOffset);
}

TEST_F(SharedMemoryBufferTests, shouldReturnVideoBufferOffsetForTwoSessions)
{
    constexpr int maxPlaybacks{2};
    constexpr int session1{0}, session2{1};
    // Video buffer for second mapped session is after video and audio buffer of 1st session - 7MB + 1MB = 8MB
    constexpr std::uint32_t expectedOffset{8 * 1024 * 1024};
    initialize(maxPlaybacks);
    mapPartitionShouldSucceed(session1);
    mapPartitionShouldSucceed(session2);
    shouldReturnVideoBufferOffset(session2, expectedOffset);
}

TEST_F(SharedMemoryBufferTests, shouldReturnAudioBufferOffsetForTwoSessions)
{
    constexpr int maxPlaybacks{2};
    constexpr int session1{0}, session2{1};
    // Video buffer for second mapped session is after video and audio buffer of 1st session
    // and video buffer of 2nd session - 7MB + 1MB + 7MB = 15MB
    constexpr std::uint32_t expectedOffset{15 * 1024 * 1024};
    initialize(maxPlaybacks);
    mapPartitionShouldSucceed(session1);
    mapPartitionShouldSucceed(session2);
    shouldReturnAudioBufferOffset(session2, expectedOffset);
}

TEST_F(SharedMemoryBufferTests, shouldFailToGetBufferForUnmappedSession)
{
    constexpr int session1{0};
    initialize();
    shouldFailToGetBuffer(session1, firebolt::rialto::MediaSourceType::VIDEO);
    shouldFailToGetBuffer(session1, firebolt::rialto::MediaSourceType::AUDIO);
}

TEST_F(SharedMemoryBufferTests, shouldFailToGetBufferForUnknownSourceType)
{
    constexpr int session1{0};
    initialize();
    mapPartitionShouldSucceed(session1);
    shouldFailToGetBuffer(session1, firebolt::rialto::MediaSourceType::UNKNOWN);
}

TEST_F(SharedMemoryBufferTests, shouldGetBuffer)
{
    constexpr int maxPlaybacks{2};
    constexpr int session1{0}, session2{1};
    initialize(maxPlaybacks);
    mapPartitionShouldSucceed(session1);
    mapPartitionShouldSucceed(session2);
    uint8_t *session1Video = shouldGetBuffer(session1, firebolt::rialto::MediaSourceType::VIDEO);
    uint8_t *session1Audio = shouldGetBuffer(session1, firebolt::rialto::MediaSourceType::AUDIO);
    uint8_t *session2Video = shouldGetBuffer(session2, firebolt::rialto::MediaSourceType::VIDEO);
    uint8_t *session2Audio = shouldGetBuffer(session2, firebolt::rialto::MediaSourceType::AUDIO);
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
