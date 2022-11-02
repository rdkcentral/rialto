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

#ifndef FIREBOLT_RIALTO_CLIENT_I_RIALTO_CONTROL_IPC_H_
#define FIREBOLT_RIALTO_CLIENT_I_RIALTO_CONTROL_IPC_H_

#include <stdint.h>

#include <memory>
#include <string>

namespace firebolt::rialto::client
{
class IRialtoControlIpc;

/**
 * @brief IRialtoControlIpc factory class, returns a singlton of IRialtoControlIpc
 */
class IRialtoControlIpcFactory
{
public:
    IRialtoControlIpcFactory() = default;
    virtual ~IRialtoControlIpcFactory() = default;

    /**
     * @brief Create a IRialtoControlIpcFactory instance.
     *
     * @retval the factory instance or null on error.
     */
    static std::shared_ptr<IRialtoControlIpcFactory> createFactory();

    /**
     * @brief Gets the IRialtoControlIpc singleton object.
     *
     * @retval the rialto controller ipc instance or null on error.
     */
    virtual std::shared_ptr<IRialtoControlIpc> getRialtoControlIpc() = 0;
};

/**
 * @brief The definition of the IRialtoControlIpc interface.
 *
 * This interface defines the rialto control ipc APIs that are used to communicate with the Rialto server.
 */
class IRialtoControlIpc
{
public:
    IRialtoControlIpc() = default;
    virtual ~IRialtoControlIpc() = default;

    IRialtoControlIpc(const IRialtoControlIpc &) = delete;
    IRialtoControlIpc &operator=(const IRialtoControlIpc &) = delete;
    IRialtoControlIpc(IRialtoControlIpc &&) = delete;
    IRialtoControlIpc &operator=(IRialtoControlIpc &&) = delete;

    /**
     * @brief Connect the IPC client.
     *
     * @retval true on success, false otherwise.
     */
    virtual bool connect() = 0;

    /**
     * @brief Disconnect the IPC client.
     *
     * @retval true on success, false otherwise.
     */
    virtual bool disconnect() = 0;

    /**
     * @brief Gets shared memory information to map.
     *
     * @param[in] fd    : The file descriptor of the shared memory region.
     * @param[in] size  : The size of the shared memory region.
     *
     * @retval true success, false otherwise.
     */
    virtual bool getSharedMemory(int32_t &fd, uint32_t &size) = 0;
};

}; // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_I_RIALTO_CONTROL_IPC_H_
