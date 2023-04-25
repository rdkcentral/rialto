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

#include "IControl.h"
#include "IControlClient.h"
#include <functional>
#include <memory>
#include <stdint.h>
namespace firebolt::rialto::client
{
class ISharedMemoryManager;

/**
 * @brief ISharedMemoryManager accessor class definition.
 */
class ISharedMemoryManagerAccessor : public IControlAccessor
{
public:
    virtual ~ISharedMemoryManagerAccessor() = default;
    ISharedMemoryManagerAccessor(const ISharedMemoryManagerAccessor &) = delete;
    ISharedMemoryManagerAccessor &operator=(const ISharedMemoryManagerAccessor &) = delete;
    ISharedMemoryManagerAccessor(ISharedMemoryManagerAccessor &&) = delete;
    ISharedMemoryManagerAccessor &operator=(ISharedMemoryManagerAccessor &&) = delete;

    /**
     * @brief Get a ISharedMemoryManagerAccessor instance.
     *
     * @retval the accessor instance
     */
    static ISharedMemoryManagerAccessor &instance();

    /**
     * @brief Get SharedMemoryManager object.
     *
     * @retval the reference to SharedMemoryManager singleton object
     */
    virtual ISharedMemoryManager &getSharedMemoryManager() const = 0;

protected:
    ISharedMemoryManagerAccessor() = default;
};

/**
 * @brief The definition of the ISharedMemoryManager interface.
 *
 * This interface defines the internal API querying shared memory.
 */
class ISharedMemoryManager : public IControl
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
};

}; // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_I_SHARED_MEMORY_MANAGER_H_
