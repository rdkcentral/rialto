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

#ifndef FIREBOLT_RIALTO_CLIENT_SHARED_MEMORY_MANAGER_H_
#define FIREBOLT_RIALTO_CLIENT_SHARED_MEMORY_MANAGER_H_

#include "IControlIpc.h"
#include "ISharedMemoryManager.h"
#include <mutex>
#include <set>

namespace firebolt::rialto::client
{
class SharedMemoryManager : public ISharedMemoryManager
{
public:
    static SharedMemoryManager &instance();

    uint8_t *getSharedMemoryBuffer() override;
    bool registerClient(ISharedMemoryManagerClient *client) override;
    bool unregisterClient(ISharedMemoryManagerClient *client) override;
    bool initSharedMemory() override;
    void termSharedMemory() override;

private:
    SharedMemoryManager(const std::shared_ptr<IControlIpcFactory> &ControlIpcFactory);
    ~SharedMemoryManager() override;

private:
    /**
     * @brief Mutex protection for class attributes.
     */
    std::mutex m_mutex;

    /**
     * @brief The shared memory file descriptor.
     */
    int32_t m_shmFd;

    /**
     * @brief The shared memory buffer pointer.
     */
    uint8_t *m_shmBuffer;

    /**
     * @brief The shared memory buffer length.
     */
    uint32_t m_shmBufferLen;

    /**
     * @brief The rialto control ipc factory.
     */
    std::shared_ptr<IControlIpc> m_controlIpc;

    /**
     * @brief Vector of clients to notify.
     */
    std::set<ISharedMemoryManagerClient *> m_clientVec;
};
} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_SHARED_MEMORY_MANAGER_H_
