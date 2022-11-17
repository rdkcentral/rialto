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

#ifndef FIREBOLT_RIALTO_SERVER_MEDIA_KEYS_SERVER_INTERNAL_H_
#define FIREBOLT_RIALTO_SERVER_MEDIA_KEYS_SERVER_INTERNAL_H_

#include "IMainThread.h"
#include "IMediaKeySession.h"
#include "IMediaKeysServerInternal.h"
#include "IOcdmSystem.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace firebolt::rialto::server
{
/**
 * @brief IMediaKeys factory class definition.
 */
class MediaKeysServerInternalFactory : public IMediaKeysServerInternalFactory
{
public:
    MediaKeysServerInternalFactory() = default;
    ~MediaKeysServerInternalFactory() override = default;

    std::unique_ptr<IMediaKeys> createMediaKeys(const std::string &keySystem) const override;
    std::unique_ptr<IMediaKeysServerInternal> createMediaKeysServerInternal(const std::string &keySystem) const override;
};

}; // namespace firebolt::rialto::server

namespace firebolt::rialto::server
{
/**
 * @brief The definition of the MediaKeysServerInternal.
 */
class MediaKeysServerInternal : public IMediaKeysServerInternal
{
public:
    /**
     * @brief The constructor.
     *
     * @param[in] keySystem                 : The key system for which to create a Media Keys instance.
     * @param[in] mainThreadFactory         : The main thread factory.
     * @param[in] ocdmSystemFactory         : The ocdm system factory.
     * @param[in] mediaKeySessionFactory    : The media key session factory.
     *
     */
    MediaKeysServerInternal(const std::string &keySystem, const std::shared_ptr<IMainThreadFactory> &mainThreadFactory,
                            std::shared_ptr<IOcdmSystemFactory> ocdmSystemFactory,
                            std::shared_ptr<IMediaKeySessionFactory> mediaKeySessionFactory);

    /**
     * @brief Virtual destructor.
     */
    virtual ~MediaKeysServerInternal();

    MediaKeyErrorStatus selectKeyId(int32_t keySessionId, const std::vector<uint8_t> &keyId) override;

    bool containsKey(int32_t keySessionId, const std::vector<uint8_t> &keyId) override;

    MediaKeyErrorStatus createKeySession(KeySessionType sessionType, std::weak_ptr<IMediaKeysClient> client, bool isLDL,
                                         int32_t &keySessionId) override;

    MediaKeyErrorStatus generateRequest(int32_t keySessionId, InitDataType initDataType,
                                        const std::vector<uint8_t> &initData) override;

    MediaKeyErrorStatus loadSession(int32_t keySessionId) override;

    MediaKeyErrorStatus updateSession(int32_t keySessionId, const std::vector<uint8_t> &responseData) override;

    MediaKeyErrorStatus setDrmHeader(int32_t keySessionId, const std::vector<uint8_t> &requestData) override;

    MediaKeyErrorStatus closeKeySession(int32_t keySessionId) override;

    MediaKeyErrorStatus removeKeySession(int32_t keySessionId) override;

    MediaKeyErrorStatus deleteDrmStore() override;

    MediaKeyErrorStatus deleteKeyStore() override;

    MediaKeyErrorStatus getDrmStoreHash(std::vector<unsigned char> &drmStoreHash) override;

    MediaKeyErrorStatus getKeyStoreHash(std::vector<unsigned char> &keyStoreHash) override;

    MediaKeyErrorStatus getLdlSessionsLimit(uint32_t &ldlLimit) override;

    MediaKeyErrorStatus getLastDrmError(uint32_t &errorCode) override;

    MediaKeyErrorStatus getDrmTime(uint64_t &drmTime) override;

    MediaKeyErrorStatus getCdmKeySessionId(int32_t keySessionId, std::string &cdmKeySessionId) override;

    MediaKeyErrorStatus decrypt(int32_t keySessionId, GstBuffer *encrypted, GstBuffer *subSample,
                                const uint32_t subSampleCount, GstBuffer *IV, GstBuffer *keyId,
                                uint32_t initWithLast15) override;

    bool hasSession(int32_t keySessionId) const override;

private:
    /**
     * @brief The mainThread object.
     */
    std::shared_ptr<IMainThread> m_mainThread;

