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
    constexpr int session1{0};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session1);
}

TEST_F(SharedMemoryBufferTests, shouldMapWebAudioPlayer)
{
    constexpr int handle1{0};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, handle1);
}

TEST_F(SharedMemoryBufferTests, shouldSkipToMapTheSameGenericPlaybackSessionTwice)
{
    constexpr int session1{0};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session1);
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session1);
}

TEST_F(SharedMemoryBufferTests, shouldSkipToMapTheSameWebAudioPlayerTwice)
{
    constexpr int handle1{0};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, handle1);
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, handle1);
}

TEST_F(SharedMemoryBufferTests, shouldFailToMapTwoGenericPlaybackSessions)
{
    // maxSessions is set to 1
    constexpr int session1{0}, session2{1};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session1);
    mapPartitionShouldFail(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session2);
}

TEST_F(SharedMemoryBufferTests, shouldFailToMapTwoWebAudioPlayers)
{
    // maxWebAudioPlayers is set to 1
    constexpr int handle1{0}, handle2{1};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, handle1);
    mapPartitionShouldFail(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, handle2);
}

TEST_F(SharedMemoryBufferTests, shouldUnmapGenericPlaybackSession)
{
    constexpr int session1{0};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session1);
    unmapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session1);
}

TEST_F(SharedMemoryBufferTests, shouldUnmapWebAudioPlayer)
{
    constexpr int handle1{0};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, handle1);
    unmapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, handle1);
}

TEST_F(SharedMemoryBufferTests, shouldMapNewGenericPlaybackSessionAfterUnmapingTheOldOne)
{
    // maxSessions is set to 1
    constexpr int session1{0}, session2{1};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session1);
    unmapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session1);
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session2);
}

TEST_F(SharedMemoryBufferTests, shouldMapNewWebAudioPlayerAfterUnmapingTheOldOne)
{
    // maxWebAudioPlayers is set to 1
    constexpr int handle1{0}, handle2{1};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, handle1);
    unmapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, handle1);
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, handle2);
}

TEST_F(SharedMemoryBufferTests, shouldFailToUnmapNotExistingGenericPlaybackSession)
{
    constexpr int session1{0};
    initialize();
    unmapPartitionShouldFail(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session1);
}

TEST_F(SharedMemoryBufferTests, shouldFailToUnmapNotExistingWebAudioPlayer)
{
    constexpr int handle1{0};
    initialize();
    unmapPartitionShouldFail(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, handle1);
}

TEST_F(SharedMemoryBufferTests, shouldReturnMaxGenericAudioDataLen)
{
    constexpr int session1{0};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session1);
    shouldReturnMaxGenericAudioDataLen(session1);
}

TEST_F(SharedMemoryBufferTests, shouldReturnMaxWebAudioDataLen)
{
    constexpr int handle1{0};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, handle1);
    shouldReturnMaxWebAudioDataLen(handle1);
}

TEST_F(SharedMemoryBufferTests, shouldNotReturnMaxAudioDataLenForNotMappedGenericPlaybackSession)
{
    constexpr int session1{0};
    initialize();
    shouldFailToReturnMaxAudioDataLen(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session1);
}

TEST_F(SharedMemoryBufferTests, shouldNotReturnMaxAudioDataLenForNotMappedWebAudioPlayer)
{
    constexpr int handle1{0};
    initialize();
    shouldFailToReturnMaxAudioDataLen(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, handle1);
}

TEST_F(SharedMemoryBufferTests, shouldReturnMaxGenericVideoDataLen)
{
    constexpr int session1{0};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session1);
    shouldReturnMaxGenericVideoDataLen(session1);
}

TEST_F(SharedMemoryBufferTests, shouldReturnZeroForMaxWebAudioVideoDataLen)
{
    constexpr int handle1{0};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, handle1);
    shouldFailToReturnMaxVideoDataLen(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO,handle1);
}

TEST_F(SharedMemoryBufferTests, shouldNotReturnMaxVideoDataLenForNotMappedGenericPlaybackSession)
{
    constexpr int session1{0};
    initialize();
    shouldFailToReturnMaxVideoDataLen(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session1);
}

