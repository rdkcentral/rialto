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

#ifndef FIREBOLT_RIALTO_SERVER_CONTROL_SERVER_INTERNAL_FACTORY_MOCK_H_
#define FIREBOLT_RIALTO_SERVER_CONTROL_SERVER_INTERNAL_FACTORY_MOCK_H_

#include "IControlServerInternal.h"
#include <gmock/gmock.h>
#include <memory>

namespace firebolt::rialto::server
{
class ControlServerInternalFactoryMock : public IControlServerInternalFactory
{
public:
    MOCK_METHOD(std::shared_ptr<IControl>, createControl, (), (const, override));
    MOCK_METHOD(std::shared_ptr<IControlServerInternal>, createControlServerInternal,
                (const std::shared_ptr<IControlClientServerInternal> &client), (const, override));
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_CONTROL_SERVER_INTERNAL_FACTORY_MOCK_H_
