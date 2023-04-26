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

#ifndef FIREBOLT_RIALTO_CLIENT_CONTROL_H_
#define FIREBOLT_RIALTO_CLIENT_CONTROL_H_

#include "IControl.h"
#include "IControlClient.h"
#include "IControlIpc.h"
#include "ISharedMemoryManager.h"
#include <memory>
#include <mutex>
#include <set>
#include <string>

namespace firebolt::rialto::client
{
/**
 * @brief The definition of the ControlAccessor.
 */
class ControlAccessor : public ISharedMemoryManagerAccessor
{
public:
    ~ControlAccessor() override = default;
    IControl &getControl() const override;
    ISharedMemoryManager &getSharedMemoryManager() const override;
};

/**
 * @brief The definition of the Control.
 */
class Control : public ISharedMemoryManager, public IControlClient
{
public:
    /**
     * @brief The constructor.
     */
    Control(IControlIpc &controlIpc);

    /**
     * @brief Virtual destructor.
     */
    ~Control() override;

    uint8_t *getSharedMemoryBuffer() override;
    bool registerClient(IControlClient *client, ApplicationState &appState) override;
    bool unregisterClient(IControlClient *client) override;

private:
    void notifyApplicationState(ApplicationState state) override;

    /**
     * @brief Initalised the shared memory for media playback. Function not thread-safe
     *
     * @retval true on success, false otherwise.
     */
    bool initSharedMemory();

    /**
     * @brief Terminates the shared memory. Function not thread-safe
     */
    void termSharedMemory();

    /**
     * @brief Coverts a ApplicationState to string.
     *
     * @param[in] state     : The application state.
     *
     * @retval the application state string, or "" on error.
     */
    std::string stateToString(ApplicationState state);

private:
    /**
     * @brief The current application state
     */
    ApplicationState m_currentState;

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
     * @brief The rialto control ipc.
     */
    IControlIpc &m_controlIpc;

    /**
     * @brief Vector of clients to notify.
     */
    std::set<IControlClient *> m_clientVec;
};

}; // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_CONTROL_H_