TEST_F(SharedMemoryBufferTests, shouldClearAudioDataForGenericPlaybackSession)
{
    constexpr int session1{0};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session1);
    shouldClearAudioData(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session1);
}

TEST_F(SharedMemoryBufferTests, shouldClearAudioDataForWebAudioPlayer)
{
    constexpr int handle1{0};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, handle1);
    shouldClearAudioData(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, handle1);
}

TEST_F(SharedMemoryBufferTests, shouldNotClearAudioDataForNotMappedGenericPlaybackSession)
{
    constexpr int session1{0};
    initialize();
    shouldFailToClearAudioData(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session1);
}

TEST_F(SharedMemoryBufferTests, shouldNotClearAudioDataForNotMappedWebAudioPlayer)
{
    constexpr int handle1{0};
    initialize();
    shouldFailToClearAudioData(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, handle1);
}

TEST_F(SharedMemoryBufferTests, shouldClearVideoDataForGenericPlaybackSession)
{
    constexpr int session1{0};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session1);
    shouldClearVideoData(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session1);
}

TEST_F(SharedMemoryBufferTests, shouldNotClearVideoDataForNotMappedGenericPlaybackSession)
{
    constexpr int session1{0};
    initialize();
    shouldFailToClearVideoData(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session1);
}

TEST_F(SharedMemoryBufferTests, shouldFailToReturnVideoDataOffsetForGenericPlaybackSession)
{
    constexpr int session1{0};
    initialize();
    shouldFailToReturnVideoDataOffset(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session1);
}

TEST_F(SharedMemoryBufferTests, shouldFailToReturnVideoDataOffsetForWebAudioPlayer)
{
    constexpr int handle1{0};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, handle1);
    shouldFailToReturnVideoDataOffset(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, handle1);
}

TEST_F(SharedMemoryBufferTests, shouldFailToReturnAudioDataOffsetForGenericPlaybackSession)
{
    constexpr int session1{0};
    initialize();
    shouldFailToReturnAudioDataOffset(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session1);
}

TEST_F(SharedMemoryBufferTests, shouldFailToReturnAudioDataOffsetForWebAudioPlayer)
{
    constexpr int handle1{0};
    initialize();
    shouldFailToReturnAudioDataOffset(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, handle1);
}

TEST_F(SharedMemoryBufferTests, shouldReturnVideoDataOffsetForOneGenericPlaybackSession)
{
    constexpr int session1{0};
    // Video buffer for first mapped session is at the beginning of shm
    constexpr std::uint32_t expectedOffset{0};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session1);
    shouldReturnVideoDataOffset(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session1, expectedOffset);
}

TEST_F(SharedMemoryBufferTests, shouldReturnAudioDataOffsetForOneGenericPlaybackSession)
{
    constexpr int session1{0};
    // Audio buffer for first mapped session is after video buffer
    constexpr std::uint32_t expectedOffset{m_videoBufferLen};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session1);
    shouldReturnAudioDataOffset(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session1, expectedOffset);
}

TEST_F(SharedMemoryBufferTests, shouldReturnAudioDataOffsetForOneWebAudioPlayer)
{
    constexpr int handle1{0};
    // Audio buffer for first mapped session is after generic playback session buffer
    constexpr std::uint32_t expectedOffset{m_audioBufferLen + m_videoBufferLen};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, handle1);
    shouldReturnAudioDataOffset(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, handle1, expectedOffset);
}

TEST_F(SharedMemoryBufferTests, shouldReturnVideoDataOffsetForTwoGenericPlaybackSessions)
{
    constexpr int maxPlaybacks{2};
    constexpr int session1{0}, session2{1};
    // Video buffer for second mapped session is after video and audio buffer of 1st session
    constexpr std::uint32_t expectedOffset{m_audioBufferLen + m_videoBufferLen};
    initialize(maxPlaybacks);
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session1);
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session2);
    shouldReturnVideoDataOffset(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session2, expectedOffset);
}

