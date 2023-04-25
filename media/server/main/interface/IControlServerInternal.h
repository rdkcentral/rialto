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

#ifndef FIREBOLT_RIALTO_SERVER_I_CONTROL_SERVER_INTERNAL_H_
#define FIREBOLT_RIALTO_SERVER_I_CONTROL_SERVER_INTERNAL_H_

/**
 * @file IControlServerInternal.h
 *
 * The definition of the IControlServerInternal interface.
 *
 * This interface defines the server internal API of Rialto for controlling rialto clients.
 */

#include "IControl.h"
#include "IControlClientServerInternal.h"

namespace firebolt::rialto::server
{
class IControlServerInternal;
class IControlServerInternalAccessor : public IControlAccessor
{
public:
    ~IControlServerInternalAccessor() override = default;

    /**
     * @brief Get a IControlServerInternalAccessor instance.
     *
     * @retval the control instance
     */
    static IControlServerInternalAccessor &instance();

    /**
     * @brief Get ControlServerInternal object.
     *
     * @retval the reference to ControlServerInternal singleton object
     */
    virtual IControlServerInternal &getControlServerInternal() const = 0;

protected:
    IControlServerInternalAccessor() = default;
};

class IControlServerInternal : public IControl
{
public:
    IControlServerInternal() = default;
    ~IControlServerInternal() override = default;

    /**
     * @brief Acknowledgement of a received ping request
     *
     * @param[in] id  : id received in ping notification
     */
    virtual void ack(uint32_t id) = 0;
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_I_CONTROL_SERVER_INTERNAL_H_
