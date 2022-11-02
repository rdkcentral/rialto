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

#ifndef FIREBOLT_RIALTO_SERVER_IPC_MOCK_MEDIA_KEYS_MODULE_SERVICE_MOCK_H_
#define FIREBOLT_RIALTO_SERVER_IPC_MOCK_MEDIA_KEYS_MODULE_SERVICE_MOCK_H_

#include "IMediaKeysModuleService.h"
#include <gmock/gmock.h>
#include <memory>

namespace firebolt::rialto::server::ipc::mock
{
class MediaKeysModuleServiceMock : public IMediaKeysModuleService
{
public:
    MediaKeysModuleServiceMock() = default;
    virtual ~MediaKeysModuleServiceMock() = default;

    MOCK_METHOD(void, clientConnected, (const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient), (override));
    MOCK_METHOD(void, clientDisconnected, (const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient),
                (override));
    MOCK_METHOD(void, createMediaKeys,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::CreateMediaKeysRequest *request,
                 ::firebolt::rialto::CreateMediaKeysResponse *response, ::google::protobuf::Closure *done),
                (override));
    MOCK_METHOD(void, destroyMediaKeys,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::DestroyMediaKeysRequest *request,
                 ::firebolt::rialto::DestroyMediaKeysResponse *response, ::google::protobuf::Closure *done),
                (override));
    MOCK_METHOD(void, selectKeyId,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::SelectKeyIdRequest *request,
                 ::firebolt::rialto::SelectKeyIdResponse *response, ::google::protobuf::Closure *done),
                (override));
    MOCK_METHOD(void, containsKey,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::ContainsKeyRequest *request,
                 ::firebolt::rialto::ContainsKeyResponse *response, ::google::protobuf::Closure *done),
                (override));
    MOCK_METHOD(void, createKeySession,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::CreateKeySessionRequest *request,
                 ::firebolt::rialto::CreateKeySessionResponse *response, ::google::protobuf::Closure *done),
                (override));
    MOCK_METHOD(void, generateRequest,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::GenerateRequestRequest *request,
                 ::firebolt::rialto::GenerateRequestResponse *response, ::google::protobuf::Closure *done),
                (override));
    MOCK_METHOD(void, loadSession,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::LoadSessionRequest *request,
                 ::firebolt::rialto::LoadSessionResponse *response, ::google::protobuf::Closure *done),
                (override));
    MOCK_METHOD(void, updateSession,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::UpdateSessionRequest *request,
                 ::firebolt::rialto::UpdateSessionResponse *response, ::google::protobuf::Closure *done),
                (override));
    MOCK_METHOD(void, setDrmHeader,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::SetDrmHeaderRequest *request,
                 ::firebolt::rialto::SetDrmHeaderResponse *response, ::google::protobuf::Closure *done),
                (override));
    MOCK_METHOD(void, closeKeySession,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::CloseKeySessionRequest *request,
                 ::firebolt::rialto::CloseKeySessionResponse *response, ::google::protobuf::Closure *done),
                (override));
    MOCK_METHOD(void, removeKeySession,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::RemoveKeySessionRequest *request,
                 ::firebolt::rialto::RemoveKeySessionResponse *response, ::google::protobuf::Closure *done),
                (override));
    MOCK_METHOD(void, deleteDrmStore,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::DeleteDrmStoreRequest *request,
                 ::firebolt::rialto::DeleteDrmStoreResponse *response, ::google::protobuf::Closure *done),
                (override));
    MOCK_METHOD(void, deleteKeyStore,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::DeleteKeyStoreRequest *request,
                 ::firebolt::rialto::DeleteKeyStoreResponse *response, ::google::protobuf::Closure *done),
                (override));
    MOCK_METHOD(void, getDrmStoreHashStore,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::GetDrmStoreHashRequest *request,
                 ::firebolt::rialto::GetDrmStoreHashResponse *response, ::google::protobuf::Closure *done),
                (override));
    MOCK_METHOD(void, getKeyStoreHashStore,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::GetKeyStoreHashRequest *request,
                 ::firebolt::rialto::GetKeyStoreHashResponse *response, ::google::protobuf::Closure *done),
                (override));
    MOCK_METHOD(void, getLdlSessionsLimit,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::GetLdlSessionsLimitRequest *request,
                 ::firebolt::rialto::GetLdlSessionsLimitResponse *response, ::google::protobuf::Closure *done),
                (override));
    MOCK_METHOD(void, getLastDrmErrorLimit,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::GetLastDrmErrorRequest *request,
                 ::firebolt::rialto::GetLastDrmErrorResponse *response, ::google::protobuf::Closure *done),
                (override));
    MOCK_METHOD(void, getDrmTime,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::GetDrmTimeRequest *request,
                 ::firebolt::rialto::GetDrmTimeResponse *response, ::google::protobuf::Closure *done),
                (override));
};
} // namespace firebolt::rialto::server::ipc::mock

#endif // FIREBOLT_RIALTO_SERVER_IPC_MOCK_MEDIA_KEYS_MODULE_SERVICE_MOCK_H_