TEST_F(SharedMemoryBufferTests, shouldReturnAudioDataOffsetForTwoGenericPlaybackSessionsAndTwoWebAudioPlayers)
{
    constexpr int maxPlaybacks{2};
    constexpr int maxWebAudioPlayers{2};
    constexpr int session1{0}, session2{1};
    constexpr int handle1{0}, handle2{1};
    // Audio buffer for second mapped session is after video and audio buffer of 1st session
    // and video buffer of 2nd session
    constexpr std::uint32_t expectedOffsetSecondGeneric{m_audioBufferLen + 2*m_videoBufferLen};
    // Audio buffer for second mapped web audio player is after video and audio buffer of 1st and 2nd generic playback sessions
    // and the audio buffer of the 1st web audio player
    constexpr std::uint32_t expectedOffsetSecondWebAudio{2*m_audioBufferLen + 2*m_videoBufferLen + m_webAudioBufferLen};
    initialize(maxPlaybacks, maxWebAudioPlayers);
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session1);
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session2);
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, handle1);
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, handle2);
    shouldReturnAudioDataOffset(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session2, expectedOffsetSecondGeneric);
    shouldReturnAudioDataOffset(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, handle2, expectedOffsetSecondWebAudio);
}

TEST_F(SharedMemoryBufferTests, shouldFailToGetDataPtrForUnmappedGenericPlaybackSession)
{
    constexpr int session1{0};
    initialize();
    shouldFailToGetDataPtr(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session1, firebolt::rialto::MediaSourceType::VIDEO);
    shouldFailToGetDataPtr(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session1, firebolt::rialto::MediaSourceType::AUDIO);
}

TEST_F(SharedMemoryBufferTests, shouldFailToGetAudioDataPtrForUnmappedWebAudioPlayer)
{
    constexpr int handle1{0};
    initialize();
    shouldFailToGetDataPtr(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, handle1, firebolt::rialto::MediaSourceType::AUDIO);
}

TEST_F(SharedMemoryBufferTests, shouldFailToGetVideoDataPtrForWebAudioPlayer)
{
    constexpr int handle1{0};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, handle1);
    shouldFailToGetDataPtr(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, handle1, firebolt::rialto::MediaSourceType::VIDEO);
}

TEST_F(SharedMemoryBufferTests, shouldFailToGetDataPtrForUnknownSourceType)
{
    constexpr int session1{0};
    initialize();
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session1);
    shouldFailToGetDataPtr(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session1, firebolt::rialto::MediaSourceType::UNKNOWN);
}

TEST_F(SharedMemoryBufferTests, shouldGetDataPtrForGenericPlaybackSessions)
{
    constexpr int maxPlaybacks{2};
    constexpr int session1{0}, session2{1};
    initialize(maxPlaybacks);
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session1);
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session2);
    uint8_t *session1Video = shouldGetDataPtr(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session1, firebolt::rialto::MediaSourceType::VIDEO);
    uint8_t *session1Audio = shouldGetDataPtr(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session1, firebolt::rialto::MediaSourceType::AUDIO);
    uint8_t *session2Video = shouldGetDataPtr(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session2, firebolt::rialto::MediaSourceType::VIDEO);
    uint8_t *session2Audio = shouldGetDataPtr(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, session2, firebolt::rialto::MediaSourceType::AUDIO);
    EXPECT_NE(nullptr, session1Video);
    EXPECT_NE(nullptr, session1Audio);
    EXPECT_NE(nullptr, session2Video);
    EXPECT_NE(nullptr, session2Audio);
    EXPECT_EQ((session1Audio - session1Video), (m_videoBufferLen));
    EXPECT_EQ((session2Video - session1Audio), (m_audioBufferLen));
    EXPECT_EQ((session2Video - session1Video), (m_videoBufferLen + m_audioBufferLen));
    EXPECT_EQ((session2Audio - session2Video), (m_videoBufferLen));
}

TEST_F(SharedMemoryBufferTests, shouldGetAudioDataPtrForWebAudioPlayers)
{
    constexpr int maxPlaybacks{2};
    constexpr int maxWebAudioPlayers{2};
    constexpr int handle1{0}, handle2{1};
    initialize(maxPlaybacks, maxWebAudioPlayers);
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, handle1);
    mapPartitionShouldSucceed(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, handle2);
    uint8_t *handle1Audio = shouldGetDataPtr(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, handle1, firebolt::rialto::MediaSourceType::AUDIO);
    uint8_t *handle2Audio = shouldGetDataPtr(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, handle2, firebolt::rialto::MediaSourceType::AUDIO);
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
