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

#ifndef FIREBOLT_RIALTO_SERVER_I_OCDM_SESSION_CLIENT_H_
#define FIREBOLT_RIALTO_SERVER_I_OCDM_SESSION_CLIENT_H_

#include <MediaCommon.h>
#include <memory>
#include <stdint.h>
#include <string>

namespace firebolt::rialto::server
{
class IOcdmSessionClient
{
public:
    IOcdmSessionClient() = default;
    virtual ~IOcdmSessionClient() = default;

    IOcdmSessionClient(const IOcdmSessionClient &) = delete;
    IOcdmSessionClient &operator=(const IOcdmSessionClient &) = delete;
    IOcdmSessionClient(IOcdmSessionClient &&) = delete;
    IOcdmSessionClient &operator=(IOcdmSessionClient &&) = delete;

    /**
     * @brief Requests the processing of the challenge data.
     *
     * @param[in]  url              : The url string.
     * @param[in]  challenge        : The challenge id buffer.
     * @param[in]  challengeLength  : The length of the challenge buffer.
     */
    virtual void onProcessChallenge(const char url[], const uint8_t challenge[], const uint16_t challengeLength) = 0;

    /**
     * @brief Notifys of a key status change for key_id.
     *
     * @param[in]  keyId        : The key id buffer.
     * @param[in]  keyIdLength  : The length of the key id buffer.
     */
    virtual void onKeyUpdated(const uint8_t keyId[], const uint8_t keyIdLength) = 0;

    /**
     * @brief Notifys that all the key statuses have been updated.
     */
    virtual void onAllKeysUpdated() = 0;

    /**
     * @brief Notifys of a new ocdm error.
     *
     * @param[in]  message      : The error message received.
     */
    virtual void onError(const char message[]) = 0;
};

}; // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_I_OCDM_SESSION_CLIENT_H_
