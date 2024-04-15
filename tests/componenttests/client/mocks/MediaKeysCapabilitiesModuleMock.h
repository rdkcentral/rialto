/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Sky UK
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

#ifndef MEDIA_KEYS_CAPABILITIES_MODULE_MOCK_H_
#define MEDIA_KEYS_CAPABILITIES_MODULE_MOCK_H_

#include "MediaKeysProtoUtils.h"
#include "mediakeyscapabilitiesmodule.pb.h"
#include <gmock/gmock.h>
#include <string>
#include <vector>

class MediaKeysCapabilitiesModuleMock : public ::firebolt::rialto::MediaKeysCapabilitiesModule
{
public:
    MOCK_METHOD(void, getSupportedKeySystems,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::GetSupportedKeySystemsRequest *request,
                 ::firebolt::rialto::GetSupportedKeySystemsResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, supportsKeySystem,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::SupportsKeySystemRequest *request,
                 ::firebolt::rialto::SupportsKeySystemResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, getSupportedKeySystemVersion,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::GetSupportedKeySystemVersionRequest *request,
                 ::firebolt::rialto::GetSupportedKeySystemVersionResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, isServerCertificateSupported,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::IsServerCertificateSupportedRequest *request,
                 ::firebolt::rialto::IsServerCertificateSupportedResponse *response, ::google::protobuf::Closure *done));

    void defaultReturn(::google::protobuf::RpcController *controller, ::google::protobuf::Closure *done)
    {
        done->Run();
    }

    void failureReturn(::google::protobuf::RpcController *controller, ::google::protobuf::Closure *done)
    {
        controller->SetFailed("Failed for some reason ...");
        done->Run();
    }

    ::firebolt::rialto::GetSupportedKeySystemsResponse getSupportedKeySystemsResponse(const std::vector<std::string> &values)
    {
        firebolt::rialto::GetSupportedKeySystemsResponse response;

        for (const auto &value : values)
        {
            response.add_key_systems(value);
        }

        return response;
    }

    ::firebolt::rialto::SupportsKeySystemResponse supportsKeySystemResponse(const bool value)
    {
        firebolt::rialto::SupportsKeySystemResponse response;
        response.set_is_supported(value);
        return response;
    }

    ::firebolt::rialto::GetSupportedKeySystemVersionResponse getSupportedKeySystemVersionResponse(const ::std::string &value)
    {
        firebolt::rialto::GetSupportedKeySystemVersionResponse response;
        response.set_version(value);
        return response;
    }

    ::firebolt::rialto::IsServerCertificateSupportedResponse supportsServerCertificateResponse(const bool value)
    {
        firebolt::rialto::IsServerCertificateSupportedResponse response;
        response.set_is_supported(value);
        return response;
    }

    MediaKeysCapabilitiesModuleMock() {}
    virtual ~MediaKeysCapabilitiesModuleMock() = default;
};

#endif // MEDIA_KEYS_CAPABILITIES_MODULE_MOCK_H_