    /**
     * @brief The factory for creating MediaKeySessions.
     */
    std::shared_ptr<IMediaKeySessionFactory> m_mediaKeySessionFactory;

    /**
     * @brief The IOcdmSystem instance.
     */
    std::unique_ptr<IOcdmSystem> m_ocdmSystem;

    /**
     * @brief Map containing created sessions.
     */
    std::map<int32_t, std::unique_ptr<IMediaKeySession>> m_mediaKeySessions;

    /**
     * @brief KeySystem type of the MediaKeysServerInternal.
     */
    const std::string m_keySystem;

    /**
     * @brief This objects id registered on the main thread
     */
    uint32_t m_mainThreadClientId;

    /**
     * @brief Creates a session internally, only to be called on the main thread.
     *
     * @param[in]  sessionType : The session type.
     * @param[in]  client      : Client object for callbacks
     * @param[in]  isLDL       : Is this an LDL
     * @param[out] keySessionId: The key session id
     *
     * @retval an error status.
     */
    MediaKeyErrorStatus createKeySessionInternal(KeySessionType sessionType, std::weak_ptr<IMediaKeysClient> client,
                                                 bool isLDL, int32_t &keySessionId);

    /**
     * @brief Generate internally, only to be called on the main thread.
     *
     * @param[in]  keySessionId : The key session id for the session.
     * @param[in]  initDataType : The init data type.
     * @param[in]  initData     : The init data.
     *
     * @retval an error status.
     */
    MediaKeyErrorStatus generateRequestInternal(int32_t keySessionId, InitDataType initDataType,
                                                const std::vector<uint8_t> &initData);

    /**
     * @brief Load internally, only to be called on the main thread.
     *
     * @param[in] keySessionId : The key session id for the session.
     *
     * @retval an error status.
     */
    MediaKeyErrorStatus loadSessionInternal(int32_t keySessionId);

    /**
     * @brief Update internally, only to be called on the main thread.
     *
     * @param[in] keySessionId : The key session id for the session.
     * @param[in] responseData : The license response data.
     *
     * @retval an error status.
     */
    MediaKeyErrorStatus updateSessionInternal(int32_t keySessionId, const std::vector<uint8_t> &responseData);

    /**
     * @brief Close a key session internally, only to be called on the main thread.
     *
     * @param[in] keySessionId : The key session id.
     *
     * @retval an error status.
     */
    MediaKeyErrorStatus closeKeySessionInternal(int32_t keySessionId);

    /**
     * @brief Removes a key session internally, only to be called on the main thread.
     *
     * @param[in] keySessionId : The key session id.
     *
     * @retval an error status.
     */
    MediaKeyErrorStatus removeKeySessionInternal(int32_t keySessionId);

    /**
     * @brief Get the key session id internally, only to be called on the main thread.
     *
     * @param[in]   keySessionId    : The key session id for the session.
     * @param[out]  cdmKeySessionId : The internal CDM key session ID
     *
     * @retval an error status.
     */
    MediaKeyErrorStatus getCdmKeySessionIdInternal(int32_t keySessionId, std::string &cdmKeySessionId);

    /**
     * @brief Decrypt internally, only to be called on the main thread.
     *
     * @param[in] keySessionId    : The session id for the session.
     * @param[in]  encrypted      : Gstreamer buffer containing encrypted data and related meta data. If applicable,
     *                              decrypted data will be stored here after this call returns.
     * @param[in]  subSample      : Gstreamer buffer containing subsamples size which has been parsed from protection
     *                              meta data.
     * @param[in]  subSampleCount : count of subsamples
     * @param[in]  IV             : Gstreamer buffer containing initial vector (IV) used during decryption.
     * @param[in]  keyId          : Gstreamer buffer containing keyID to use for decryption
     * @param[in]  initWithLast15 : The value deciding whether decryption context needs to be initialized with
     *                              last 15 bytes. Currently this only applies to PlayReady DRM.
     *
     * @retval an error status.
     */
    MediaKeyErrorStatus decryptInternal(int32_t keySessionId, GstBuffer *encrypted, GstBuffer *subSample,
                                        const uint32_t subSampleCount, GstBuffer *IV, GstBuffer *keyId,
                                        uint32_t initWithLast15);
};

}; // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_MEDIA_KEYS_SERVER_INTERNAL_H_
