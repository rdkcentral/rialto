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

#include "ISharedMemoryHandle.h"
#include <cerrno>
#include <fcntl.h>
#include <gtest/gtest.h>
#include <stdexcept>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace firebolt::rialto::client
{
namespace
{
constexpr std::uint32_t kMappingSize{4096U};

int createSharedMemory(std::uint32_t size)
{
    int fd = syscall(SYS_memfd_create, "rialto_client_test", MFD_CLOEXEC);
    if (fd >= 0 && ftruncate(fd, static_cast<off_t>(size)) != 0)
    {
        close(fd);
        return -1;
    }
    return fd;
}
} // namespace

TEST(SharedMemoryHandleTest, MapsAndOwnsValidDescriptor)
{
    int fd = createSharedMemory(kMappingSize);
    ASSERT_GE(fd, 0);

    auto handle = ISharedMemoryHandle::create(fd, kMappingSize);
    ASSERT_NE(handle, nullptr);
    ASSERT_NE(handle->getShm(), nullptr);
    handle->getShm()[0] = 0x5A;
    EXPECT_EQ(handle->getShm()[0], 0x5A);

    handle.reset();
    errno = 0;
    EXPECT_EQ(fcntl(fd, F_GETFD), -1);
    EXPECT_EQ(errno, EBADF);
}

TEST(SharedMemoryHandleTest, RejectsZeroSizeAndClosesDescriptor)
{
    int fd = createSharedMemory(kMappingSize);
    ASSERT_GE(fd, 0);

    EXPECT_THROW(ISharedMemoryHandle::create(fd, 0U), std::runtime_error);
    errno = 0;
    EXPECT_EQ(fcntl(fd, F_GETFD), -1);
    EXPECT_EQ(errno, EBADF);
}

TEST(SharedMemoryHandleTest, RejectsAdvertisedSizeLargerThanObjectAndClosesDescriptor)
{
    int fd = createSharedMemory(kMappingSize);
    ASSERT_GE(fd, 0);

    EXPECT_THROW(ISharedMemoryHandle::create(fd, kMappingSize + 1U), std::runtime_error);
    errno = 0;
    EXPECT_EQ(fcntl(fd, F_GETFD), -1);
    EXPECT_EQ(errno, EBADF);
}

TEST(SharedMemoryHandleTest, RejectsInvalidDescriptor)
{
    EXPECT_THROW(ISharedMemoryHandle::create(-1, kMappingSize), std::runtime_error);
}
} // namespace firebolt::rialto::client
