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

#ifndef FIREBOLT_RIALTO_CLIENT_I_SHARED_MEMORY_HANDLE_H_
#define FIREBOLT_RIALTO_CLIENT_I_SHARED_MEMORY_HANDLE_H_

#include <cstdint>

namespace firebolt::rialto::client
{
class ISharedMemoryHandle
{
public:
    ISharedMemoryHandle() = default;
    ISharedMemoryHandle(const ISharedMemoryHandle &) = delete;
    ISharedMemoryHandle(ISharedMemoryHandle &&) = delete;
    ISharedMemoryHandle &operator=(const ISharedMemoryHandle &) = delete;
    ISharedMemoryHandle &operator=(ISharedMemoryHandle &&) = delete;
    virtual ~ISharedMemoryHandle() = default;

    virtual std::uint8_t *getShm() const = 0;
};
} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_I_SHARED_MEMORY_HANDLE_H_
