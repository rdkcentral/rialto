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

#ifndef FIREBOLT_RIALTO_CLIENT_I_SHARED_MEMORY_MANAGER_H_
#define FIREBOLT_RIALTO_CLIENT_I_SHARED_MEMORY_MANAGER_H_

#include "ISharedMemoryManagerClient.h"
#include <functional>
#include <memory>
#include <stdint.h>
namespace firebolt::rialto::client
{
class ISharedMemoryManager;

/**
 * @brief ISharedMemoryManager factory class, returns a concrete implementation of ISharedMemoryManager.
 */
class ISharedMemoryManagerFactory
{
public:
    ISharedMemoryManagerFactory() = default;
    virtual ~ISharedMemoryManagerFactory() = default;

    /**
     * @brief Creates a ISharedMemoryManagerFactory instance.
     *
     * @retval the factory instance or null on error.
     */
    static std::shared_ptr<ISharedMemoryManagerFactory> createFactory();

    /**
     * @brief ISharedMemoryManager factory method, returns a the singleton ISharedMemoryManager object.
     *
     * @retval the shared memory manager instance or null on error.
     */
    virtual std::shared_ptr<ISharedMemoryManager> getSharedMemoryManager() const = 0;
};

/**
 * @brief The definition of the ISharedMemoryManager interface.
 *
 * This interface defines the internal API querying shared memory.
 */
class ISharedMemoryManager
{
public:
    ISharedMemoryManager() = default;
    virtual ~ISharedMemoryManager() = default;

    ISharedMemoryManager(const ISharedMemoryManager &) = delete;
    ISharedMemoryManager &operator=(const ISharedMemoryManager &) = delete;
    ISharedMemoryManager(ISharedMemoryManager &&) = delete;
    ISharedMemoryManager &operator=(ISharedMemoryManager &&) = delete;

    /**
     * @brief Gets the pointer to the mapped shared memory.
     *
     * @retval valid pointer on sucess, nullptr otherwise.
     */
    virtual uint8_t *getSharedMemoryBuffer() = 0;

    /**
     * @brief Register a client notify when the shared buffer changes.
     *
     * @param[in] client    : Client to register.
     *
     * @retval true on success, false otherwise.
     */
    virtual bool registerClient(ISharedMemoryManagerClient *client) = 0;

    /**
     * @brief Unregister a client.
     *
     * @param[in] client    : Client to unregister.
     *
     * @retval true on success, false otherwise.
     */
    virtual bool unregisterClient(ISharedMemoryManagerClient *client) = 0;

    /**
     * @brief Initalised the shared memory for media playback.
     *
     * @retval true on success, false otherwise.
     */
    virtual bool initSharedMemory() = 0;

    /**
     * @brief Terminates the shared memory.
     */
    virtual void termSharedMemory() = 0;
};

}; // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_I_SHARED_MEMORY_MANAGER_H_
