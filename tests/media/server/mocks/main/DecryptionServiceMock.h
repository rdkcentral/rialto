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

#ifndef FIREBOLT_RIALTO_SERVER_DECRYPTION_SERVICE_MOCK_H_
#define FIREBOLT_RIALTO_SERVER_DECRYPTION_SERVICE_MOCK_H_

#include "IDecryptionService.h"
#include <gmock/gmock.h>
#include <vector>

namespace firebolt::rialto::server
{
class DecryptionServiceMock : public IDecryptionService
{
public:
    MOCK_METHOD(MediaKeyErrorStatus, decrypt, (int32_t keySessionId, GstBuffer *encrypted, GstCaps *caps), (override));
    // TODO(RIALTO-127): Remove
    MOCK_METHOD(MediaKeyErrorStatus, decrypt,
                (int32_t keySessionId, GstBuffer *encrypted, GstBuffer *subSample, const uint32_t subSampleCount,
                 GstBuffer *IV, GstBuffer *keyId, uint32_t initWithLast15, GstCaps *caps),
                (override));
    MOCK_METHOD(bool, isNetflixKeySystem, (int32_t keySessionId), (const, override));
    MOCK_METHOD(MediaKeyErrorStatus, selectKeyId, (int32_t keySessionId, const std::vector<uint8_t> &keyId), (override));
    MOCK_METHOD(void, incrementSessionIdUsageCounter, (int32_t keySessionId), (override));
    MOCK_METHOD(void, decrementSessionIdUsageCounter, (int32_t keySessionId), (override));
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_DECRYPTION_SERVICE_MOCK_H_
