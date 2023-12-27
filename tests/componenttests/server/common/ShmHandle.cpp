/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Sky UK
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

#include "ShmHandle.h"
#include <gtest/gtest.h>
#include <sys/mman.h>
#include <sys/un.h>
#include <unistd.h>

namespace firebolt::rialto::server::ct
{
ShmHandle::~ShmHandle()
{
    if (-1 == m_shmFd)
    {
        return;
    }

    EXPECT_NE(-1, munmap(m_shmBuffer, m_shmBufferLen));
    close(m_shmFd);
}

void ShmHandle::init(std::int32_t fd, std::uint32_t length)
{
    m_shmFd = fd;
    m_shmBufferLen = length;

    m_shmBuffer = reinterpret_cast<uint8_t *>(mmap(NULL, m_shmBufferLen, PROT_READ | PROT_WRITE, MAP_SHARED, m_shmFd, 0));
    EXPECT_NE(MAP_FAILED, m_shmBuffer);
    if (MAP_FAILED == m_shmBuffer)
    {
        close(m_shmFd);
        m_shmFd = -1;
        m_shmBuffer = nullptr;
        m_shmBufferLen = 0U;
    }
    std::cout << "lukewill4: fd " << fd << ", length " << m_shmBufferLen << std::endl;
}

std::uint8_t *ShmHandle::getShm() const
{
    return m_shmBuffer;
}
} // namespace firebolt::rialto::server::ct
