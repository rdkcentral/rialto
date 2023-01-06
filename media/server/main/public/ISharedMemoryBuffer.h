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

#ifndef FIREBOLT_RIALTO_SERVER_I_SHARED_MEMORY_BUFFER_H_
#define FIREBOLT_RIALTO_SERVER_I_SHARED_MEMORY_BUFFER_H_

#include <cstdint>
#include <memory>

#include "MediaCommon.h"

namespace firebolt::rialto::server
{
class ISharedMemoryBuffer;
class ISharedMemoryBufferFactory
{
public:
    ISharedMemoryBufferFactory() = default;
    virtual ~ISharedMemoryBufferFactory() = default;

    static std::unique_ptr<ISharedMemoryBufferFactory> createFactory();
    virtual std::shared_ptr<ISharedMemoryBuffer> createSharedMemoryBuffer(unsigned numOfPlaybacks,
                                                                          unsigned numOfWebAudioPlayers) const = 0;
};

class ISharedMemoryBuffer
{
public:
    ISharedMemoryBuffer() = default;
    virtual ~ISharedMemoryBuffer() = default;

    ISharedMemoryBuffer(const ISharedMemoryBuffer &) = delete;
    ISharedMemoryBuffer(ISharedMemoryBuffer &&) = delete;
    ISharedMemoryBuffer &operator=(const ISharedMemoryBuffer &) = delete;
    ISharedMemoryBuffer &operator=(ISharedMemoryBuffer &&) = delete;

    virtual bool mapPartition(int sessionId) = 0;
    virtual bool unmapPartition(int sessionId) = 0;

    virtual bool clearData(int sessionId, const MediaSourceType &mediaSourceType) const = 0;

    virtual std::uint32_t getDataOffset(int sessionId, const MediaSourceType &mediaSourceType) const = 0;
    virtual std::uint32_t getMaxDataLen(int sessionId, const MediaSourceType &mediaSourceType) const = 0;
    virtual std::uint8_t *getDataPtr(int sessionId, const MediaSourceType &mediaSourceType) const = 0;

    virtual int getFd() const = 0;
    virtual std::uint32_t getSize() const = 0;
    virtual std::uint8_t *getBuffer() const = 0;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_I_SHARED_MEMORY_BUFFER_H_
