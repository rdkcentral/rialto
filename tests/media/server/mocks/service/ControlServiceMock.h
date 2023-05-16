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

#ifndef FIREBOLT_RIALTO_SERVER_SERVICE_CONTROL_SERVICE_MOCK_H_
#define FIREBOLT_RIALTO_SERVER_SERVICE_CONTROL_SERVICE_MOCK_H_

#include "IControlService.h"
#include <gmock/gmock.h>
#include <memory>

namespace firebolt::rialto::server::service
{
class ControlServiceMock : public IControlService
{
public:
    MOCK_METHOD(void, addControl, (int controlId, const std::shared_ptr<IControlClientServerInternal> &client),
                (override));
    MOCK_METHOD(void, removeControl, (int controlId), (override));
    MOCK_METHOD(bool, ack, (int controlId, std::uint32_t id), (override));
    MOCK_METHOD(void, setApplicationState, (const ApplicationState &state), (override));
};
} // namespace firebolt::rialto::server::service

#endif // FIREBOLT_RIALTO_SERVER_SERVICE_CONTROL_SERVICE_MOCK_H_
