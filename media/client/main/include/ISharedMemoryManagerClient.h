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

#ifndef FIREBOLT_RIALTO_CLIENT_I_SHARED_MEMORY_MANAGER_CLIENT_H_
#define FIREBOLT_RIALTO_CLIENT_I_SHARED_MEMORY_MANAGER_CLIENT_H_

#include <memory>
#include <stdint.h>
#include <string>

namespace firebolt::rialto::client
{
/**
 * @brief The Rialto shared memory manager client interface.
 *
 * This is The shared memory manager client abstract base class. It should be
 * implemented by any object that wishes to be notified of changes to the
 * shared memory.
 */
class ISharedMemoryManagerClient
{
public:
    ISharedMemoryManagerClient() = default;
    virtual ~ISharedMemoryManagerClient() = default;

    ISharedMemoryManagerClient(const ISharedMemoryManagerClient &) = delete;
    ISharedMemoryManagerClient &operator=(const ISharedMemoryManagerClient &) = delete;
    ISharedMemoryManagerClient(ISharedMemoryManagerClient &&) = delete;
    ISharedMemoryManagerClient &operator=(ISharedMemoryManagerClient &&) = delete;

    /**
     * @brief Notification that the shared buffer will be terminated.
     * The client should treat the buffer pointer returned from getSharedMemoryBuffer as invalid.
     */
    virtual void notifyBufferTerm() = 0;
};

}; // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_I_SHARED_MEMORY_MANAGER_CLIENT_H_
