/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Sky UK
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

#ifndef MEDIA_KEYS_PROTO_REQUEST_MATCHERS_H_
#define MEDIA_KEYS_PROTO_REQUEST_MATCHERS_H_

#include "MediaCommon.h"
#include "mediakeyscapabilitiesmodule.pb.h"
#include "mediakeysmodule.pb.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <vector>

MATCHER_P(createMediaKeysRequestMatcher, keySystem, "")
{
    const ::firebolt::rialto::CreateMediaKeysRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::CreateMediaKeysRequest *>(arg);
    return (kRequest->key_system() == keySystem);
}

MATCHER_P(destroyMediaKeysRequestMatcher, mediaKeysHandle, "")
{
    const ::firebolt::rialto::DestroyMediaKeysRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::DestroyMediaKeysRequest *>(arg);
    return (kRequest->media_keys_handle() == mediaKeysHandle);
}

MATCHER_P3(createKeySessionRequestMatcher, mediaKeysHandle, sessionType, isLdl, "")
{
    const ::firebolt::rialto::CreateKeySessionRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::CreateKeySessionRequest *>(arg);
    return ((kRequest->media_keys_handle() == mediaKeysHandle) && (kRequest->session_type() == sessionType) &&
            (kRequest->is_ldl() == isLdl));
}

MATCHER_P4(generateRequestRequestMatcher, mediaKeysHandle, keySessionId, initDataType, initData, "")
{
    const ::firebolt::rialto::GenerateRequestRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::GenerateRequestRequest *>(arg);
    return ((kRequest->media_keys_handle() == mediaKeysHandle) && (kRequest->key_session_id() == keySessionId) &&
            (kRequest->init_data_type() == initDataType) &&
            (std::vector<std::uint8_t>{kRequest->init_data().begin(), kRequest->init_data().end()} == initData));
}

MATCHER_P3(updateSessionRequestMatcher, mediaKeysHandle, keySessionId, requestData, "")
{
    const ::firebolt::rialto::UpdateSessionRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::UpdateSessionRequest *>(arg);
    return (
        (kRequest->media_keys_handle() == mediaKeysHandle) && (kRequest->key_session_id() == keySessionId) &&
        (std::vector<std::uint8_t>{kRequest->response_data().begin(), kRequest->response_data().end()} == requestData));
}

MATCHER_P2(closeKeySessionRequestMatcher, mediaKeysHandle, keySessionId, "")
{
    const ::firebolt::rialto::CloseKeySessionRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::CloseKeySessionRequest *>(arg);
    return ((kRequest->media_keys_handle() == mediaKeysHandle) && (kRequest->key_session_id() == keySessionId));
}

MATCHER_P2(loadSessionRequestMatcher, mediaKeysHandle, keySessionId, "")
{
    const ::firebolt::rialto::LoadSessionRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::LoadSessionRequest *>(arg);
    return ((kRequest->media_keys_handle() == mediaKeysHandle) && (kRequest->key_session_id() == keySessionId));
}

MATCHER_P3(containsKeyRequestMatcher, mediaKeysHandle, keySessionId, keyId, "")
{
    const ::firebolt::rialto::ContainsKeyRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::ContainsKeyRequest *>(arg);
    bool keyIdMatch{keyId.size() == static_cast<size_t>(kRequest->key_id().size())};
    if (keyIdMatch)
    {
        for (size_t i = 0; i < keyId.size(); ++i)
        {
            keyIdMatch &= kRequest->key_id(i) == keyId[i];
        }
    }
    return ((kRequest->media_keys_handle() == mediaKeysHandle) && (kRequest->key_session_id() == keySessionId) &&
            keyIdMatch);
}

MATCHER_P2(removeKeySessionRequestMatcher, mediaKeysHandle, keySessionId, "")
{
    const ::firebolt::rialto::RemoveKeySessionRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::RemoveKeySessionRequest *>(arg);
    return ((kRequest->media_keys_handle() == mediaKeysHandle) && (kRequest->key_session_id() == keySessionId));
}

MATCHER_P(getKeyStoreHashRequestMatcher, mediaKeysHandle, "")
{
    const ::firebolt::rialto::GetKeyStoreHashRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::GetKeyStoreHashRequest *>(arg);
    return (kRequest->media_keys_handle() == mediaKeysHandle);
}

