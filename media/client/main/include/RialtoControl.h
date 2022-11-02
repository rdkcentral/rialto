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

#ifndef FIREBOLT_RIALTO_CLIENT_RIALTO_CONTROL_H_
#define FIREBOLT_RIALTO_CLIENT_RIALTO_CONTROL_H_

#include "IRialtoControl.h"
#include "IRialtoControlIpc.h"
#include "ISharedMemoryManager.h"
#include <memory>
#include <mutex>
#include <set>
#include <string>

namespace firebolt::rialto::client
{
class RialtoControl;

/**
 * @brief IRialtoControl factory class definition.
 */
class RialtoControlFactory : public IRialtoControlFactory, public ISharedMemoryManagerFactory
{
public:
    RialtoControlFactory() = default;
    ~RialtoControlFactory() override = default;

    std::shared_ptr<IRialtoControl> getRialtoControl() const override;

    std::shared_ptr<ISharedMemoryManager> getSharedMemoryManager() const override;

    /**
     * @brief Create the generic rialto control factory object.
     *
     * @retval the generic rialto control factory instance or null on error.
     */
    static std::shared_ptr<RialtoControlFactory> createFactory();

protected:
    /**
     * @brief Weak pointer to the singleton rialto control object.
     */
    static std::weak_ptr<RialtoControl> m_rialtoControl;

    /**
     * @brief Get the generic rialto control singleton object.
     *
     * @retval the generic rialto control singleton or null on error.
     */
    std::shared_ptr<RialtoControl> getGeneric() const;
};

/**
 * @brief The definition of the RialtoControl.
 */
class RialtoControl : public IRialtoControl, public ISharedMemoryManager
{
public:
    /**
     * @brief The constructor.
     *
     * @param[in] rialtoControlIpcFactory   : The factory for creating the rialto control ipc object.
     */
    explicit RialtoControl(const std::shared_ptr<IRialtoControlIpcFactory> &rialtoControlIpcFactory);

    /**
     * @brief Virtual destructor.
     */
    virtual ~RialtoControl();

    bool setApplicationState(ApplicationState state) override;

    uint8_t *getSharedMemoryBuffer() override;

    bool registerClient(ISharedMemoryManagerClient *client) override;

    bool unregisterClient(ISharedMemoryManagerClient *client) override;

private:
    /**
     * @brief Initialise the rialto client.
     * If initialisation is successful, object will be in INACTIVE state.
     *
     * @retval true on success, false otherwise.
     */
    bool init();

    /**
     * @brief Terminate the rialto client.
     */
    void term();

protected:
    /**
     * @brief The rialto control ipc factory.
     */
    std::shared_ptr<IRialtoControlIpc> m_rialtoControlIpc;

    /**
     * @brief The current application state
     */
    ApplicationState m_currentState;

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
     * @brief Mutex protection for the shared memory.
     */
    std::mutex m_shmMutex;

    /**
     * @brief Vector of clients to notify.
     */
    std::set<ISharedMemoryManagerClient *> m_clientVec;

    /**
     * @brief Mutex to protect read/write of the client vector.
     */
    std::mutex m_clientVecMutex;

    /**
     * @brief Initalised the shared memory for media playback.
     *
     * @retval true on success, false otherwise.
     */
    bool initSharedMemory();

    /**
     * @brief Terminates the shared memory.
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
};

}; // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_RIALTO_CONTROL_H_
