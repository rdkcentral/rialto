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
constexpr int maxPlaybacks{1};
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

void SharedMemoryBufferTests::shouldReturnAudioBufferLen(int sessionId)
{
    ASSERT_TRUE(m_sut);
    EXPECT_EQ(m_sut->getBufferLen(sessionId, firebolt::rialto::MediaSourceType::AUDIO), audioBufferLen);
}

void SharedMemoryBufferTests::shouldFailToReturnAudioBufferLen(int sessionId)
{
    ASSERT_TRUE(m_sut);
    EXPECT_EQ(m_sut->getBufferLen(sessionId, firebolt::rialto::MediaSourceType::AUDIO), 0);
}

void SharedMemoryBufferTests::shouldReturnVideoBufferLen(int sessionId)
{
    ASSERT_TRUE(m_sut);
    EXPECT_EQ(m_sut->getBufferLen(sessionId, firebolt::rialto::MediaSourceType::VIDEO), videoBufferLen);
}

void SharedMemoryBufferTests::shouldFailToReturnVideoBufferLen(int sessionId)
{
    ASSERT_TRUE(m_sut);
    EXPECT_EQ(m_sut->getBufferLen(sessionId, firebolt::rialto::MediaSourceType::VIDEO), 0);
}

void SharedMemoryBufferTests::shouldReturnVideoBufferOffset(int sessionId, std::uint32_t expectedOffset)
{
    ASSERT_TRUE(m_sut);
    EXPECT_EQ(m_sut->getBufferOffset(sessionId, firebolt::rialto::MediaSourceType::VIDEO), expectedOffset);
}

void SharedMemoryBufferTests::shouldFailToReturnVideoBufferOffset(int sessionId)
{
    ASSERT_TRUE(m_sut);
    EXPECT_THROW(m_sut->getBufferOffset(sessionId, firebolt::rialto::MediaSourceType::VIDEO), std::runtime_error);
}

void SharedMemoryBufferTests::shouldReturnAudioBufferOffset(int sessionId, std::uint32_t expectedOffset)
{
    ASSERT_TRUE(m_sut);
    EXPECT_EQ(m_sut->getBufferOffset(sessionId, firebolt::rialto::MediaSourceType::AUDIO), expectedOffset);
}

void SharedMemoryBufferTests::shouldFailToReturnAudioBufferOffset(int sessionId)
{
    ASSERT_TRUE(m_sut);
    EXPECT_THROW(m_sut->getBufferOffset(sessionId, firebolt::rialto::MediaSourceType::AUDIO), std::runtime_error);
}

void SharedMemoryBufferTests::shouldClearAudioBuffer(int sessionId)
{
    ASSERT_TRUE(m_sut);
    EXPECT_TRUE(m_sut->clearBuffer(sessionId, firebolt::rialto::MediaSourceType::AUDIO));
}

void SharedMemoryBufferTests::shouldFailToClearAudioBuffer(int sessionId)
{
    ASSERT_TRUE(m_sut);
    EXPECT_FALSE(m_sut->clearBuffer(sessionId, firebolt::rialto::MediaSourceType::AUDIO));
}

void SharedMemoryBufferTests::shouldClearVideoBuffer(int sessionId)
{
    ASSERT_TRUE(m_sut);
    EXPECT_TRUE(m_sut->clearBuffer(sessionId, firebolt::rialto::MediaSourceType::VIDEO));
}

void SharedMemoryBufferTests::shouldFailToClearVideoBuffer(int sessionId)
{
    ASSERT_TRUE(m_sut);
    EXPECT_FALSE(m_sut->clearBuffer(sessionId, firebolt::rialto::MediaSourceType::VIDEO));
}

uint8_t *SharedMemoryBufferTests::shouldGetBuffer(int sessionId, const firebolt::rialto::MediaSourceType &mediaSourceType)
{
    EXPECT_TRUE(m_sut);
    return m_sut->getBuffer(sessionId, mediaSourceType);
}

void SharedMemoryBufferTests::shouldFailToGetBuffer(int sessionId,
                                                    const firebolt::rialto::MediaSourceType &mediaSourceType)
{
    ASSERT_TRUE(m_sut);
    EXPECT_EQ(nullptr, m_sut->getBuffer(sessionId, mediaSourceType));
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
