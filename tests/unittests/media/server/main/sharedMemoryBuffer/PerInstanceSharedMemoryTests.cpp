/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
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

#include "PerInstanceSharedMemory.h"
#include <algorithm>
#include <fcntl.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <stdexcept>
#include <sys/mman.h>
#include <sys/stat.h>

namespace firebolt::rialto::server
{
namespace
{
class SharedMemorySyscallsMock : public IPerInstanceSharedMemorySyscalls
{
public:
    MOCK_METHOD(int, create, (), (const, override));
    MOCK_METHOD(int, resize, (int fd, std::uint32_t size), (const, override));
    MOCK_METHOD(int, seal, (int fd), (const, override));
    MOCK_METHOD(void *, map, (int fd, std::uint32_t size), (const, override));
    MOCK_METHOD(int, unmap, (void *address, std::uint32_t size), (const, override));
    MOCK_METHOD(int, close, (int fd), (const, override));
};

constexpr std::uint32_t kTestSize{8U};
constexpr int kTestFd{42};
} // namespace

TEST(PerInstanceSharedMemoryTest, MediaPipelineLayoutIsFixedAndInitiallyZero)
{
    MediaPipelineSharedMemory memory;

    EXPECT_EQ(memory.getSize(), 8U * 1024U * 1024U + 256U * 1024U);
    EXPECT_EQ(memory.getDataOffset(MediaSourceType::VIDEO), 0U);
    EXPECT_EQ(memory.getDataOffset(MediaSourceType::AUDIO), 7U * 1024U * 1024U);
    EXPECT_EQ(memory.getDataOffset(MediaSourceType::SUBTITLE), 8U * 1024U * 1024U);
    EXPECT_EQ(memory.getMaxDataLen(MediaSourceType::VIDEO), 7U * 1024U * 1024U);
    EXPECT_EQ(memory.getMaxDataLen(MediaSourceType::AUDIO), 1024U * 1024U);
    EXPECT_EQ(memory.getMaxDataLen(MediaSourceType::SUBTITLE), 256U * 1024U);
    EXPECT_TRUE(std::all_of(memory.getBuffer(), memory.getBuffer() + memory.getSize(),
                            [](std::uint8_t value) { return value == 0; }));
}

TEST(PerInstanceSharedMemoryTest, ClearingOneRegionDoesNotChangeOtherRegions)
{
    MediaPipelineSharedMemory memory;
    std::fill(memory.getBuffer(), memory.getBuffer() + memory.getSize(), 0x5A);

    ASSERT_TRUE(memory.clearData(MediaSourceType::AUDIO));
    EXPECT_EQ(memory.getDataPtr(MediaSourceType::VIDEO)[0], 0x5A);
    EXPECT_EQ(memory.getDataPtr(MediaSourceType::AUDIO)[0], 0);
    EXPECT_EQ(memory.getDataPtr(MediaSourceType::SUBTITLE)[0], 0x5A);
    EXPECT_FALSE(memory.clearData(MediaSourceType::UNKNOWN));
}

TEST(PerInstanceSharedMemoryTest, AllocationsAreIndependentAndSealed)
{
    MediaPipelineSharedMemory first;
    MediaPipelineSharedMemory second;
    struct stat firstStat
    {
    };
    struct stat secondStat
    {
    };

    ASSERT_EQ(fstat(first.getFd(), &firstStat), 0);
    ASSERT_EQ(fstat(second.getFd(), &secondStat), 0);
    EXPECT_NE(firstStat.st_ino, secondStat.st_ino);
    EXPECT_EQ(fcntl(first.getFd(), F_GET_SEALS), F_SEAL_SEAL | F_SEAL_GROW | F_SEAL_SHRINK);

    first.getBuffer()[0] = 0x11;
    second.getBuffer()[0] = 0x22;
    EXPECT_EQ(first.getBuffer()[0], 0x11);
    EXPECT_EQ(second.getBuffer()[0], 0x22);
}

TEST(PerInstanceSharedMemoryTest, WebAudioLayoutIsFixed)
{
    WebAudioSharedMemory memory;

    EXPECT_EQ(memory.getSize(), 10U * 1024U);
    EXPECT_EQ(memory.getDataOffset(), 0U);
    EXPECT_EQ(memory.getMaxDataLen(), 10U * 1024U);
    EXPECT_EQ(memory.getBuffer()[0], 0);
}

TEST(PerInstanceSharedMemoryTest, CreateFailureDoesNotRequireCleanup)
{
    auto syscalls = std::make_shared<testing::StrictMock<SharedMemorySyscallsMock>>();
    EXPECT_CALL(*syscalls, create()).WillOnce(testing::Return(-1));

    EXPECT_THROW(PerInstanceSharedMemory(kTestSize, syscalls), std::runtime_error);
}

TEST(PerInstanceSharedMemoryTest, ResizeFailureClosesDescriptor)
{
    auto syscalls = std::make_shared<testing::StrictMock<SharedMemorySyscallsMock>>();
    EXPECT_CALL(*syscalls, create()).WillOnce(testing::Return(kTestFd));
    EXPECT_CALL(*syscalls, resize(kTestFd, kTestSize)).WillOnce(testing::Return(-1));
    EXPECT_CALL(*syscalls, close(kTestFd)).WillOnce(testing::Return(0));

    EXPECT_THROW(PerInstanceSharedMemory(kTestSize, syscalls), std::runtime_error);
}

TEST(PerInstanceSharedMemoryTest, SealFailureClosesDescriptor)
{
    auto syscalls = std::make_shared<testing::StrictMock<SharedMemorySyscallsMock>>();
    EXPECT_CALL(*syscalls, create()).WillOnce(testing::Return(kTestFd));
    EXPECT_CALL(*syscalls, resize(kTestFd, kTestSize)).WillOnce(testing::Return(0));
    EXPECT_CALL(*syscalls, seal(kTestFd)).WillOnce(testing::Return(-1));
    EXPECT_CALL(*syscalls, close(kTestFd)).WillOnce(testing::Return(0));

    EXPECT_THROW(PerInstanceSharedMemory(kTestSize, syscalls), std::runtime_error);
}

TEST(PerInstanceSharedMemoryTest, MappingFailureClosesDescriptor)
{
    auto syscalls = std::make_shared<testing::StrictMock<SharedMemorySyscallsMock>>();
    EXPECT_CALL(*syscalls, create()).WillOnce(testing::Return(kTestFd));
    EXPECT_CALL(*syscalls, resize(kTestFd, kTestSize)).WillOnce(testing::Return(0));
    EXPECT_CALL(*syscalls, seal(kTestFd)).WillOnce(testing::Return(0));
    EXPECT_CALL(*syscalls, map(kTestFd, kTestSize)).WillOnce(testing::Return(MAP_FAILED));
    EXPECT_CALL(*syscalls, close(kTestFd)).WillOnce(testing::Return(0));

    EXPECT_THROW(PerInstanceSharedMemory(kTestSize, syscalls), std::runtime_error);
}

TEST(PerInstanceSharedMemoryTest, DestructionUnmapsAndClosesDescriptor)
{
    std::uint8_t buffer[kTestSize]{};
    auto syscalls = std::make_shared<testing::StrictMock<SharedMemorySyscallsMock>>();
    EXPECT_CALL(*syscalls, create()).WillOnce(testing::Return(kTestFd));
    EXPECT_CALL(*syscalls, resize(kTestFd, kTestSize)).WillOnce(testing::Return(0));
    EXPECT_CALL(*syscalls, seal(kTestFd)).WillOnce(testing::Return(0));
    EXPECT_CALL(*syscalls, map(kTestFd, kTestSize)).WillOnce(testing::Return(buffer));
    EXPECT_CALL(*syscalls, unmap(buffer, kTestSize)).WillOnce(testing::Return(0));
    EXPECT_CALL(*syscalls, close(kTestFd)).WillOnce(testing::Return(0));

    PerInstanceSharedMemory memory{kTestSize, syscalls};
    EXPECT_EQ(memory.getFd(), kTestFd);
    EXPECT_EQ(memory.getSize(), kTestSize);
    EXPECT_EQ(memory.getBuffer(), buffer);
}
} // namespace firebolt::rialto::server
