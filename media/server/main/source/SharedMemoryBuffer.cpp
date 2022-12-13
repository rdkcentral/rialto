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

#include "SharedMemoryBuffer.h"
#include "RialtoServerLogging.h"
#include <algorithm>
#include <cstring>
#include <fcntl.h>
#include <numeric>
#include <stdexcept>
#include <sys/mman.h>
#include <syscall.h>
#include <unistd.h>

#if !defined(SYS_memfd_create)
#if defined(__NR_memfd_create)
#define SYS_memfd_create __NR_memfd_create
#elif defined(__arm__)
#define SYS_memfd_create 385
#endif
#endif

#if !defined(MFD_CLOEXEC)
#define MFD_CLOEXEC 0x0001U
#endif

#if !defined(MFD_ALLOW_SEALING)
#define MFD_ALLOW_SEALING 0x0002U
#endif

#if !defined(F_ADD_SEALS)
#if !defined(F_LINUX_SPECIFIC_BASE)
#define F_LINUX_SPECIFIC_BASE 1024
#endif
#define F_ADD_SEALS (F_LINUX_SPECIFIC_BASE + 9)
#define F_GET_SEALS (F_LINUX_SPECIFIC_BASE + 10)

#define F_SEAL_SEAL 0x0001
#define F_SEAL_SHRINK 0x0002
#define F_SEAL_GROW 0x0004
#define F_SEAL_WRITE 0x0008
#endif

namespace
{
const char *memoryBufferName{"rialto_avbuf"};
constexpr int NO_SESSION_ASSIGNED{-1};
const uint32_t videoRegionSize = 7 * 1024 * 1024; // 7MB
const uint32_t audioRegionSize = 1 * 1024 * 1024; // 1MB

std::vector<firebolt::rialto::server::SharedMemoryBuffer::Partition> calculatePartitionSize(int numOfPlaybacks)
{
    // As (for now) type of playback (for example HD or UHD) is not known, partitions have the same size.
    firebolt::rialto::server::SharedMemoryBuffer::Partition singlePlaybackDataBuffer{NO_SESSION_ASSIGNED,
                                                                                     audioRegionSize, videoRegionSize};
    return std::vector<firebolt::rialto::server::SharedMemoryBuffer::Partition>(numOfPlaybacks, singlePlaybackDataBuffer);
}
} // namespace

