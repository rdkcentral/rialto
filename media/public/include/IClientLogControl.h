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

#ifndef FIREBOLT_RIALTO_I_CLIENT_LOG_CONTROL_H_
#define FIREBOLT_RIALTO_I_CLIENT_LOG_CONTROL_H_

/**
 * @file IControl.h
 *
 * The definition of the IControl interface.
 *
 * This interface defines the public API of Rialto for control of Rialto,
 * including the IPC connection and shared memory.
 */

#include <memory>
#include <stdint.h>

#include "ControlCommon.h"
#include "IClientLogHandler.h"
#include "IControlClient.h"

namespace firebolt::rialto
{
class IClientLogControl;

/**
 * @brief IClientLogControl factory class, returns a concrete implementation of IClientLogControl
 */
class IClientLogControlFactory
{
public:
    IClientLogControlFactory() = default;
    virtual ~IClientLogControlFactory() = default;

    /**
     * @brief Creates the IClientLogControlFactory singleton
     *
     * @retval the factory instance or null on error.
     */
    static std::shared_ptr<IClientLogControlFactory> createFactory();

    /**
     * @brief IClientLogControl factory method, returns a concrete singleton implementation of IClientLogControl
     *
     * @retval the new IClientLogControl instance or null on error.
     */
    virtual std::shared_ptr<IClientLogControl> createClientLogControl() = 0;
};

/**
 * @brief The definition of the IClientLogControl interface.
 *
 * This interface defines the public API for controlling Rialto client's
 * log handling. This class is a singleton
 *
 */
class IClientLogControl
{
public:
    IClientLogControl() = default;
    virtual ~IClientLogControl() = default;

    IClientLogControl(const IClientLogControl &) = delete;
    IClientLogControl &operator=(const IClientLogControl &) = delete;
    IClientLogControl(IClientLogControl &&) = delete;
    IClientLogControl &operator=(IClientLogControl &&) = delete;

    /**
     * @brief Register new log handler
     *
     * @param[in]  handler   : Client object for callbacks
     * @param[in]  ignoreLogLevels   : If true then the handler will receive ALL log level messages regardless of the
     * currently configured log level
     *
     * @retval true if successful
     */
    virtual bool registerLogHandler(std::shared_ptr<IClientLogHandler> &handler, bool ignoreLogLevels) = 0;
};

}; // namespace firebolt::rialto

#endif // FIREBOLT_RIALTO_I_CLIENT_LOG_CONTROL_H_
