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

#ifndef FIREBOLT_RIALTO_SERVER_SERVICE_CDM_SERVICE_MOCK_H_
#define FIREBOLT_RIALTO_SERVER_SERVICE_CDM_SERVICE_MOCK_H_

#include "ICdmService.h"
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <vector>

namespace firebolt::rialto::server::service
{
class CdmServiceMock : public ICdmService
{
public:
    MOCK_METHOD(bool, switchToActive, (), (override));
    MOCK_METHOD(void, switchToInactive, (), (override));
    MOCK_METHOD(bool, createMediaKeys, (int mediaKeysHandle, std::string keySystem), (override));
    MOCK_METHOD(bool, destroyMediaKeys, (int mediaKeysHandle), (override));
    MOCK_METHOD(MediaKeyErrorStatus, createKeySession,
                (int mediaKeysHandle, KeySessionType sessionType, const std::shared_ptr<IMediaKeysClient> &client,
                 bool isLDL, int32_t &keySessionId),
                (override));
    MOCK_METHOD(MediaKeyErrorStatus, generateRequest,
                (int mediaKeysHandle, int32_t keySessionId, InitDataType initDataType,
                 const std::vector<uint8_t> &initData),
                (override));
    MOCK_METHOD(MediaKeyErrorStatus, loadSession, (int mediaKeysHandle, int32_t keySessionId), (override));
    MOCK_METHOD(MediaKeyErrorStatus, updateSession,
                (int mediaKeysHandle, int32_t keySessionId, const std::vector<uint8_t> &responseData), (override));
    MOCK_METHOD(MediaKeyErrorStatus, closeKeySession, (int mediaKeysHandle, int32_t keySessionId), (override));
    MOCK_METHOD(MediaKeyErrorStatus, removeKeySession, (int mediaKeysHandle, int32_t keySessionId), (override));
    MOCK_METHOD(MediaKeyErrorStatus, getCdmKeySessionId,
                (int mediaKeysHandle, int32_t keySessionId, std::string &cdmKeySessionId), (override));
    MOCK_METHOD(bool, containsKey, (int mediaKeysHandle, int32_t keySessionId, const std::vector<uint8_t> &keyId),
                (override));
    MOCK_METHOD(MediaKeyErrorStatus, setDrmHeader,
                (int mediaKeysHandle, int32_t keySessionId, const std::vector<uint8_t> &requestData), (override));
    MOCK_METHOD(MediaKeyErrorStatus, deleteDrmStore, (int mediaKeysHandle), (override));
    MOCK_METHOD(MediaKeyErrorStatus, deleteKeyStore, (int mediaKeysHandle), (override));
    MOCK_METHOD(MediaKeyErrorStatus, getDrmStoreHash, (int mediaKeysHandle, std::vector<unsigned char> &drmStoreHash),
                (override));
    MOCK_METHOD(MediaKeyErrorStatus, getKeyStoreHash, (int mediaKeysHandle, std::vector<unsigned char> &keyStoreHash),
                (override));
    MOCK_METHOD(MediaKeyErrorStatus, getLdlSessionsLimit, (int mediaKeysHandle, uint32_t &ldlLimit), (override));
    MOCK_METHOD(MediaKeyErrorStatus, getLastDrmError, (int mediaKeysHandle, int32_t keySessionId, uint32_t &errorCode),
                (override));
    MOCK_METHOD(MediaKeyErrorStatus, getDrmTime, (int mediaKeysHandle, uint64_t &drmTime), (override));
    MOCK_METHOD(std::vector<std::string>, getSupportedKeySystems, (), (override));
    MOCK_METHOD(bool, supportsKeySystem, (const std::string &keySystem), (override));
    MOCK_METHOD(bool, getSupportedKeySystemVersion, (const std::string &keySystem, std::string &version), (override));
    MOCK_METHOD(void, ping, (const std::shared_ptr<IHeartbeatProcedure> &heartbeatProcedure), (override));
};
} // namespace firebolt::rialto::server::service

#endif // FIREBOLT_RIALTO_SERVER_SERVICE_CDM_SERVICE_MOCK_H_
