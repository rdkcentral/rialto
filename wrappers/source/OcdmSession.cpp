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

#include "OcdmSession.h"
#include "OcdmCommon.h"
#include "RialtoCommonLogging.h"
#include "opencdm/open_cdm_adapter.h"
#include "opencdm/open_cdm_ext.h"
#include <dlfcn.h>
#include <mutex>
namespace
{
LicenseType convertLicenseType(const firebolt::rialto::KeySessionType &sessionType)
{
    switch (sessionType)
    {
    case firebolt::rialto::KeySessionType::PERSISTENT_LICENCE:
    {
        return PersistentLicense;
    }
    case firebolt::rialto::KeySessionType::TEMPORARY:
    case firebolt::rialto::KeySessionType::PERSISTENT_RELEASE_MESSAGE:
    case firebolt::rialto::KeySessionType::UNKNOWN:
    default:
    {
        return Temporary;
    }
    }
}
const char *convertInitDataType(const firebolt::rialto::InitDataType &initDataType)
{
    switch (initDataType)
    {
    case firebolt::rialto::InitDataType::CENC:
    {
        return "cenc";
    }
    case firebolt::rialto::InitDataType::KEY_IDS:
    {
        return "keyids";
    }
    case firebolt::rialto::InitDataType::WEBM:
    {
        return "webm";
    }
    case firebolt::rialto::InitDataType::DRMHEADER:
    {
        return "drmheader";
    }
    case firebolt::rialto::InitDataType::UNKNOWN:
    default:
    {
        return "unknown";
    }
    }
}
const firebolt::rialto::KeyStatus convertKeyStatus(const KeyStatus &ocdmKeyStatus)
{
    switch (ocdmKeyStatus)
    {
    case Usable:
    {
        return firebolt::rialto::KeyStatus::USABLE;
    }
    case Expired:
    {
        return firebolt::rialto::KeyStatus::EXPIRED;
    }
    case Released:
    {
        return firebolt::rialto::KeyStatus::RELEASED;
    }
    case OutputRestricted:
    case OutputRestrictedHDCP22:
    {
        return firebolt::rialto::KeyStatus::OUTPUT_RESTRICTED;
    }
    case StatusPending:
    {
        return firebolt::rialto::KeyStatus::PENDING;
    }
    case OutputDownscaled:
    case InternalError:
    case HWError:
    default:
    {
        return firebolt::rialto::KeyStatus::INTERNAL_ERROR;
    }
    }
}
} // namespace

