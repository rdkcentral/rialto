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

#ifndef FIREBOLT_RIALTO_SERVER_PER_INSTANCE_SHARED_MEMORY_H_
#define FIREBOLT_RIALTO_SERVER_PER_INSTANCE_SHARED_MEMORY_H_

#include "IPerInstanceSharedMemory.h"
#include <cstdint>
#include <memory>

namespace firebolt::rialto::server
{
/**
 * @brief System-call boundary used to verify descriptor and mapping cleanup on every allocation failure path.
 */
class IPerInstanceSharedMemorySyscalls
{
public:
    virtual ~IPerInstanceSharedMemorySyscalls() = default;
    virtual int create() const = 0;
    virtual int resize(int fd, std::uint32_t size) const = 0;
    virtual int seal(int fd) const = 0;
    virtual void *map(int fd, std::uint32_t size) const = 0;
    virtual int unmap(void *address, std::uint32_t size) const = 0;
    virtual int close(int fd) const = 0;
};

class PerInstanceSharedMemory
{
public:
    explicit PerInstanceSharedMemory(std::uint32_t size,
                                     std::shared_ptr<IPerInstanceSharedMemorySyscalls> syscalls = nullptr);
    ~PerInstanceSharedMemory();
    int getFd() const;
    std::uint32_t getSize() const;
    std::uint8_t *getBuffer() const;

private:
    std::shared_ptr<IPerInstanceSharedMemorySyscalls> m_syscalls;
    std::uint32_t m_size{0};
    int m_fd{-1};
    std::uint8_t *m_buffer{nullptr};
};

class MediaPipelineSharedMemory : public IMediaPipelineSharedMemory
{
public:
    MediaPipelineSharedMemory();
    int getFd() const override;
    std::uint32_t getSize() const override;
    std::uint8_t *getBuffer() const override;
    bool clearData(MediaSourceType mediaSourceType) const override;
    std::uint32_t getDataOffset(MediaSourceType mediaSourceType) const override;
    std::uint32_t getMaxDataLen(MediaSourceType mediaSourceType) const override;
    std::uint8_t *getDataPtr(MediaSourceType mediaSourceType) const override;

private:
    PerInstanceSharedMemory m_memory;
};

class WebAudioSharedMemory : public IWebAudioSharedMemory
{
public:
    WebAudioSharedMemory();
    int getFd() const override;
    std::uint32_t getSize() const override;
    std::uint8_t *getBuffer() const override;
    std::uint32_t getDataOffset() const override;
    std::uint32_t getMaxDataLen() const override;

private:
    PerInstanceSharedMemory m_memory;
};

class PerInstanceSharedMemoryFactory : public IPerInstanceSharedMemoryFactory
{
public:
    std::shared_ptr<IMediaPipelineSharedMemory> createMediaPipelineSharedMemory() const override;
    std::shared_ptr<IWebAudioSharedMemory> createWebAudioSharedMemory() const override;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_PER_INSTANCE_SHARED_MEMORY_H_
