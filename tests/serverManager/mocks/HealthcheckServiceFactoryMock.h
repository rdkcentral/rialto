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

#ifndef RIALTO_SERVERMANAGER_COMMON_HEALTHCHECK_SERVICE_FACTORY_MOCK_H_
#define RIALTO_SERVERMANAGER_COMMON_HEALTHCHECK_SERVICE_FACTORY_MOCK_H_

#include "IHealthcheckServiceFactory.h"
#include <gmock/gmock.h>

namespace rialto::servermanager::common
{
class HealthcheckServiceFactoryMock : public IHealthcheckServiceFactory
{
public:
    MOCK_METHOD(std::unique_ptr<IHealthcheckService>, createHealthcheckService, (ISessionServerAppManager & appManager),
                (const, override));
};
} // namespace rialto::servermanager::common

#endif // RIALTO_SERVERMANAGER_COMMON_HEALTHCHECK_SERVICE_FACTORY_MOCK_H_
