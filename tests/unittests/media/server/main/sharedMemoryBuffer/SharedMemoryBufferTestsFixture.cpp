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

void SharedMemoryBufferTests::initialize(int maxPlaybacks, int maxWebAudioPlayers)
{
    m_sut = firebolt::rialto::server::SharedMemoryBufferFactory().createSharedMemoryBuffer(maxPlaybacks,
                                                                                           maxWebAudioPlayers);
}

void SharedMemoryBufferTests::mapPartitionShouldSucceed(
    firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType playbackType, int id)
{
    ASSERT_TRUE(m_sut);
    EXPECT_TRUE(m_sut->mapPartition(playbackType, id));
}

void SharedMemoryBufferTests::mapPartitionShouldFail(
    firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType playbackType, int id)
{
    ASSERT_TRUE(m_sut);
    EXPECT_FALSE(m_sut->mapPartition(playbackType, id));
}

void SharedMemoryBufferTests::unmapPartitionShouldSucceed(
    firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType playbackType, int id)
{
    ASSERT_TRUE(m_sut);
    EXPECT_TRUE(m_sut->unmapPartition(playbackType, id));
}

void SharedMemoryBufferTests::unmapPartitionShouldFail(
    firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType playbackType, int id)
{
    ASSERT_TRUE(m_sut);
    EXPECT_FALSE(m_sut->unmapPartition(playbackType, id));
}

void SharedMemoryBufferTests::shouldReturnMaxGenericAudioDataLen(int id)
{
    ASSERT_TRUE(m_sut);
    EXPECT_EQ(m_sut->getMaxDataLen(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, id,
                                   firebolt::rialto::MediaSourceType::AUDIO),
              m_audioBufferLen);
}

void SharedMemoryBufferTests::shouldFailToReturnMaxAudioDataLen(
    firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType playbackType, int id)
{
    ASSERT_TRUE(m_sut);
    EXPECT_EQ(m_sut->getMaxDataLen(playbackType, id, firebolt::rialto::MediaSourceType::AUDIO), 0);
}

void SharedMemoryBufferTests::shouldReturnMaxGenericVideoDataLen(int id)
{
    ASSERT_TRUE(m_sut);
    EXPECT_EQ(m_sut->getMaxDataLen(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::GENERIC, id,
                                   firebolt::rialto::MediaSourceType::VIDEO),
              m_videoBufferLen);
}

void SharedMemoryBufferTests::shouldFailToReturnMaxVideoDataLen(
    firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType playbackType, int id)
{
    ASSERT_TRUE(m_sut);
    EXPECT_EQ(m_sut->getMaxDataLen(playbackType, id, firebolt::rialto::MediaSourceType::VIDEO), 0);
}

void SharedMemoryBufferTests::shouldReturnMaxWebAudioDataLen(int id)
{
    ASSERT_TRUE(m_sut);
    EXPECT_EQ(m_sut->getMaxDataLen(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType::WEB_AUDIO, id,
                                   firebolt::rialto::MediaSourceType::AUDIO),
              m_webAudioBufferLen);
}

void SharedMemoryBufferTests::shouldReturnVideoDataOffset(
    firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType playbackType, int id, std::uint32_t expectedOffset)
{
    ASSERT_TRUE(m_sut);
    EXPECT_EQ(m_sut->getDataOffset(playbackType, id, firebolt::rialto::MediaSourceType::VIDEO), expectedOffset);
}

void SharedMemoryBufferTests::shouldFailToReturnVideoDataOffset(
    firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType playbackType, int id)
{
    ASSERT_TRUE(m_sut);
    EXPECT_THROW(m_sut->getDataOffset(playbackType, id, firebolt::rialto::MediaSourceType::VIDEO), std::runtime_error);
}

void SharedMemoryBufferTests::shouldReturnAudioDataOffset(
    firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType playbackType, int id, std::uint32_t expectedOffset)
{
    ASSERT_TRUE(m_sut);
    EXPECT_EQ(m_sut->getDataOffset(playbackType, id, firebolt::rialto::MediaSourceType::AUDIO), expectedOffset);
}

void SharedMemoryBufferTests::shouldFailToReturnAudioDataOffset(
    firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType playbackType, int id)
{
    ASSERT_TRUE(m_sut);
    EXPECT_THROW(m_sut->getDataOffset(playbackType, id, firebolt::rialto::MediaSourceType::AUDIO), std::runtime_error);
}

void SharedMemoryBufferTests::shouldClearAudioData(
    firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType playbackType, int id)
{
    ASSERT_TRUE(m_sut);
    EXPECT_TRUE(m_sut->clearData(playbackType, id, firebolt::rialto::MediaSourceType::AUDIO));
}

void SharedMemoryBufferTests::shouldFailToClearAudioData(
    firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType playbackType, int id)
{
    ASSERT_TRUE(m_sut);
    EXPECT_FALSE(m_sut->clearData(playbackType, id, firebolt::rialto::MediaSourceType::AUDIO));
}

void SharedMemoryBufferTests::shouldClearVideoData(
    firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType playbackType, int id)
{
    ASSERT_TRUE(m_sut);
    EXPECT_TRUE(m_sut->clearData(playbackType, id, firebolt::rialto::MediaSourceType::VIDEO));
}

void SharedMemoryBufferTests::shouldFailToClearVideoData(
    firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType playbackType, int id)
{
    ASSERT_TRUE(m_sut);
    EXPECT_FALSE(m_sut->clearData(playbackType, id, firebolt::rialto::MediaSourceType::VIDEO));
}

uint8_t *
SharedMemoryBufferTests::shouldGetDataPtr(firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType playbackType,
                                          int id, const firebolt::rialto::MediaSourceType &mediaSourceType)
{
    EXPECT_TRUE(m_sut);
    return m_sut->getDataPtr(playbackType, id, mediaSourceType);
}

void SharedMemoryBufferTests::shouldFailToGetDataPtr(
    firebolt::rialto::server::ISharedMemoryBuffer::MediaPlaybackType playbackType, int id,
    const firebolt::rialto::MediaSourceType &mediaSourceType)
{
    ASSERT_TRUE(m_sut);
    EXPECT_EQ(nullptr, m_sut->getDataPtr(playbackType, id, mediaSourceType));
}

void SharedMemoryBufferTests::shouldGetFd()
{
    ASSERT_TRUE(m_sut);
    EXPECT_NE(-1, m_sut->getFd());
}

void SharedMemoryBufferTests::shouldGetSize()
{
    ASSERT_TRUE(m_sut);
    EXPECT_EQ(m_audioBufferLen + m_videoBufferLen + m_webAudioBufferLen,
              m_sut->getSize()); // Size for one session & one webaudio
}

void SharedMemoryBufferTests::shouldGetBuffer()
{
    ASSERT_TRUE(m_sut);
    EXPECT_NE(nullptr, m_sut->getBuffer());
}
