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

#ifndef MEDIA_KEYS_MODULE_MOCK_H_
#define MEDIA_KEYS_MODULE_MOCK_H_

#include "MediaKeysProtoUtils.h"
#include "mediakeysmodule.pb.h"
#include <gmock/gmock.h>

class MediaKeysModuleMock : public ::firebolt::rialto::MediaKeysModule
{
public:
    MOCK_METHOD(void, createMediaKeys,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::CreateMediaKeysRequest *request,
                 ::firebolt::rialto::CreateMediaKeysResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, destroyMediaKeys,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::DestroyMediaKeysRequest *request,
                 ::firebolt::rialto::DestroyMediaKeysResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, containsKey,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::ContainsKeyRequest *request,
                 ::firebolt::rialto::ContainsKeyResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, createKeySession,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::CreateKeySessionRequest *request,
                 ::firebolt::rialto::CreateKeySessionResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, generateRequest,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::GenerateRequestRequest *request,
                 ::firebolt::rialto::GenerateRequestResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, loadSession,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::LoadSessionRequest *request,
                 ::firebolt::rialto::LoadSessionResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, updateSession,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::UpdateSessionRequest *request,
                 ::firebolt::rialto::UpdateSessionResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, setDrmHeader,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::SetDrmHeaderRequest *request,
                 ::firebolt::rialto::SetDrmHeaderResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, closeKeySession,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::CloseKeySessionRequest *request,
                 ::firebolt::rialto::CloseKeySessionResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, removeKeySession,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::RemoveKeySessionRequest *request,
                 ::firebolt::rialto::RemoveKeySessionResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, deleteDrmStore,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::DeleteDrmStoreRequest *request,
                 ::firebolt::rialto::DeleteDrmStoreResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, deleteKeyStore,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::DeleteKeyStoreRequest *request,
                 ::firebolt::rialto::DeleteKeyStoreResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, getDrmStoreHash,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::GetDrmStoreHashRequest *request,
                 ::firebolt::rialto::GetDrmStoreHashResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, getKeyStoreHash,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::GetKeyStoreHashRequest *request,
                 ::firebolt::rialto::GetKeyStoreHashResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, getLdlSessionsLimit,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::GetLdlSessionsLimitRequest *request,
                 ::firebolt::rialto::GetLdlSessionsLimitResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, getLastDrmError,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::GetLastDrmErrorRequest *request,
                 ::firebolt::rialto::GetLastDrmErrorResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, getDrmTime,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::GetDrmTimeRequest *request,
                 ::firebolt::rialto::GetDrmTimeResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, getCdmKeySessionId,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::GetCdmKeySessionIdRequest *request,
                 ::firebolt::rialto::GetCdmKeySessionIdResponse *response, ::google::protobuf::Closure *done));

    void defaultReturn(::google::protobuf::RpcController *controller, ::google::protobuf::Closure *done)
    {
        done->Run();
    }

    void failureReturn(::google::protobuf::RpcController *controller, ::google::protobuf::Closure *done)
    {
        controller->SetFailed("Failed for some reason ...");
        done->Run();
    }

    ::firebolt::rialto::CreateMediaKeysResponse createMediaKeysResponse(const int32_t mediaKeysHandle)
    {
        firebolt::rialto::CreateMediaKeysResponse response;
        response.set_media_keys_handle(mediaKeysHandle);
        return response;
    }

    ::firebolt::rialto::CreateKeySessionResponse
    createKeySessionResponse(const firebolt::rialto::MediaKeyErrorStatus &status, const int32_t keySessionId)
    {
        firebolt::rialto::CreateKeySessionResponse response;
        response.set_error_status(convertMediaKeyErrorStatus(status));
        response.set_key_session_id(keySessionId);
        return response;
    }

    ::firebolt::rialto::GenerateRequestResponse generateRequestResponse(const firebolt::rialto::MediaKeyErrorStatus &status)
    {
        firebolt::rialto::GenerateRequestResponse response;
        response.set_error_status(convertMediaKeyErrorStatus(status));
        return response;
    }

    ::firebolt::rialto::UpdateSessionResponse updateSessionResponse(const firebolt::rialto::MediaKeyErrorStatus &status)
    {
        firebolt::rialto::UpdateSessionResponse response;
        response.set_error_status(convertMediaKeyErrorStatus(status));
        return response;
    }

    ::firebolt::rialto::CloseKeySessionResponse closeKeySessionResponse(const firebolt::rialto::MediaKeyErrorStatus &status)
    {
        firebolt::rialto::CloseKeySessionResponse response;
        response.set_error_status(convertMediaKeyErrorStatus(status));
        return response;
    }

    ::firebolt::rialto::LoadSessionResponse loadSessionResponse(const firebolt::rialto::MediaKeyErrorStatus &status)
    {
        firebolt::rialto::LoadSessionResponse response;
        response.set_error_status(convertMediaKeyErrorStatus(status));
        return response;
    }

    ::firebolt::rialto::ContainsKeyResponse containsKeyResponse(const bool containsKey)
    {
        firebolt::rialto::ContainsKeyResponse response;
        response.set_contains_key(containsKey);
        return response;
    }

    ::firebolt::rialto::RemoveKeySessionResponse removeKeySessionResponse(const firebolt::rialto::MediaKeyErrorStatus &status)
    {
        firebolt::rialto::RemoveKeySessionResponse response;
        response.set_error_status(convertMediaKeyErrorStatus(status));
        return response;
    }

    ::firebolt::rialto::GetKeyStoreHashResponse getKeyStoreHashResponse(const firebolt::rialto::MediaKeyErrorStatus &status, const std::vector<unsigned char> &keyStoreHash)
    {
        firebolt::rialto::GetKeyStoreHashResponse response;
        response.set_error_status(convertMediaKeyErrorStatus(status));
        for (auto it = keyStoreHash.begin(); it != keyStoreHash.end(); it++)
        {
            response.add_key_store_hash(*it);
        }
        return response;
    }

    ::firebolt::rialto::GetDrmStoreHashResponse getDrmStoreHashResponse(const firebolt::rialto::MediaKeyErrorStatus &status, const std::vector<unsigned char> &drmStoreHash)
    {
        firebolt::rialto::GetDrmStoreHashResponse response;
        response.set_error_status(convertMediaKeyErrorStatus(status));
        for (auto it = drmStoreHash.begin(); it != drmStoreHash.end(); it++)
        {
            response.add_drm_store_hash(*it);
        }
        return response;
    }

    ::firebolt::rialto::DeleteKeyStoreResponse deleteKeyStoreResponse(const firebolt::rialto::MediaKeyErrorStatus &status)
    {
        firebolt::rialto::DeleteKeyStoreResponse response;
        response.set_error_status(convertMediaKeyErrorStatus(status));
        return response;
    }

    ::firebolt::rialto::DeleteDrmStoreResponse deleteDrmStoreResponse(const firebolt::rialto::MediaKeyErrorStatus &status)
    {
        firebolt::rialto::DeleteDrmStoreResponse response;
        response.set_error_status(convertMediaKeyErrorStatus(status));
        return response;
    }

    ::firebolt::rialto::SetDrmHeaderResponse setDrmHeaderResponse(const firebolt::rialto::MediaKeyErrorStatus &status)
    {
        firebolt::rialto::SetDrmHeaderResponse response;
        response.set_error_status(convertMediaKeyErrorStatus(status));
        return response;
    }

    ::firebolt::rialto::GetCdmKeySessionIdResponse getCdmKeySessionIdResponse(const firebolt::rialto::MediaKeyErrorStatus &status, const std::string &cdmKeySessionId)
    {
        firebolt::rialto::GetCdmKeySessionIdResponse response;
        response.set_error_status(convertMediaKeyErrorStatus(status));
        response.set_cdm_key_session_id(cdmKeySessionId);
        return response;
    }

    ::firebolt::rialto::GetLastDrmErrorResponse getLastDrmErrorResponse(const firebolt::rialto::MediaKeyErrorStatus &status, const uint32_t errorCode)
    {
        firebolt::rialto::GetLastDrmErrorResponse response;
        response.set_error_status(convertMediaKeyErrorStatus(status));
        response.set_error_code(errorCode);
        return response;
    }

    ::firebolt::rialto::GetLdlSessionsLimitResponse getLdlSessionsLimitResponse(const firebolt::rialto::MediaKeyErrorStatus &status, const uint32_t ldlLimit)
    {
        firebolt::rialto::GetLdlSessionsLimitResponse response;
        response.set_error_status(convertMediaKeyErrorStatus(status));
        response.set_ldl_limit(ldlLimit);
        return response;
    }

    ::firebolt::rialto::GetDrmTimeResponse getDrmTimeResponse(const firebolt::rialto::MediaKeyErrorStatus &status, const uint64_t drmTime)
    {
        firebolt::rialto::GetDrmTimeResponse response;
        response.set_error_status(convertMediaKeyErrorStatus(status));
        response.set_drm_time(drmTime);
        return response;
    }

    MediaKeysModuleMock() {}
    virtual ~MediaKeysModuleMock() = default;
};

#endif // MEDIA_KEYS_MODULE_MOCK_H_
