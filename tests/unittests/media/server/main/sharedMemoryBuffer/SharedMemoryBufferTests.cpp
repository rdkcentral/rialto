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

TEST_F(SharedMemoryBufferTests, shouldMapGenericPlaybackSession)
{
    constexpr int kSession1{0};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSession1);
}

TEST_F(SharedMemoryBufferTests, shouldMapWebAudioPlayer)
{
    constexpr int kHandle1{0};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, kHandle1);
}

TEST_F(SharedMemoryBufferTests, shouldSkipToMapTheSameGenericPlaybackSessionTwice)
{
    constexpr int kSession1{0};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSession1);
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSession1);
}

TEST_F(SharedMemoryBufferTests, shouldSkipToMapTheSameWebAudioPlayerTwice)
{
    constexpr int kHandle1{0};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, kHandle1);
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, kHandle1);
}

TEST_F(SharedMemoryBufferTests, shouldFailToMapTwoGenericPlaybackSessions)
{
    // maxSessions is set to 1
    constexpr int kSession1{0}, kSession2{1};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSession1);
    mapPartitionShouldFail(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSession2);
}

TEST_F(SharedMemoryBufferTests, shouldFailToMapTwoWebAudioPlayers)
{
    // maxWebAudioPlayers is set to 1
    constexpr int kHandle1{0}, kHandle2{1};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, kHandle1);
    mapPartitionShouldFail(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, kHandle2);
}

TEST_F(SharedMemoryBufferTests, shouldUnmapGenericPlaybackSession)
{
    constexpr int kSession1{0};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSession1);
    unmapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSession1);
}

TEST_F(SharedMemoryBufferTests, shouldUnmapWebAudioPlayer)
{
    constexpr int kHandle1{0};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, kHandle1);
    unmapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, kHandle1);
}

TEST_F(SharedMemoryBufferTests, shouldMapNewGenericPlaybackSessionAfterUnmapingTheOldOne)
{
    // maxSessions is set to 1
    constexpr int kSession1{0}, kSession2{1};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSession1);
    unmapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSession1);
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSession2);
}

TEST_F(SharedMemoryBufferTests, shouldMapNewWebAudioPlayerAfterUnmapingTheOldOne)
{
    // maxWebAudioPlayers is set to 1
    constexpr int kHandle1{0}, kHandle2{1};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, kHandle1);
    unmapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, kHandle1);
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, kHandle2);
}

TEST_F(SharedMemoryBufferTests, shouldFailToUnmapNotExistingGenericPlaybackSession)
{
    constexpr int kSession1{0};
    initialize();
    unmapPartitionShouldFail(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSession1);
}

TEST_F(SharedMemoryBufferTests, shouldFailToUnmapNotExistingWebAudioPlayer)
{
    constexpr int kHandle1{0};
    initialize();
    unmapPartitionShouldFail(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, kHandle1);
}

TEST_F(SharedMemoryBufferTests, shouldReturnMaxGenericAudioDataLen)
{
    constexpr int kSession1{0};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSession1);
    shouldReturnMaxGenericAudioDataLen(kSession1);
}

TEST_F(SharedMemoryBufferTests, shouldReturnMaxWebAudioDataLen)
{
    constexpr int kHandle1{0};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, kHandle1);
    shouldReturnMaxWebAudioDataLen(kHandle1);
}

TEST_F(SharedMemoryBufferTests, shouldNotReturnMaxAudioDataLenForNotMappedGenericPlaybackSession)
{
    constexpr int kSession1{0};
    initialize();
    shouldFailToReturnMaxAudioDataLen(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC,
                                      kSession1);
}

TEST_F(SharedMemoryBufferTests, shouldNotReturnMaxAudioDataLenForNotMappedWebAudioPlayer)
{
    constexpr int kHandle1{0};
    initialize();
    shouldFailToReturnMaxAudioDataLen(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO,
                                      kHandle1);
}

TEST_F(SharedMemoryBufferTests, shouldReturnMaxGenericVideoDataLen)
{
    constexpr int kSession1{0};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSession1);
    shouldReturnMaxGenericVideoDataLen(kSession1);
}

TEST_F(SharedMemoryBufferTests, shouldReturnZeroForMaxWebAudioVideoDataLen)
{
    constexpr int kHandle1{0};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, kHandle1);
    shouldFailToReturnMaxVideoDataLen(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO,
                                      kHandle1);
}

TEST_F(SharedMemoryBufferTests, shouldNotReturnMaxVideoDataLenForNotMappedGenericPlaybackSession)
{
    constexpr int kSession1{0};
    initialize();
    shouldFailToReturnMaxVideoDataLen(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC,
                                      kSession1);
}

TEST_F(SharedMemoryBufferTests, shouldClearAudioDataForGenericPlaybackSession)
{
    constexpr int kSession1{0};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSession1);
    shouldClearAudioData(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSession1);
}