namespace firebolt::rialto::server
{
std::unique_ptr<ISharedMemoryBufferFactory> ISharedMemoryBufferFactory::createFactory()
{
    return std::make_unique<SharedMemoryBufferFactory>();
}

std::shared_ptr<ISharedMemoryBuffer> SharedMemoryBufferFactory::createSharedMemoryBuffer(unsigned numOfPlaybacks) const
{
    return std::make_shared<SharedMemoryBuffer>(numOfPlaybacks);
}

SharedMemoryBuffer::SharedMemoryBuffer(unsigned numOfPlaybacks)
    : m_partitions{calculatePartitionSize(numOfPlaybacks)}, m_dataBufferLen{0}, m_dataBufferFd{-1}, m_dataBuffer{nullptr}
{
    int fd = syscall(SYS_memfd_create, memoryBufferName, MFD_CLOEXEC | MFD_ALLOW_SEALING);
    if (fd < 0)
    {
        RIALTO_SERVER_LOG_SYS_ERROR(errno, "failed to create memory buffer");
    }
    else
    {
        const size_t bufferSize{calculateBufferSize()};
        if (ftruncate(fd, static_cast<off_t>(bufferSize)) == -1)
        {
            RIALTO_SERVER_LOG_SYS_ERROR(errno, "failed to resize memfd");
        }
        else if (fcntl(fd, F_ADD_SEALS, (F_SEAL_SEAL | F_SEAL_GROW | F_SEAL_SHRINK)) == -1)
        {
            RIALTO_SERVER_LOG_SYS_ERROR(errno, "failed to seal memfd");
        }
        else
        {
            void *addr = mmap(nullptr, bufferSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
            if (addr != MAP_FAILED)
            {
                m_dataBufferLen = bufferSize;
                m_dataBufferFd = fd;
                m_dataBuffer = reinterpret_cast<uint8_t *>(addr);
                RIALTO_SERVER_LOG_INFO("Shared Memory Buffer size: %d, ptr: %p", m_dataBufferLen, m_dataBuffer);
            }
        }

        if (m_dataBufferFd == -1)
        {
            if (close(fd) != 0)
                RIALTO_SERVER_LOG_SYS_ERROR(errno, "failed to close fd");
        }
    }

    if (m_dataBufferFd == -1)
    {
        RIALTO_SERVER_LOG_ERROR("Shared Memory Buffer initialization failed");
        throw std::runtime_error("Shared Memory Buffer initialization failed");
    }
}

SharedMemoryBuffer::~SharedMemoryBuffer()
{
    RIALTO_SERVER_LOG_INFO("Destroying Shared Memory Buffer");
    if (m_dataBufferFd != -1)
    {
        if (munmap(m_dataBuffer, m_dataBufferLen) != 0)
            RIALTO_SERVER_LOG_SYS_ERROR(errno, "failed to unmap buffer");
        if (close(m_dataBufferFd) != 0)
            RIALTO_SERVER_LOG_SYS_ERROR(errno, "failed to close data buffer fd");

        m_dataBufferLen = 0;
        m_dataBufferFd = -1;
        m_dataBuffer = nullptr;
    }
}

bool SharedMemoryBuffer::mapPartition(int sessionId)
{
    auto sessionPartition = std::find_if(m_partitions.begin(), m_partitions.end(),
                                         [sessionId](const auto &p) { return p.sessionId == sessionId; });
    if (sessionPartition != m_partitions.end())
    {
        RIALTO_SERVER_LOG_DEBUG("Skip to map shm partition for session: %d. - partition already assigned", sessionId);
        return true;
    }
    auto freePartition = std::find_if(m_partitions.begin(), m_partitions.end(),
                                      [](const auto &p) { return p.sessionId == NO_SESSION_ASSIGNED; });
    if (freePartition == m_partitions.end())
    {
        RIALTO_SERVER_LOG_ERROR("Failed to map Shm partition for session: %d. No free partition available.", sessionId);
        return false;
    }
    freePartition->sessionId = sessionId;
    return true;
}

bool SharedMemoryBuffer::unmapPartition(int sessionId)
{
    auto sessionPartition = std::find_if(m_partitions.begin(), m_partitions.end(),
                                         [sessionId](const auto &p) { return p.sessionId == sessionId; });
    if (sessionPartition == m_partitions.end())
    {
        RIALTO_SERVER_LOG_WARN("Failed to unmap Shm partition for session: %d. - partition could not be found",
                               sessionId);
        return false;
    }
    sessionPartition->sessionId = NO_SESSION_ASSIGNED;
    return true;
}

bool SharedMemoryBuffer::clearData(int sessionId, const MediaSourceType &mediaSourceType) const
{
    auto sessionPartition = std::find_if(m_partitions.begin(), m_partitions.end(),
                                         [sessionId](const auto &p) { return p.sessionId == sessionId; });
    if (sessionPartition == m_partitions.end())
    {
        RIALTO_SERVER_LOG_WARN("Failed to clear data for session: %d. - partition could not be found", sessionId);
        return false;
    }
    if (MediaSourceType::AUDIO == mediaSourceType)
    {
        std::uint8_t *audioData = getDataPtrForSession(sessionId) + sessionPartition->dataBufferVideoLen;
        memset(audioData, 0x00, sessionPartition->dataBufferAudioLen);
        return true;
    }
    if (MediaSourceType::VIDEO == mediaSourceType)
    {
        std::uint8_t *videoData = getDataPtrForSession(sessionId);
        memset(videoData, 0x00, sessionPartition->dataBufferVideoLen);
        return true;
    }
    return false;
}

std::uint32_t SharedMemoryBuffer::getDataOffset(int sessionId, const MediaSourceType &mediaSourceType) const
{
    std::uint8_t *sessionBuffer = getDataPtr(sessionId, mediaSourceType);
    if (!sessionBuffer)
    {
        throw std::runtime_error("Buffer not found for session: " + std::to_string(sessionId));
    }
    return sessionBuffer - m_dataBuffer;
}

std::uint32_t SharedMemoryBuffer::getMaxDataLen(int sessionId, const MediaSourceType &mediaSourceType) const
{
    auto sessionPartition = std::find_if(m_partitions.begin(), m_partitions.end(),
                                         [sessionId](const auto &p) { return p.sessionId == sessionId; });
    if (sessionPartition == m_partitions.end())
    {
        RIALTO_SERVER_LOG_WARN("Failed to get buffer length for session: %d. - partition could not be found", sessionId);
        return 0;
    }
    if (MediaSourceType::AUDIO == mediaSourceType)
    {
        return sessionPartition->dataBufferAudioLen;
    }
    if (MediaSourceType::VIDEO == mediaSourceType)
    {
        return sessionPartition->dataBufferVideoLen;
    }
    return 0;
}

std::uint8_t *SharedMemoryBuffer::getDataPtr(int sessionId, const MediaSourceType &mediaSourceType) const
{
    auto sessionPartition = std::find_if(m_partitions.begin(), m_partitions.end(),
                                         [sessionId](const auto &p) { return p.sessionId == sessionId; });
    if (sessionPartition == m_partitions.end())
    {
        RIALTO_SERVER_LOG_WARN("Failed to get buffer offset for session: %d. - partition could not be found", sessionId);
        return nullptr;
    }
    if (MediaSourceType::AUDIO == mediaSourceType)
    {
        return getDataPtrForSession(sessionId) + sessionPartition->dataBufferVideoLen;
    }
    if (MediaSourceType::VIDEO == mediaSourceType)
    {
        return getDataPtrForSession(sessionId);
    }
    return nullptr;
}

int SharedMemoryBuffer::getFd() const
{
    return m_dataBufferFd;
}

std::uint32_t SharedMemoryBuffer::getSize() const
{
    return m_dataBufferLen;
}

std::uint8_t *SharedMemoryBuffer::getBuffer() const
{
    return m_dataBuffer;
}

size_t SharedMemoryBuffer::calculateBufferSize() const
{
    return std::accumulate(m_partitions.begin(), m_partitions.end(), 0,
                           [](size_t sum, const Partition &p)
                           { return sum + p.dataBufferAudioLen + p.dataBufferVideoLen; });
}

std::uint8_t *SharedMemoryBuffer::getDataPtrForSession(int sessionId) const
{
    // IMPORTANT: Function assumes, that session exists in Partitions vector!
    std::uint8_t *result = m_dataBuffer;
    for (const auto &partition : m_partitions)
    {
        if (partition.sessionId == sessionId)
        {
            break;
        }
        result += (partition.dataBufferVideoLen + partition.dataBufferAudioLen);
    }
    return result;
}
} // namespace firebolt::rialto::server
