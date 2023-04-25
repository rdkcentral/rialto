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

#ifndef FIREBOLT_RIALTO_I_CONTROL_H_
#define FIREBOLT_RIALTO_I_CONTROL_H_

/**
 * @file IControl.h
 *
 * The definition of the IControl interface.
 *
 * This interface defines the public API of Rialto for control of Rialto,
 * including the IPC connection and shared memory.
 */

#include "ControlCommon.h"
#include "IControlClient.h"
#include <memory>
#include <stdint.h>

namespace firebolt::rialto
{
class IControl;

/**
 * @brief IControl accessor class definition.
 */
class IControlAccessor
{
public:
    virtual ~IControlAccessor() = default;
    IControlAccessor(const IControlAccessor &) = delete;
    IControlAccessor &operator=(const IControlAccessor &) = delete;
    IControlAccessor(IControlAccessor &&) = delete;
    IControlAccessor &operator=(IControlAccessor &&) = delete;

    /**
     * @brief Get a IControlAccessor instance.
     *
     * @retval the control instance
     */
    static IControlAccessor &instance();

    /**
     * @brief Get Control object.
     *
     * @retval the reference to Control singleton object
     */
    virtual IControl &getControl() const = 0;

protected:
    IControlAccessor() = default;
};

/**
 * @brief The definition of the IControl interface.
 *
 * This interface defines the public API for control of the ipc and shared memory.
 *
 */
class IControl
{
public:
    IControl() = default;
    virtual ~IControl() = default;

    IControl(const IControl &) = delete;
    IControl &operator=(const IControl &) = delete;
    IControl(IControl &&) = delete;
    IControl &operator=(IControl &&) = delete;

    /**
     * @brief Register new IControlClient
     *
     * @param[in]  state    : Client object for callbacks
     * @param[out] appState : Current application state
     *
     * @retval true on success, false otherwise.
     */
    virtual bool registerClient(IControlClient *client, ApplicationState &appState) = 0;

    /**
     * @brief Unregister a client.
     *
     * @param[in] client    : Client to unregister.
     *
     * @retval true on success, false otherwise.
     */
    virtual bool unregisterClient(IControlClient *client) = 0;
};

}; // namespace firebolt::rialto

#endif // FIREBOLT_RIALTO_I_CONTROL_H_
