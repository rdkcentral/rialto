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

#ifndef FIREBOLT_RIALTO_SERVER_MEDIA_KEYS_SERVER_INTERNAL_MOCK_H_
#define FIREBOLT_RIALTO_SERVER_MEDIA_KEYS_SERVER_INTERNAL_MOCK_H_

#include "IMediaKeysServerInternal.h"
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <vector>

namespace firebolt::rialto::server
{
class MediaKeysServerInternalMock : public IMediaKeysServerInternal
{
public:
    MediaKeysServerInternalMock() = default;
    virtual ~MediaKeysServerInternalMock() = default;

    MOCK_METHOD(MediaKeyErrorStatus, selectKeyId, (int32_t keySessionId, const std::vector<uint8_t> &keyId), (override));
    MOCK_METHOD(bool, containsKey, (int32_t keySessionId, const std::vector<uint8_t> &keyId), (override));
    MOCK_METHOD(MediaKeyErrorStatus, createKeySession,
                (KeySessionType sessionType, std::weak_ptr<IMediaKeysClient> client, bool isLDL, int32_t &keySessionId),
                (override));
    MOCK_METHOD(MediaKeyErrorStatus, generateRequest,
                (int32_t keySessionId, InitDataType initDataType, const std::vector<uint8_t> &initData), (override));
    MOCK_METHOD(MediaKeyErrorStatus, loadSession, (int32_t keySessionId), (override));
    MOCK_METHOD(MediaKeyErrorStatus, updateSession, (int32_t keySessionId, const std::vector<uint8_t> &responseData),
                (override));
    MOCK_METHOD(MediaKeyErrorStatus, setDrmHeader, (int32_t keySessionId, const std::vector<uint8_t> &requestData),
                (override));
    MOCK_METHOD(MediaKeyErrorStatus, closeKeySession, (int32_t keySessionId), (override));
    MOCK_METHOD(MediaKeyErrorStatus, removeKeySession, (int32_t keySessionId), (override));
    MOCK_METHOD(MediaKeyErrorStatus, deleteDrmStore, (), (override));
    MOCK_METHOD(MediaKeyErrorStatus, deleteKeyStore, (), (override));
    MOCK_METHOD(MediaKeyErrorStatus, getDrmStoreHash, (std::vector<unsigned char> & drmStoreHash), (override));
    MOCK_METHOD(MediaKeyErrorStatus, getKeyStoreHash, (std::vector<unsigned char> & keyStoreHash), (override));
    MOCK_METHOD(MediaKeyErrorStatus, getLdlSessionsLimit, (uint32_t & ldlLimit), (override));
    MOCK_METHOD(MediaKeyErrorStatus, getLastDrmError, (int32_t keySessionId, uint32_t &errorCode), (override));
    MOCK_METHOD(MediaKeyErrorStatus, getDrmTime, (uint64_t & drmTime), (override));
    MOCK_METHOD(MediaKeyErrorStatus, getCdmKeySessionId, (int32_t keySessionId, std::string &cdmKeySessionId),
                (override));
    MOCK_METHOD(MediaKeyErrorStatus, decrypt,
                (int32_t keySessionId, GstBuffer *encrypted, GstCaps *caps),
                (override));
    MOCK_METHOD(bool, hasSession, (int32_t keySessionId), (const, override));
    MOCK_METHOD(bool, isNetflixKeySystem, (int32_t keySessionId), (const, override));
    MOCK_METHOD(void, incrementSessionIdUsageCounter, (int32_t keySessionId), (override));
    MOCK_METHOD(void, decrementSessionIdUsageCounter, (int32_t keySessionId), (override));
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_MEDIA_KEYS_SERVER_INTERNAL_MOCK_H_
