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

#ifndef FIREBOLT_RIALTO_CLIENT_I_CONTROL_IPC_H_
#define FIREBOLT_RIALTO_CLIENT_I_CONTROL_IPC_H_

#include "IControlClient.h"
#include <stdint.h>

#include <memory>
#include <string>

namespace firebolt::rialto::client
{
class IControlIpc;

/**
 * @brief IControlIpc accessor class definition.
 */
class IControlIpcAccessor
{
public:
    virtual ~IControlIpcAccessor() = default;
    IControlIpcAccessor(const IControlIpcAccessor &) = delete;
    IControlIpcAccessor &operator=(const IControlIpcAccessor &) = delete;
    IControlIpcAccessor(IControlIpcAccessor &&) = delete;
    IControlIpcAccessor &operator=(IControlIpcAccessor &&) = delete;

    /**
     * @brief Get a IControlIpcAccessor instance.
     *
     * @retval the IControlIpcAccessor instance
     */
    static IControlIpcAccessor &instance();

    /**
     * @brief Get ControlIpc object.
     *
     * @retval the reference to ControlIpc singleton object
     */
    virtual IControlIpc &getControlIpc() const = 0;

protected:
    IControlIpcAccessor() = default;
};

/**
 * @brief The definition of the IControlIpc interface.
 *
 * This interface defines the rialto control ipc APIs that are used to communicate with the Rialto server.
 */
class IControlIpc
{
public:
    IControlIpc() = default;
    virtual ~IControlIpc() = default;

    IControlIpc(const IControlIpc &) = delete;
    IControlIpc &operator=(const IControlIpc &) = delete;
    IControlIpc(IControlIpc &&) = delete;
    IControlIpc &operator=(IControlIpc &&) = delete;

    /**
     * @brief Gets shared memory information to map.
     *
     * @param[in] fd    : The file descriptor of the shared memory region.
     * @param[in] size  : The size of the shared memory region.
     *
     * @retval true success, false otherwise.
     */
    virtual bool getSharedMemory(int32_t &fd, uint32_t &size) = 0;

    /**
     * @brief Register new IControlClient
     *
     * @param[in]  state    : Client object for callbacks
     * @param[out] appState : Current application state
     *
     * @retval true on success, false otherwise.
     */
    virtual bool registerClient(IControlClient *client) = 0;

    /**
     * @brief Unregister a client.
     *
     * @param[in] client    : Client to unregister.
     *
     * @retval true on success, false otherwise.
     */
    virtual bool unregisterClient(IControlClient *client) = 0;
};

}; // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_I_CONTROL_IPC_H_
