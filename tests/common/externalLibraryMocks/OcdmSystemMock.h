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

#ifndef FIREBOLT_RIALTO_WRAPPERS_OCDM_SYSTEM_MOCK_H_
#define FIREBOLT_RIALTO_WRAPPERS_OCDM_SYSTEM_MOCK_H_

#include "IOcdmSystem.h"
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <vector>

namespace firebolt::rialto::wrappers
{
class OcdmSystemMock : public IOcdmSystem
{
public:
    MOCK_METHOD(MediaKeyErrorStatus, getVersion, (std::string & version), (override));
    MOCK_METHOD(MediaKeyErrorStatus, getLdlSessionsLimit, (uint32_t * ldlLimit), (override));
    MOCK_METHOD(MediaKeyErrorStatus, deleteKeyStore, (), (override));
    MOCK_METHOD(MediaKeyErrorStatus, deleteSecureStore, (), (override));
    MOCK_METHOD(MediaKeyErrorStatus, getKeyStoreHash, (uint8_t keyStoreHash[], uint32_t keyStoreHashLength), (override));
    MOCK_METHOD(MediaKeyErrorStatus, getSecureStoreHash, (uint8_t secureStoreHash[], uint32_t secureStoreHashLength),
                (override));
    MOCK_METHOD(MediaKeyErrorStatus, getDrmTime, (uint64_t * time), (override));
    MOCK_METHOD(std::unique_ptr<IOcdmSession>, createSession, (IOcdmSessionClient * client), (override, const));
    MOCK_METHOD(bool, supportsServerCertificate, (), (const, override));
    MOCK_METHOD(MediaKeyErrorStatus, getMetricSystemData, (uint32_t &bufferLength, std::vector<uint8_t> &buffer),
                (override));
};
} // namespace firebolt::rialto::wrappers

#endif // FIREBOLT_RIALTO_WRAPPERS_OCDM_SYSTEM_MOCK_H_
