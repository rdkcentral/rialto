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

#ifndef FIREBOLT_RIALTO_SERVER_I_OCDM_SESSION_H_
#define FIREBOLT_RIALTO_SERVER_I_OCDM_SESSION_H_

#include <MediaCommon.h>
#include <gst/gst.h>
#include <memory>
#include <stdint.h>
#include <string>

namespace firebolt::rialto::server
{
class IOcdmSession
{
public:
    IOcdmSession() = default;
    virtual ~IOcdmSession() = default;

    IOcdmSession(const IOcdmSession &) = delete;
    IOcdmSession &operator=(const IOcdmSession &) = delete;
    IOcdmSession(IOcdmSession &&) = delete;
    IOcdmSession &operator=(IOcdmSession &&) = delete;

    /**
     * @brief Construct an ocdm session.
     *
     * @param[in]  sessionType  : The session type.
     * @param[in]  initDataType : The init data type.
     * @param[in]  initData     : The init data pointer.
     * @param[in]  initDataSize : The size of the init data pointed too by initData.
     *
     * @retval the return status.
     */
    virtual MediaKeyErrorStatus constructSession(KeySessionType sessionType, InitDataType initDataType,
                                                 const uint8_t initData[], uint32_t initDataSize) = 0;

    /**
     * @brief Gets challenge data for session.
     *
     * If this API is called without challenge of nullptr, the challengeSize will
     * still be returned successfully.
     *
     * @param[in]   isLDL           : Is this an LDL.
     * @param[out]  challenge       : The chanllenge for the session.
     * @param[out]  challengeSize   : The size of the challenge.
     *
     * @retval the return status.
     */
    virtual MediaKeyErrorStatus getChallengeData(bool isLDL, uint8_t *challenge, uint32_t *challengeSize) = 0;

    /**
     * @brief Stores license challenge data for session.
     *
     * @param[in]  challenge        : The challenge data pointer.
     * @param[in]  challengeSize    : The challenge size.
     *
     * @retval the return status.
     */
    virtual MediaKeyErrorStatus storeLicenseData(const uint8_t challenge[], uint32_t challengeSize) = 0;

    /**
     * @brief Loads the data of the key session.
     *
     * @retval the return status.
     */
    virtual MediaKeyErrorStatus load() = 0;

    /**
     * @brief Process a key session response.
     *
     * @param[in]  response     : The response data pointer.
     * @param[in]  responseSize : The response size.
     *
     * @retval the return status.
     */
    virtual MediaKeyErrorStatus update(const uint8_t response[], uint32_t responseSize) = 0;

    /**
     * @brief Decrypts the buffer.
     *
     * @param[in]  encrypted        : Gstreamer buffer containing encrypted data and related meta data. If applicable,
     *                                decrypted data will be stored here after this call returns.
     * @param[in]  subSample        : Gstreamer buffer containing subsamples size which has been parsed from protection
     *                                meta data.
     * @param[in]  subSampleCount   : count of subsamples
     * @param[in]  IV               : Gstreamer buffer containing initial vector (IV) used during decryption.
     * @param[in]  keyId            : Gstreamer buffer containing keyID to use for decryption
     * @param[in]  initWithLast15   : The value deciding whether decryption context needs to be initialized with
     *                                last 15 bytes. Currently this only applies to PlayReady DRM.
     *
     *
     * @retval the return status.
     */
    virtual MediaKeyErrorStatus decrypt(GstBuffer *encrypted, GstBuffer *subSample, const uint32_t subSampleCount,
                                        GstBuffer *IV, GstBuffer *keyId, uint32_t initWithLast15) = 0;

    /**
     * @brief Removes all data related to a session.
     *
     * @retval the return status.
     */
    virtual MediaKeyErrorStatus remove() = 0;

    /**
     * @brief Closes a session.
     *
     * @retval the return status.
     */
    virtual MediaKeyErrorStatus close() = 0;

    /**
     * @brief Cancels a challenge data in progress.
     *
     * @retval the return status.
     */
    virtual MediaKeyErrorStatus cancelChallengeData() = 0;

    /**
     * @brief Deinitializes the decryption context of a session.
     *
     * @retval the return status.
     */
    virtual MediaKeyErrorStatus cleanDecryptContext() = 0;

    /**
     * @brief Destructs the session.
     *
     * @retval the return status.
     */
    virtual MediaKeyErrorStatus destructSession() = 0;

    /**
     * @brief Get the status of the key in the session.
     *
     * @param[in]  keyId        : The key id buffer.
     * @param[in]  keyIdLength  : The length of the key id buffer.
     *
     * @retval the return status.
     */
    virtual KeyStatus getStatus(const uint8_t keyId[], const uint8_t keyIdLength) = 0;

    /**
     * @brief Get the internal CDM key session ID
     *
     * @param[out]  cdmKeySessionId  : The internal CDM key session ID
     *
     * @retval the return status.
     */
    virtual MediaKeyErrorStatus getCdmKeySessionId(std::string &cdmKeySessionId) = 0;
};

}; // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_I_OCDM_SESSION_H_