namespace firebolt::rialto::wrappers
{
OcdmSession::OcdmGstSessionDecryptExFn OcdmSession::m_ocdmGstSessionDecryptEx{nullptr};
OcdmSession::OcdmGstSessionDecryptBufferOnceFn OcdmSession::m_ocdmGstSessionDecryptBufferOnce{nullptr};

OcdmSession::OcdmSession(struct OpenCDMSystem *systemHandle, IOcdmSessionClient *client)
    : m_systemHandle(systemHandle), m_ocdmSessionClient(client), m_session(nullptr)
{
    m_ocdmCallbacks = {&OcdmSession::onProcessChallenge, &OcdmSession::onKeyUpdated, &OcdmSession::onError,
                       &OcdmSession::onAllKeysUpdated};
    static std::once_flag flag;
    std::call_once(flag,
                   []()
                   {
                       void *handle = dlopen("libocdm.so", RTLD_LAZY);
                       m_ocdmGstSessionDecryptEx =
                           (OcdmGstSessionDecryptExFn)dlsym(RTLD_DEFAULT, "opencdm_gstreamer_session_decrypt_ex");
                       m_ocdmGstSessionDecryptBufferOnce =
                           (OcdmGstSessionDecryptBufferOnceFn)dlsym(handle,
                                                                    "opencdm_gstreamer_session_decrypt_buffer_once");
                       if (m_ocdmGstSessionDecryptBufferOnce != NULL)
                       {
                           RIALTO_COMMON_LOG_ERROR("DEBUG PURPOSE : m_ocdmGstSessionDecryptBufferOnce exists\n");
                       }
                   });
}

OcdmSession::~OcdmSession() {}

MediaKeyErrorStatus OcdmSession::constructSession(KeySessionType sessionType, InitDataType initDataType,
                                                  const uint8_t initData[], uint32_t initDataSize)
{
    if (m_session)
    {
        return MediaKeyErrorStatus::OK;
    }

    OpenCDMError status = opencdm_construct_session(m_systemHandle, convertLicenseType(sessionType),
                                                    convertInitDataType(initDataType), initData, initDataSize, nullptr,
                                                    0, &m_ocdmCallbacks, this, &m_session);
    return convertOpenCdmError(status);
}

MediaKeyErrorStatus OcdmSession::getChallengeData(bool isLDL, uint8_t *challenge, uint32_t *challengeSize)
{
    if (!m_session)
    {
        return MediaKeyErrorStatus::FAIL;
    }

    OpenCDMError status =
        opencdm_session_get_challenge_data(m_session, challenge, challengeSize, static_cast<uint32_t>(isLDL));
    return convertOpenCdmError(status);
}

MediaKeyErrorStatus OcdmSession::storeLicenseData(const uint8_t challenge[], uint32_t challengeSize)
{
    if (!m_session)
    {
        return MediaKeyErrorStatus::FAIL;
    }

    // Rialto does not support secureStop, but we need to pass buffer to avoid ocdm crash
    constexpr size_t kSecureStopOutputBufferSize{16};
    unsigned char secureStopBuffer[kSecureStopOutputBufferSize];
    OpenCDMError status = opencdm_session_store_license_data(m_session, challenge, challengeSize, secureStopBuffer);
    return convertOpenCdmError(status);
}

MediaKeyErrorStatus OcdmSession::load()
{
    if (!m_session)
    {
        return MediaKeyErrorStatus::FAIL;
    }

    OpenCDMError status = opencdm_session_load(m_session);
    return convertOpenCdmError(status);
}

MediaKeyErrorStatus OcdmSession::update(const uint8_t response[], uint32_t responseSize)
{
    if (!m_session)
    {
        return MediaKeyErrorStatus::FAIL;
    }

    OpenCDMError status = opencdm_session_update(m_session, response, responseSize);
    return convertOpenCdmError(status);
}

MediaKeyErrorStatus OcdmSession::decryptBuffer(GstBuffer *encrypted, GstCaps *caps)
{
    if (!m_session)
    {
        return MediaKeyErrorStatus::FAIL;
    }

    if (m_ocdmGstSessionDecryptBufferOnce)
    {
        // Extract key ID from the buffer's protection metadata
        std::vector<uint8_t> keyId;
        GstProtectionMeta *pm = reinterpret_cast<GstProtectionMeta *>(gst_buffer_get_protection_meta(encrypted));
        if (pm)
        {
            const GValue *kidValue = gst_structure_get_value(pm->info, "kid");
            if (kidValue)
            {
                GstBuffer *kidBuf = gst_value_get_buffer(kidValue);
                if (kidBuf)
                {
                    GstMapInfo kidMap;
                    if (gst_buffer_map(kidBuf, &kidMap, GST_MAP_READ))
                    {
                        keyId.assign(kidMap.data, kidMap.data + kidMap.size);
                        gst_buffer_unmap(kidBuf, &kidMap);
                    }
                }
            }
        }

        // Pre-decrypt key status check: return OUTPUT_RESTRICTED immediately (no sleep) so
        // the caller (MediaKeysServerInternal::decrypt) can retry from the GStreamer thread.
        if (!keyId.empty())
        {
            const ::KeyStatus preStatus =
                opencdm_session_status(m_session, keyId.data(), static_cast<uint8_t>(keyId.size()));
            if (preStatus == OutputRestricted || preStatus == OutputRestrictedHDCP22)
            {
                RIALTO_COMMON_LOG_ERROR("DEBUG PURPOSE : OcdmSession::decryptBuffer() : returning "
                                        "MediaKeyErrorStatus::OUTPUT_RESTRICTED(Pre decrypt)\n");
                return MediaKeyErrorStatus::OUTPUT_RESTRICTED;
            }
        }

        OpenCDMError result = m_ocdmGstSessionDecryptBufferOnce(m_session, encrypted, caps);

        // Post-decrypt status check: a failed decrypt during HDCP reauth may not carry a
        // specific error code, so confirm via key status before signalling the caller to retry.
        if (result != ERROR_NONE && !keyId.empty())
        {
            const ::KeyStatus postStatus =
                opencdm_session_status(m_session, keyId.data(), static_cast<uint8_t>(keyId.size()));
            if (postStatus == OutputRestricted || postStatus == OutputRestrictedHDCP22)
            {
                RIALTO_COMMON_LOG_ERROR("DEBUG PURPOSE : OcdmSession::decryptBuffer() : returning "
                                        "MediaKeyErrorStatus::OUTPUT_RESTRICTED(Post decrypt)\n");
                return MediaKeyErrorStatus::OUTPUT_RESTRICTED;
            }
        }

        return convertOpenCdmError(result);
    }

    // Fallback: adapter without _once handles retries internally.
    OpenCDMError status = opencdm_gstreamer_session_decrypt_buffer(m_session, encrypted, caps);
    return convertOpenCdmError(status);
}

MediaKeyErrorStatus OcdmSession::remove()
{
    if (!m_session)
    {
        return MediaKeyErrorStatus::FAIL;
    }

    OpenCDMError status = opencdm_session_remove(m_session);
    return convertOpenCdmError(status);
}

MediaKeyErrorStatus OcdmSession::close()
{
    if (!m_session)
    {
        return MediaKeyErrorStatus::FAIL;
    }

    OpenCDMError status = opencdm_session_close(m_session);
    return convertOpenCdmError(status);
}

MediaKeyErrorStatus OcdmSession::cancelChallengeData()
{
    if (!m_session)
    {
        return MediaKeyErrorStatus::FAIL;
    }

    OpenCDMError status = opencdm_session_cancel_challenge_data(m_session);
    return convertOpenCdmError(status);
}

MediaKeyErrorStatus OcdmSession::cleanDecryptContext()
{
    if (!m_session)
    {
        return MediaKeyErrorStatus::FAIL;
    }

    OpenCDMError status = opencdm_session_clean_decrypt_context(m_session);
    return convertOpenCdmError(status);
}

MediaKeyErrorStatus OcdmSession::destructSession()
{
    if (!m_session)
    {
        return MediaKeyErrorStatus::FAIL;
    }

    OpenCDMError status = opencdm_destruct_session(m_session);
    return convertOpenCdmError(status);
}

KeyStatus OcdmSession::getStatus(const uint8_t keyId[], const uint8_t keyIdLength)
{
    return convertKeyStatus(opencdm_session_status(m_session, keyId, keyIdLength));
}

MediaKeyErrorStatus OcdmSession::getCdmKeySessionId(std::string &cdmKeySessionId)
{
    if (!m_session)
    {
        return MediaKeyErrorStatus::FAIL;
    }
    cdmKeySessionId = std::string{opencdm_session_id(m_session)};
    return MediaKeyErrorStatus::OK;
}

MediaKeyErrorStatus OcdmSession::selectKeyId(uint8_t keyLength, const uint8_t keyId[])
{
    if (!m_session)
    {
        return MediaKeyErrorStatus::FAIL;
    }
    OpenCDMError status = opencdm_session_select_key_id(m_session, keyLength, keyId);
    return convertOpenCdmError(status);
}

uint32_t OcdmSession::hasKeyId(const uint8_t keyId[], const uint8_t keyIdSize)
{
    if (!m_session)
    {
        return 0;
    }
    return opencdm_session_has_key_id(m_session, keyIdSize, keyId);
}

MediaKeyErrorStatus OcdmSession::setDrmHeader(const uint8_t drmHeader[], uint32_t drmHeaderSize)
{
    if (!m_session)
    {
        return MediaKeyErrorStatus::FAIL;
    }
    OpenCDMError status = opencdm_session_set_drm_header(m_session, drmHeader, drmHeaderSize);
    return convertOpenCdmError(status);
}

MediaKeyErrorStatus OcdmSession::getLastDrmError(uint32_t &errorCode)
{
    if (!m_session)
    {
        return MediaKeyErrorStatus::FAIL;
    }
    OpenCDMError returnedError = opencdm_session_system_error(m_session);
    errorCode = static_cast<uint32_t>(returnedError);
    return MediaKeyErrorStatus::OK;
}

void OcdmSession::handleProcessChallenge(struct OpenCDMSession *session, const char url[], const uint8_t challenge[],
                                         const uint16_t challengeLength)
{
    if (m_ocdmSessionClient)
    {
        m_ocdmSessionClient->onProcessChallenge(url, challenge, challengeLength);
    }
}

void OcdmSession::handleKeyUpdated(struct OpenCDMSession *session, const uint8_t keyId[], const uint8_t keyIdLength)
{
    if (m_ocdmSessionClient)
    {
        m_ocdmSessionClient->onKeyUpdated(keyId, keyIdLength);
    }
}

void OcdmSession::handleAllKeysUpdated(const struct OpenCDMSession *session)
{
    if (m_ocdmSessionClient)
    {
        m_ocdmSessionClient->onAllKeysUpdated();
    }
}

void OcdmSession::handleError(struct OpenCDMSession *session, const char message[])
{
    if (m_ocdmSessionClient)
    {
        m_ocdmSessionClient->onError(message);
    }
}

void OcdmSession::onProcessChallenge(OpenCDMSession *session, void *userData, const char url[],
                                     const uint8_t challenge[], const uint16_t challengeLength)
{
    OcdmSession *ocdmSession = static_cast<OcdmSession *>(userData);

    if (ocdmSession)
    {
        ocdmSession->handleProcessChallenge(session, url, challenge, challengeLength);
    }
}

void OcdmSession::onKeyUpdated(struct OpenCDMSession *session, void *userData, const uint8_t keyId[],
                               const uint8_t keyIdLength)
{
    OcdmSession *ocdmSession = static_cast<OcdmSession *>(userData);

    if (ocdmSession)
    {
        ocdmSession->handleKeyUpdated(session, keyId, keyIdLength);
    }
}

void OcdmSession::onAllKeysUpdated(const struct OpenCDMSession *session, void *userData)
{
    OcdmSession *ocdmSession = static_cast<OcdmSession *>(userData);

    if (ocdmSession)
    {
        ocdmSession->handleAllKeysUpdated(session);
    }
}

void OcdmSession::onError(struct OpenCDMSession *session, void *userData, const char message[])
{
    OcdmSession *ocdmSession = static_cast<OcdmSession *>(userData);

    if (ocdmSession)
    {
        ocdmSession->handleError(session, message);
    }
}
}; // namespace firebolt::rialto::wrappers
