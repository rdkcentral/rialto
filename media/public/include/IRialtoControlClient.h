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

#ifndef FIREBOLT_RIALTO_I_RIALTO_CONTROL_CLIENT_H_
#define FIREBOLT_RIALTO_I_RIALTO_CONTROL_CLIENT_H_

/**
 * @file IRialtoControlClient.h
 *
 * The definition of the IRialtoControlClient interface.
 *
 * This file comprises the definition of the IRialtoControlClient abstract
 * class. This is the API by which a IRialtoControl implementation will
 * pass notifications to its client.
 */

#include "RialtoControlCommon.h"

namespace firebolt::rialto
{
/**
 * @brief The Rialto control client interface.
 *
 * This is The Rialto control client abstract base class. It should be
 * implemented by any object that wishes to be notified by changes in the
 * state of the rialto server.
 */
class IRialtoControlClient
{
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~IRialtoControlClient() = default;

    /**
     * @brief Notifies the client that rialto server reached new application state
     *
     * @param[in] state: The new application state.
     */
    virtual void notifyApplicationState(ApplicationState state) = 0;
};
} // namespace firebolt::rialto

#endif // FIREBOLT_RIALTO_I_RIALTO_CONTROL_CLIENT_H_