TEST_F(SharedMemoryBufferTests, shouldClearAudioDataForWebAudioPlayer)
{
    constexpr int kHandle1{0};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, kHandle1);
    shouldClearAudioData(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, kHandle1);
}

TEST_F(SharedMemoryBufferTests, shouldNotClearAudioDataForNotMappedGenericPlaybackSession)
{
    constexpr int kSession1{0};
    initialize();
    shouldFailToClearAudioData(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSession1);
}

TEST_F(SharedMemoryBufferTests, shouldNotClearAudioDataForNotMappedWebAudioPlayer)
{
    constexpr int kHandle1{0};
    initialize();
    shouldFailToClearAudioData(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, kHandle1);
}

TEST_F(SharedMemoryBufferTests, shouldClearVideoDataForGenericPlaybackSession)
{
    constexpr int kSession1{0};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSession1);
    shouldClearVideoData(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSession1);
}

TEST_F(SharedMemoryBufferTests, shouldNotClearVideoDataForNotMappedGenericPlaybackSession)
{
    constexpr int kSession1{0};
    initialize();
    shouldFailToClearVideoData(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSession1);
}

TEST_F(SharedMemoryBufferTests, shouldFailToReturnVideoDataOffsetForGenericPlaybackSession)
{
    constexpr int kSession1{0};
    initialize();
    shouldFailToReturnVideoDataOffset(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC,
                                      kSession1);
}

TEST_F(SharedMemoryBufferTests, shouldFailToReturnVideoDataOffsetForWebAudioPlayer)
{
    constexpr int kHandle1{0};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, kHandle1);
    shouldFailToReturnVideoDataOffset(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC,
                                      kHandle1);
}

TEST_F(SharedMemoryBufferTests, shouldFailToReturnAudioDataOffsetForGenericPlaybackSession)
{
    constexpr int kSession1{0};
    initialize();
    shouldFailToReturnAudioDataOffset(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC,
                                      kSession1);
}

TEST_F(SharedMemoryBufferTests, shouldFailToReturnAudioDataOffsetForWebAudioPlayer)
{
    constexpr int kHandle1{0};
    initialize();
    shouldFailToReturnAudioDataOffset(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO,
                                      kHandle1);
}

TEST_F(SharedMemoryBufferTests, shouldReturnVideoDataOffsetForOneGenericPlaybackSession)
{
    constexpr int kSession1{0};
    // Video buffer for first mapped session is at the beginning of shm
    constexpr std::uint32_t kExpectedOffset{0};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSession1);
    shouldReturnVideoDataOffset(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSession1,
                                kExpectedOffset);
}

TEST_F(SharedMemoryBufferTests, shouldReturnAudioDataOffsetForOneGenericPlaybackSession)
{
    constexpr int kSession1{0};
    // Audio buffer for first mapped session is after video buffer
    constexpr std::uint32_t kExpectedOffset{m_videoBufferLen};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSession1);
    shouldReturnAudioDataOffset(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSession1,
                                kExpectedOffset);
}

TEST_F(SharedMemoryBufferTests, shouldReturnAudioDataOffsetForOneWebAudioPlayer)
{
    constexpr int kHandle1{0};
    // Audio buffer for first mapped session is after generic playback session buffer
    constexpr std::uint32_t kExpectedOffset{m_audioBufferLen + m_videoBufferLen + m_subtitleBufferLen};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, kHandle1);
    shouldReturnAudioDataOffset(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, kHandle1,
                                kExpectedOffset);
}

TEST_F(SharedMemoryBufferTests, shouldReturnVideoDataOffsetForTwoGenericPlaybackSessions)
{
    constexpr int kMaxPlaybacks{2};
    constexpr int kSession1{0}, kSession2{1};
    // Video buffer for second mapped session is after video and audio buffer of 1st session
    constexpr std::uint32_t kExpectedOffset{m_audioBufferLen + m_videoBufferLen + m_subtitleBufferLen};
    initialize(kMaxPlaybacks);
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSession1);
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSession2);
    shouldReturnVideoDataOffset(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSession2,
                                kExpectedOffset);
}

TEST_F(SharedMemoryBufferTests, shouldReturnAudioDataOffsetForTwoGenericPlaybackSessionsAndTwoWebAudioPlayers)
{
    constexpr int kMaxPlaybacks{2};
    constexpr int kMaxWebAudioPlayers{2};
    constexpr int kSession1{0}, kSession2{1};
    constexpr int kHandle1{0}, kHandle2{1};
    // Audio buffer for second mapped session is after video and audio buffer of 1st session
    // and video buffer of 2nd session
    constexpr std::uint32_t kExpectedOffsetSecondGeneric{m_audioBufferLen + m_videoBufferLen + m_subtitleBufferLen + m_videoBufferLen};
    // Audio buffer for second mapped web audio player is after video and audio buffer of 1st and 2nd generic playback
    // sessions and the audio buffer of the 1st web audio player
    constexpr std::uint32_t kExpectedOffsetSecondWebAudio{2 * m_audioBufferLen + 2 * m_videoBufferLen + 2 * m_subtitleBufferLen +
                                                          m_webAudioBufferLen};
    initialize(kMaxPlaybacks, kMaxWebAudioPlayers);
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSession1);
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSession2);
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, kHandle1);
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, kHandle2);
    shouldReturnAudioDataOffset(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSession2,
                                kExpectedOffsetSecondGeneric);
    shouldReturnAudioDataOffset(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, kHandle2,
                                kExpectedOffsetSecondWebAudio);
}

