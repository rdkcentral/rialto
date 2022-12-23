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

#ifndef FIREBOLT_RIALTO_I_RIALTO_CONTROL_H_
#define FIREBOLT_RIALTO_I_RIALTO_CONTROL_H_

/**
 * @file IRialtoControl.h
 *
 * The definition of the IRialtoControl interface.
 *
 * This interface defines the public API of Rialto for control of Rialto,
 * including the IPC connection and shared memory.
 */

#include "RialtoControlCommon.h"
#include <memory>
#include <stdint.h>

namespace firebolt::rialto
{
class IRialtoControl;

/**
 * @brief IRialtoControl factory class, returns a concrete implementation of IRialtoControl
 */
class IRialtoControlFactory
{
public:
    IRialtoControlFactory() = default;
    virtual ~IRialtoControlFactory() = default;

    /**
     * @brief Creates a IRialtoControlFactory instance.
     *
     * @retval the factory instance or null on error.
     */
    static std::shared_ptr<IRialtoControlFactory> createFactory();

    /**
     * @brief IRialtoControl factory method, returns a the singleton IRialtoControl object.
     *
     * @retval the rialto control instance or null on error.
     */
    virtual std::shared_ptr<IRialtoControl> getRialtoControl() const = 0;
};

/**
 * @brief The definition of the IRialtoControl interface.
 *
 * This interface defines the public API for control of the ipc and shared memory,
 * should be implemented by Rialto Client only.
 * Initially, IRialtoControl instance is in INACTIVE state.
 * It can be changed using IRialtoControl::setApplicationState method.
 *
 */
class IRialtoControl
{
public:
    IRialtoControl() = default;
    virtual ~IRialtoControl() = default;

    IRialtoControl(const IRialtoControl &) = delete;
    IRialtoControl &operator=(const IRialtoControl &) = delete;
    IRialtoControl(IRialtoControl &&) = delete;
    IRialtoControl &operator=(IRialtoControl &&) = delete;

    /**
     * @brief Set the application state
     * On a transtion from INACTIVE->RUNNING, map the shared memory region.
     * On a transtion from RUNNING->INACTIVE, unmap the shared memory region.
     * Initial state of IRialtoControl instance is INACTIVE.
     *
     * @param[in] state  : The application state.
     *
     * @retval true on success, false otherwise.
     */
    virtual bool setApplicationState(ApplicationState state) = 0;
};

}; // namespace firebolt::rialto

#endif // FIREBOLT_RIALTO_I_RIALTO_CONTROL_H_
