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

#ifndef FIREBOLT_RIALTO_SERVER_OCDM_SESSION_MOCK_H_
#define FIREBOLT_RIALTO_SERVER_OCDM_SESSION_MOCK_H_

#include "IOcdmSession.h"
#include <gmock/gmock.h>
#include <memory>
#include <string>

namespace firebolt::rialto::server
{
class OcdmSessionMock : public IOcdmSession
{
public:
    MOCK_METHOD(MediaKeyErrorStatus, constructSession,
                (KeySessionType sessionType, InitDataType initDataType, const uint8_t initData[], uint32_t initDataSize),
                (override));
    MOCK_METHOD(MediaKeyErrorStatus, getChallengeData, (bool isLDL, uint8_t *challenge, uint32_t *challengeSize),
                (override));
    MOCK_METHOD(MediaKeyErrorStatus, storeLicenseData, (const uint8_t challengeData[], uint32_t challengeSize),
                (override));
    MOCK_METHOD(MediaKeyErrorStatus, load, (), (override));
    MOCK_METHOD(MediaKeyErrorStatus, update, (const uint8_t response[], uint32_t responseSize), (override));
    MOCK_METHOD(MediaKeyErrorStatus, decrypt,
                (GstBuffer * encrypted, GstBuffer *subSample, const uint32_t subSampleCount, GstBuffer *IV,
                 GstBuffer *keyId, uint32_t initWithLast15),
                (override));
    MOCK_METHOD(MediaKeyErrorStatus, remove, (), (override));
    MOCK_METHOD(MediaKeyErrorStatus, close, (), (override));
    MOCK_METHOD(MediaKeyErrorStatus, cancelChallengeData, (), (override));
    MOCK_METHOD(MediaKeyErrorStatus, cleanDecryptContext, (), (override));
    MOCK_METHOD(MediaKeyErrorStatus, destructSession, (), (override));
    MOCK_METHOD(KeyStatus, getStatus, (const uint8_t keyId[], const uint8_t keyIdLength), (override));
    MOCK_METHOD(MediaKeyErrorStatus, getCdmKeySessionId, (std::string & cdmKeySessionId), (override));
    MOCK_METHOD(MediaKeyErrorStatus, selectKeyId, (uint8_t * keyId, uint32_t *keyIdSize), (override));
    MOCK_METHOD(uint32_t, hasKeyId, (const uint8_t keyId[], const uint8_t keyIdSize), (override));
    MOCK_METHOD(MediaKeyErrorStatus, setDrmHeader, (const uint8_t drmHeader[], uint32_t drmHeaderSize), (override));
    MOCK_METHOD(MediaKeyErrorStatus, getLastDrmError, (uint32_t & errorCode), (override));
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_OCDM_SESSION_MOCK_H_