TEST_F(SharedMemoryBufferTests, shouldFailToGetDataPtrForUnmappedGenericPlaybackSession)
{
    constexpr int kSession1{0};
    initialize();
    shouldFailToGetDataPtr(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSession1,
                           firebolt::rialto::MediaSourceType::VIDEO);
    shouldFailToGetDataPtr(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSession1,
                           firebolt::rialto::MediaSourceType::AUDIO);
}

TEST_F(SharedMemoryBufferTests, shouldFailToGetAudioDataPtrForUnmappedWebAudioPlayer)
{
    constexpr int kHandle1{0};
    initialize();
    shouldFailToGetDataPtr(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, kHandle1,
                           firebolt::rialto::MediaSourceType::AUDIO);
}

TEST_F(SharedMemoryBufferTests, shouldFailToGetVideoDataPtrForWebAudioPlayer)
{
    constexpr int kHandle1{0};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, kHandle1);
    shouldFailToGetDataPtr(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, kHandle1,
                           firebolt::rialto::MediaSourceType::VIDEO);
}

TEST_F(SharedMemoryBufferTests, shouldFailToGetDataPtrForUnknownSourceType)
{
    constexpr int kSession1{0};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSession1);
    shouldFailToGetDataPtr(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSession1,
                           firebolt::rialto::MediaSourceType::UNKNOWN);
}

TEST_F(SharedMemoryBufferTests, shouldGetDataPtrForGenericPlaybackSessions)
{
    constexpr int kMaxPlaybacks{2};
    constexpr int kSession1{0}, kSession2{1};
    initialize(kMaxPlaybacks);
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSession1);
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, kSession2);
    uint8_t *session1Video = shouldGetDataPtr(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC,
                                              kSession1, firebolt::rialto::MediaSourceType::VIDEO);
    uint8_t *session1Audio = shouldGetDataPtr(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC,
                                              kSession1, firebolt::rialto::MediaSourceType::AUDIO);
    uint8_t *session1Subtitle = shouldGetDataPtr(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC,
                                                 kSession1, firebolt::rialto::MediaSourceType::SUBTITLE);
    uint8_t *session2Video = shouldGetDataPtr(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC,
                                              kSession2, firebolt::rialto::MediaSourceType::VIDEO);
    uint8_t *session2Audio = shouldGetDataPtr(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC,
                                              kSession2, firebolt::rialto::MediaSourceType::AUDIO);
    uint8_t *session2Subtitle = shouldGetDataPtr(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC,
                                                 kSession2, firebolt::rialto::MediaSourceType::SUBTITLE);
    EXPECT_NE(nullptr, session1Video);
    EXPECT_NE(nullptr, session1Audio);
    EXPECT_NE(nullptr, session1Subtitle);
    EXPECT_NE(nullptr, session2Video);
    EXPECT_NE(nullptr, session2Audio);
    EXPECT_NE(nullptr, session2Subtitle);
    EXPECT_EQ((session1Audio - session1Video), (m_videoBufferLen));
    EXPECT_EQ((session1Subtitle - session1Audio), m_audioBufferLen);
    EXPECT_EQ((session2Video - session1Subtitle), (m_subtitleBufferLen));
    EXPECT_EQ((session2Video - session1Video), (m_videoBufferLen + m_audioBufferLen + m_subtitleBufferLen));
    EXPECT_EQ((session2Audio - session2Video), (m_videoBufferLen));
}

TEST_F(SharedMemoryBufferTests, shouldGetAudioDataPtrForWebAudioPlayers)
{
    constexpr int kMaxPlaybacks{2};
    constexpr int kMaxWebAudioPlayers{2};
    constexpr int kHandle1{0}, kHandle2{1};
    initialize(kMaxPlaybacks, kMaxWebAudioPlayers);
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, kHandle1);
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, kHandle2);
    uint8_t *handle1Audio = shouldGetDataPtr(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO,
                                             kHandle1, firebolt::rialto::MediaSourceType::AUDIO);
    uint8_t *handle2Audio = shouldGetDataPtr(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO,
                                             kHandle2, firebolt::rialto::MediaSourceType::AUDIO);
    EXPECT_NE(nullptr, handle1Audio);
    EXPECT_NE(nullptr, handle2Audio);
    EXPECT_EQ((handle2Audio - handle1Audio), (m_webAudioBufferLen));
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