MATCHER_P(getDrmStoreHashRequestMatcher, mediaKeysHandle, "")
{
    const ::firebolt::rialto::GetDrmStoreHashRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::GetDrmStoreHashRequest *>(arg);
    return (kRequest->media_keys_handle() == mediaKeysHandle);
}

MATCHER_P(deleteKeyStoreRequestMatcher, mediaKeysHandle, "")
{
    const ::firebolt::rialto::DeleteKeyStoreRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::DeleteKeyStoreRequest *>(arg);
    return (kRequest->media_keys_handle() == mediaKeysHandle);
}

MATCHER_P(deleteDrmStoreRequestMatcher, mediaKeysHandle, "")
{
    const ::firebolt::rialto::DeleteDrmStoreRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::DeleteDrmStoreRequest *>(arg);
    return (kRequest->media_keys_handle() == mediaKeysHandle);
}

MATCHER_P3(setDrmHeaderRequestMatcher, mediaKeysHandle, keySessionId, drmHeader, "")
{
    const ::firebolt::rialto::SetDrmHeaderRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::SetDrmHeaderRequest *>(arg);
    bool drmHeaderMatch{drmHeader.size() == static_cast<size_t>(kRequest->request_data().size())};
    if (drmHeaderMatch)
    {
        for (size_t i = 0; i < drmHeader.size(); ++i)
        {
            drmHeaderMatch &= kRequest->request_data(i) == drmHeader[i];
        }
    }
    return ((kRequest->media_keys_handle() == mediaKeysHandle) && (kRequest->key_session_id() == keySessionId) &&
            drmHeaderMatch);
}

MATCHER_P2(getCdmKeySessionIdRequestMatcher, mediaKeysHandle, keySessionId, "")
{
    const ::firebolt::rialto::GetCdmKeySessionIdRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::GetCdmKeySessionIdRequest *>(arg);
    return ((kRequest->media_keys_handle() == mediaKeysHandle) && (kRequest->key_session_id() == keySessionId));
}

MATCHER_P2(getLastDrmErrorRequestMatcher, mediaKeysHandle, keySessionId, "")
{
    const ::firebolt::rialto::GetLastDrmErrorRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::GetLastDrmErrorRequest *>(arg);
    return ((kRequest->media_keys_handle() == mediaKeysHandle) && (kRequest->key_session_id() == keySessionId));
}

MATCHER_P(getLdlSessionsLimitRequestMatcher, mediaKeysHandle, "")
{
    const ::firebolt::rialto::GetLdlSessionsLimitRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::GetLdlSessionsLimitRequest *>(arg);
    return (kRequest->media_keys_handle() == mediaKeysHandle);
}

MATCHER_P(getDrmTimeRequestMatcher, mediaKeysHandle, "")
{
    const ::firebolt::rialto::GetDrmTimeRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::GetDrmTimeRequest *>(arg);
    return (kRequest->media_keys_handle() == mediaKeysHandle);
}

MATCHER_P(supportsKeySystemRequestMatcher, keySystem, "")
{
    const ::firebolt::rialto::SupportsKeySystemRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::SupportsKeySystemRequest *>(arg);
    return (kRequest->key_system() == keySystem);
}

MATCHER_P(getSupportedKeySystemVersionRequestMatcher, keySystem, "")
{
    const ::firebolt::rialto::GetSupportedKeySystemVersionRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::GetSupportedKeySystemVersionRequest *>(arg);
    return (kRequest->key_system() == keySystem);
}

MATCHER_P(isServerCertificateSupportedRequestMatcher, keySystem, "")
{
    const ::firebolt::rialto::IsServerCertificateSupportedRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::IsServerCertificateSupportedRequest *>(arg);
    return (kRequest->key_system() == keySystem);
}

MATCHER_P2(releaseKeySessionRequestMatcher, mediaKeysHandle, keySessionId, "")
{
    const ::firebolt::rialto::ReleaseKeySessionRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::ReleaseKeySessionRequest *>(arg);
    return ((kRequest->media_keys_handle() == mediaKeysHandle) && (kRequest->key_session_id() == keySessionId));
}

MATCHER_P(getMetricSystemDataRequestMatcher, mediaKeysHandle, "")
{
    const ::firebolt::rialto::GetMetricSystemDataRequest *kRequest =
        dynamic_cast<const ::firebolt::rialto::GetMetricSystemDataRequest *>(arg);
    return (kRequest->media_keys_handle() == mediaKeysHandle);
}

#endif // MEDIA_KEYS_PROTO_REQUEST_MATCHERS_H_
