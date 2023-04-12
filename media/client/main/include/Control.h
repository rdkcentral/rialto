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
 * @brief IControl factory class definition.
 */
class ControlFactory : public IControlFactory
{
public:
    ControlFactory() = default;
    ~ControlFactory() override = default;

    std::shared_ptr<IControl> createControl(std::weak_ptr<IControlClient> client) const override;

    /**
     * @brief Create the generic rialto control factory object.
     *
     * @retval the generic rialto control factory instance or null on error.
     */
    static std::shared_ptr<ControlFactory> createFactory();
};

/**
 * @brief The definition of the Control.
 */
class Control : public IControl, public IControlClient
{
public:
    /**
     * @brief The constructor.
     */
    Control(std::weak_ptr<IControlClient> client, ISharedMemoryManager &sharedMemoryManager);

    /**
     * @brief Virtual destructor.
     */
    ~Control() override;

    void ack(uint32_t id) override;

    void notifyApplicationState(ApplicationState state) override;
    void ping(uint32_t id) override;

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
     * @brief The current application state
     */
    ApplicationState m_currentState;

    /**
     * @brief Mutex protection for the states of Control.
     */
    std::mutex m_stateMutex;

    /**
     * @brief The control client
     */
    std::weak_ptr<IControlClient> m_client;

    /**
     * @brief The rialto shared memory manager object.
     */
    ISharedMemoryManager &m_sharedMemoryManager;

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

#endif // FIREBOLT_RIALTO_CLIENT_CONTROL_H_
