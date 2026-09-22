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
#include "RialtoServerLogging.h"
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <stdexcept>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <utility>

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
#define F_SEAL_SEAL 0x0001
#define F_SEAL_SHRINK 0x0002
#define F_SEAL_GROW 0x0004
#endif

namespace
{
constexpr std::uint32_t kVideoSize{7U * 1024U * 1024U};
constexpr std::uint32_t kAudioSize{1024U * 1024U};
constexpr std::uint32_t kSubtitleSize{256U * 1024U};
constexpr std::uint32_t kWebAudioSize{10U * 1024U};
constexpr std::uint32_t kPipelineSize{kVideoSize + kAudioSize + kSubtitleSize};

class PerInstanceSharedMemorySyscalls : public firebolt::rialto::server::IPerInstanceSharedMemorySyscalls
{
public:
    int create() const override { return syscall(SYS_memfd_create, "rialto_avbuf", MFD_CLOEXEC | MFD_ALLOW_SEALING); }

    int resize(int fd, std::uint32_t size) const override { return ftruncate(fd, static_cast<off_t>(size)); }

    int seal(int fd) const override { return fcntl(fd, F_ADD_SEALS, F_SEAL_SEAL | F_SEAL_GROW | F_SEAL_SHRINK); }

    void *map(int fd, std::uint32_t size) const override
    {
        return mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    }

    int unmap(void *address, std::uint32_t size) const override { return munmap(address, size); }

    int close(int fd) const override { return ::close(fd); }
};
} // namespace

namespace firebolt::rialto::server
{
std::shared_ptr<IPerInstanceSharedMemoryFactory> IPerInstanceSharedMemoryFactory::createFactory()
{
    return std::make_shared<PerInstanceSharedMemoryFactory>();
}

PerInstanceSharedMemory::PerInstanceSharedMemory(std::uint32_t size,
                                                 std::shared_ptr<IPerInstanceSharedMemorySyscalls> syscalls)
    : m_syscalls{syscalls ? std::move(syscalls) : std::make_shared<PerInstanceSharedMemorySyscalls>()}
{
    int fd = m_syscalls->create();
    if (fd < 0)
    {
        throw std::runtime_error("Shared Memory Buffer initialization failed");
    }

    if (m_syscalls->resize(fd, size) == -1 || m_syscalls->seal(fd) == -1)
    {
        m_syscalls->close(fd);
        throw std::runtime_error("Shared Memory Buffer initialization failed");
    }

    void *addr = m_syscalls->map(fd, size);
    if (addr == MAP_FAILED)
    {
        m_syscalls->close(fd);
        throw std::runtime_error("Shared Memory Buffer initialization failed");
    }

    m_size = size;
    m_fd = fd;
    m_buffer = static_cast<std::uint8_t *>(addr);
    RIALTO_SERVER_LOG_INFO("Shared Memory Buffer size: %u, ptr: %p", m_size, m_buffer);
}

PerInstanceSharedMemory::~PerInstanceSharedMemory()
{
    if (m_buffer && m_syscalls->unmap(m_buffer, m_size) != 0)
    {
        RIALTO_SERVER_LOG_SYS_ERROR(errno, "failed to unmap buffer");
    }
    if (m_fd >= 0 && m_syscalls->close(m_fd) != 0)
    {
        RIALTO_SERVER_LOG_SYS_ERROR(errno, "failed to close data buffer fd");
    }
}

int PerInstanceSharedMemory::getFd() const
{
    return m_fd;
}

std::uint32_t PerInstanceSharedMemory::getSize() const
{
    return m_size;
}

std::uint8_t *PerInstanceSharedMemory::getBuffer() const
{
    return m_buffer;
}

MediaPipelineSharedMemory::MediaPipelineSharedMemory() : m_memory{kPipelineSize} {}

int MediaPipelineSharedMemory::getFd() const
{
    return m_memory.getFd();
}

std::uint32_t MediaPipelineSharedMemory::getSize() const
{
    return m_memory.getSize();
}

std::uint8_t *MediaPipelineSharedMemory::getBuffer() const
{
    return m_memory.getBuffer();
}

bool MediaPipelineSharedMemory::clearData(MediaSourceType mediaSourceType) const
{
    auto data = getDataPtr(mediaSourceType);
    auto size = getMaxDataLen(mediaSourceType);
    if (!data || !size)
    {
        return false;
    }
    std::memset(data, 0, size);
    return true;
}

std::uint32_t MediaPipelineSharedMemory::getDataOffset(MediaSourceType mediaSourceType) const
{
    switch (mediaSourceType)
    {
    case MediaSourceType::VIDEO:
        return 0;
    case MediaSourceType::AUDIO:
        return kVideoSize;
    case MediaSourceType::SUBTITLE:
        return kVideoSize + kAudioSize;
    default:
        throw std::runtime_error("Unsupported media source type");
    }
}

std::uint32_t MediaPipelineSharedMemory::getMaxDataLen(MediaSourceType mediaSourceType) const
{
    switch (mediaSourceType)
    {
    case MediaSourceType::VIDEO:
        return kVideoSize;
    case MediaSourceType::AUDIO:
        return kAudioSize;
    case MediaSourceType::SUBTITLE:
        return kSubtitleSize;
    default:
        return 0;
    }
}

std::uint8_t *MediaPipelineSharedMemory::getDataPtr(MediaSourceType mediaSourceType) const
{
    auto size = getMaxDataLen(mediaSourceType);
    if (!size)
    {
        return nullptr;
    }
    return m_memory.getBuffer() + getDataOffset(mediaSourceType);
}

WebAudioSharedMemory::WebAudioSharedMemory() : m_memory{kWebAudioSize} {}

int WebAudioSharedMemory::getFd() const
{
    return m_memory.getFd();
}

std::uint32_t WebAudioSharedMemory::getSize() const
{
    return m_memory.getSize();
}

std::uint8_t *WebAudioSharedMemory::getBuffer() const
{
    return m_memory.getBuffer();
}

std::uint32_t WebAudioSharedMemory::getDataOffset() const
{
    return 0;
}

std::uint32_t WebAudioSharedMemory::getMaxDataLen() const
{
    return m_memory.getSize();
}

std::shared_ptr<IMediaPipelineSharedMemory> PerInstanceSharedMemoryFactory::createMediaPipelineSharedMemory() const
{
    return std::make_shared<MediaPipelineSharedMemory>();
}

std::shared_ptr<IWebAudioSharedMemory> PerInstanceSharedMemoryFactory::createWebAudioSharedMemory() const
{
    return std::make_shared<WebAudioSharedMemory>();
}
} // namespace firebolt::rialto::server
