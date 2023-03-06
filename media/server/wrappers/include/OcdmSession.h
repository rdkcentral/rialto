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

#ifndef FIREBOLT_RIALTO_SERVER_OCDM_SESSION_H_
#define FIREBOLT_RIALTO_SERVER_OCDM_SESSION_H_

#include "IOcdmSession.h"
#include "IOcdmSessionClient.h"
#include "opencdm/open_cdm.h"
#include "opencdm/open_cdm_adapter.h"
#include "opencdm/open_cdm_ext.h"
#include <memory>
#include <string>

namespace firebolt::rialto::server
{
/**
 * @brief The definition of the OcdmSession.
 */
class OcdmSession : public IOcdmSession
{
public:
    /**
     * @brief The constructor.
     *
     * @param[in]  systemHandle : The system handle for this session.
     * @param[in]  client       : Client object for callbacks.
     */
    OcdmSession(struct OpenCDMSystem *systemHandle, IOcdmSessionClient *client);

    /**
     * @brief Virtual destructor.
     */
    virtual ~OcdmSession();

    MediaKeyErrorStatus constructSession(KeySessionType sessionType, InitDataType initDataType,
                                         const uint8_t initData[], uint32_t initDataSize) override;

    MediaKeyErrorStatus getChallengeData(bool isLDL, uint8_t *challenge, uint32_t *challengeSize) override;

    MediaKeyErrorStatus storeLicenseData(const uint8_t challengeData[], uint32_t challengeSize) override;

    MediaKeyErrorStatus load() override;

    MediaKeyErrorStatus update(const uint8_t response[], uint32_t responseSize) override;

    MediaKeyErrorStatus decryptBuffer(GstBuffer *encrypted, GstCaps *caps) override;

    //TODO(RIALTO-127): Remove
    MediaKeyErrorStatus decrypt(GstBuffer *encrypted, GstBuffer *subSample, const uint32_t subSampleCount,
                                GstBuffer *IV, GstBuffer *keyId, uint32_t initWithLast15, GstCaps *caps) override;

    MediaKeyErrorStatus remove();

    MediaKeyErrorStatus close();

    MediaKeyErrorStatus cancelChallengeData();

    MediaKeyErrorStatus cleanDecryptContext();

    MediaKeyErrorStatus destructSession();

    KeyStatus getStatus(const uint8_t keyId[], const uint8_t keyIdLength);

    MediaKeyErrorStatus getCdmKeySessionId(std::string &cdmKeySessionId) override;

    MediaKeyErrorStatus selectKeyId(uint8_t keyLength, const uint8_t keyId[]) override;

    uint32_t hasKeyId(const uint8_t keyId[], const uint8_t keyIdSize) override;

    MediaKeyErrorStatus setDrmHeader(const uint8_t drmHeader[], uint32_t drmHeaderSize) override;

    MediaKeyErrorStatus getLastDrmError(uint32_t &errorCode) override;

    void handleProcessChallenge(struct OpenCDMSession *session, const char url[], const uint8_t challenge[],
                                const uint16_t challengeLength);

    void handleKeyUpdated(struct OpenCDMSession *session, const uint8_t keyId[], const uint8_t keyIdLength);

    void handleAllKeysUpdated(const struct OpenCDMSession *session);

    void handleError(struct OpenCDMSession *session, const char message[]);

private:
    using OcdmGstSessionDecryptExFn = OpenCDMError (*)(struct OpenCDMSession *, GstBuffer *, GstBuffer *,
                                                       const uint32_t, GstBuffer *, GstBuffer *, uint32_t, GstCaps *);
    /**
     * @brief The System handle.
     */
    struct OpenCDMSystem *m_systemHandle;

    /**
     * @brief The media keys client to callback.
     */
    IOcdmSessionClient *m_ocdmSessionClient;

    /**
     * @brief The ocdm callback methods.
     */
    OpenCDMSessionCallbacks m_ocdmCallbacks;

    /**
     * @brief The session pointer.
     */
    struct OpenCDMSession *m_session;

    static OcdmGstSessionDecryptExFn m_ocdmGstSessionDecryptEx;

    /**
     * @brief Requests the processing of the challenge data.
     *
     * @param[in]  session      : The ocdm session object pointer.
     * @param[in]  user_data        : The ocdm client pointer.
     * @param[in]  url              : The url string.
     * @param[in]  challenge        : The challenge id buffer.
     * @param[in]  challengeLength  : The length of the challenge buffer.
     */
    static void onProcessChallenge(struct OpenCDMSession *session, void *userData, const char url[],
                                   const uint8_t challenge[], const uint16_t challengeLength);

    /**
     * @brief Notifys of a key status change for key_id.
     *
     * @param[in]  session      : The ocdm session object pointer.
     * @param[in]  user_data    : The ocdm client pointer.
     * @param[in]  keyId        : The key id buffer.
     * @param[in]  keyIdLength  : The length of the key id buffer.
     */
    static void onKeyUpdated(struct OpenCDMSession *session, void *userData, const uint8_t keyId[],
                             const uint8_t keyIdLength);

    /**
     * @brief Notifys that all the key statuses have been updated.
     *
     * @param[in]  session      : The ocdm session object pointer.
     * @param[in]  user_data    : The ocdm client pointer.
     */
    static void onAllKeysUpdated(const struct OpenCDMSession *session, void *userData);

    /**
     * @brief Notifys of a new ocdm error.
     *
     * @param[in]  session      : The ocdm session object pointer.
     * @param[in]  user_data    : The ocdm client pointer.
     * @param[in]  message      : The error message received.
     */
    static void onError(struct OpenCDMSession *session, void *userData, const char message[]);
};

}; // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_OCDM_SESSION_H_
