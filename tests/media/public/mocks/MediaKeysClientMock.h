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

#ifndef FIREBOLT_RIALTO_MOCK_MEDIA_KEYS_CLIENT_MOCK_H_
#define FIREBOLT_RIALTO_MOCK_MEDIA_KEYS_CLIENT_MOCK_H_

#include "IMediaKeysClient.h"
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <vector>

namespace firebolt::rialto::mock
{
class MediaKeysClientMock : public IMediaKeysClient
{
public:
    MediaKeysClientMock() = default;
    virtual ~MediaKeysClientMock() = default;

    MOCK_METHOD(void, onLicenseRequest,
                (int32_t keySessionId, const std::vector<unsigned char> &licenseRequestMessage, const std::string &url),
                (override));
    MOCK_METHOD(void, onLicenseRenewal, (int32_t keySessionId, const std::vector<unsigned char> &licenseRenewalMessage),
                (override));
    MOCK_METHOD(void, onKeyStatusesChanged, (int32_t keySessionId, const KeyStatusVector &keyStatuses), (override));
};
} // namespace firebolt::rialto::mock

#endif // FIREBOLT_RIALTO_MOCK_MEDIA_KEYS_CLIENT_MOCK_H_
