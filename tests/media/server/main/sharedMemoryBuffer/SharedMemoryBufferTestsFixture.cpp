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

namespace
{
constexpr std::uint32_t audioBufferLen{1 * 1024 * 1024}; // 1MB
constexpr std::uint32_t videoBufferLen{7 * 1024 * 1024}; // 7MB
} // namespace

void SharedMemoryBufferTests::initialize(int maxPlaybacks)
{
    m_sut = firebolt::rialto::server::SharedMemoryBufferFactory().createSharedMemoryBuffer(maxPlaybacks);
}

void SharedMemoryBufferTests::mapPartitionShouldSucceed(int sessionId)
{
    ASSERT_TRUE(m_sut);
    EXPECT_TRUE(m_sut->mapPartition(sessionId));
}

void SharedMemoryBufferTests::mapPartitionShouldFail(int sessionId)
{
    ASSERT_TRUE(m_sut);
    EXPECT_FALSE(m_sut->mapPartition(sessionId));
}

void SharedMemoryBufferTests::unmapPartitionShouldSucceed(int sessionId)
{
    ASSERT_TRUE(m_sut);
    EXPECT_TRUE(m_sut->unmapPartition(sessionId));
}

void SharedMemoryBufferTests::unmapPartitionShouldFail(int sessionId)
{
    ASSERT_TRUE(m_sut);
    EXPECT_FALSE(m_sut->unmapPartition(sessionId));
}

void SharedMemoryBufferTests::shouldReturnMaxAudioDataLen(int sessionId)
{
    ASSERT_TRUE(m_sut);
    EXPECT_EQ(m_sut->getMaxDataLen(sessionId, firebolt::rialto::MediaSourceType::AUDIO), audioBufferLen);
}

void SharedMemoryBufferTests::shouldFailToReturnMaxAudioDataLen(int sessionId)
{
    ASSERT_TRUE(m_sut);
    EXPECT_EQ(m_sut->getMaxDataLen(sessionId, firebolt::rialto::MediaSourceType::AUDIO), 0);
}

void SharedMemoryBufferTests::shouldReturnMaxVideoDataLen(int sessionId)
{
    ASSERT_TRUE(m_sut);
    EXPECT_EQ(m_sut->getMaxDataLen(sessionId, firebolt::rialto::MediaSourceType::VIDEO), videoBufferLen);
}

void SharedMemoryBufferTests::shouldFailToReturnMaxVideoDataLen(int sessionId)
{
    ASSERT_TRUE(m_sut);
    EXPECT_EQ(m_sut->getMaxDataLen(sessionId, firebolt::rialto::MediaSourceType::VIDEO), 0);
}

void SharedMemoryBufferTests::shouldReturnVideoDataOffset(int sessionId, std::uint32_t expectedOffset)
{
    ASSERT_TRUE(m_sut);
    EXPECT_EQ(m_sut->getDataOffset(sessionId, firebolt::rialto::MediaSourceType::VIDEO), expectedOffset);
}

void SharedMemoryBufferTests::shouldFailToReturnVideoDataOffset(int sessionId)
{
    ASSERT_TRUE(m_sut);
    EXPECT_THROW(m_sut->getDataOffset(sessionId, firebolt::rialto::MediaSourceType::VIDEO), std::runtime_error);
}

void SharedMemoryBufferTests::shouldReturnAudioDataOffset(int sessionId, std::uint32_t expectedOffset)
{
    ASSERT_TRUE(m_sut);
    EXPECT_EQ(m_sut->getDataOffset(sessionId, firebolt::rialto::MediaSourceType::AUDIO), expectedOffset);
}

void SharedMemoryBufferTests::shouldFailToReturnAudioDataOffset(int sessionId)
{
    ASSERT_TRUE(m_sut);
    EXPECT_THROW(m_sut->getDataOffset(sessionId, firebolt::rialto::MediaSourceType::AUDIO), std::runtime_error);
}

void SharedMemoryBufferTests::shouldClearAudioData(int sessionId)
{
    ASSERT_TRUE(m_sut);
    EXPECT_TRUE(m_sut->clearData(sessionId, firebolt::rialto::MediaSourceType::AUDIO));
}

void SharedMemoryBufferTests::shouldFailToClearAudioData(int sessionId)
{
    ASSERT_TRUE(m_sut);
    EXPECT_FALSE(m_sut->clearData(sessionId, firebolt::rialto::MediaSourceType::AUDIO));
}

void SharedMemoryBufferTests::shouldClearVideoData(int sessionId)
{
    ASSERT_TRUE(m_sut);
    EXPECT_TRUE(m_sut->clearData(sessionId, firebolt::rialto::MediaSourceType::VIDEO));
}

void SharedMemoryBufferTests::shouldFailToClearVideoData(int sessionId)
{
    ASSERT_TRUE(m_sut);
    EXPECT_FALSE(m_sut->clearData(sessionId, firebolt::rialto::MediaSourceType::VIDEO));
}

uint8_t *SharedMemoryBufferTests::shouldGetDataPtr(int sessionId, const firebolt::rialto::MediaSourceType &mediaSourceType)
{
    EXPECT_TRUE(m_sut);
    return m_sut->getDataPtr(sessionId, mediaSourceType);
}

void SharedMemoryBufferTests::shouldFailToGetDataPtr(int sessionId,
                                                     const firebolt::rialto::MediaSourceType &mediaSourceType)
{
    ASSERT_TRUE(m_sut);
    EXPECT_EQ(nullptr, m_sut->getDataPtr(sessionId, mediaSourceType));
}

void SharedMemoryBufferTests::shouldGetFd()
{
    ASSERT_TRUE(m_sut);
    EXPECT_NE(-1, m_sut->getFd());
}

void SharedMemoryBufferTests::shouldGetSize()
{
    ASSERT_TRUE(m_sut);
    EXPECT_EQ(8 * 1024 * 1024, m_sut->getSize()); // Size for one session should be 8 MB
}

void SharedMemoryBufferTests::shouldGetBuffer()
{
    ASSERT_TRUE(m_sut);
    EXPECT_NE(nullptr, m_sut->getBuffer());
}
