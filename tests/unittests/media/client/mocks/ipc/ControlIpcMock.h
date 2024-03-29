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

#ifndef FIREBOLT_RIALTO_CLIENT_CONTROL_IPC_MOCK_H_
#define FIREBOLT_RIALTO_CLIENT_CONTROL_IPC_MOCK_H_

#include "IControlIpc.h"
#include <gmock/gmock.h>

namespace firebolt::rialto::client
{
class ControlIpcMock : public IControlIpc
{
public:
    ControlIpcMock() = default;
    virtual ~ControlIpcMock() = default;

    MOCK_METHOD(bool, getSharedMemory, (int32_t & fd, uint32_t &size), (override));
    MOCK_METHOD(bool, registerClient, (), (override));
};
} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_CONTROL_IPC_MOCK_H_
