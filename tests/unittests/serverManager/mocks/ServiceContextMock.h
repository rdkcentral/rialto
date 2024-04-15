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

#ifndef RIALTO_SERVERMANAGER_SERVICE_SERVICE_CONTEXT_MOCK_H_
#define RIALTO_SERVERMANAGER_SERVICE_SERVICE_CONTEXT_MOCK_H_

#include "IServiceContext.h"
#include <gmock/gmock.h>

namespace rialto::servermanager::service
{
class ServiceContextMock : public IServiceContext
{
public:
    ServiceContextMock() = default;
    virtual ~ServiceContextMock() = default;

    MOCK_METHOD(common::ISessionServerAppManager &, getSessionServerAppManager, (), (override));
};
} // namespace rialto::servermanager::service

#endif // RIALTO_SERVERMANAGER_SERVICE_SERVICE_CONTEXT_MOCK_H_
