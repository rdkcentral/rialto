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

#ifndef MEDIA_PIPELINE_CAPABILITIES_MODULE_MOCK_H_
#define MEDIA_PIPELINE_CAPABILITIES_MODULE_MOCK_H_

#include "mediapipelinecapabilitiesmodule.pb.h"
#include <gmock/gmock.h>
#include <string>
#include <vector>

class MediaPipelineCapabilitiesModuleMock : public ::firebolt::rialto::MediaPipelineCapabilitiesModule
{
public:
    MOCK_METHOD(void, getSupportedMimeTypes,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::GetSupportedMimeTypesRequest *request,
                 ::firebolt::rialto::GetSupportedMimeTypesResponse *response, ::google::protobuf::Closure *done),
                (override));

    MOCK_METHOD(void, isMimeTypeSupported,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::IsMimeTypeSupportedRequest *request,
                 ::firebolt::rialto::IsMimeTypeSupportedResponse *response, ::google::protobuf::Closure *done),
                (override));

    MOCK_METHOD(void, getSupportedProperties,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::GetSupportedPropertiesRequest *request,
                 ::firebolt::rialto::GetSupportedPropertiesResponse *response, ::google::protobuf::Closure *done),
                (override));

    MOCK_METHOD(void, isVideoMaster,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::IsVideoMasterCapabilityRequest *request,
                 ::firebolt::rialto::IsVideoMasterCapabilityResponse *response, ::google::protobuf::Closure *done),
                (override));

    void defaultReturn(::google::protobuf::RpcController *controller, ::google::protobuf::Closure *done)
    {
        done->Run();
    }

    void failureReturn(::google::protobuf::RpcController *controller, ::google::protobuf::Closure *done)
    {
        controller->SetFailed("Failed for some reason ...");
        done->Run();
    }

    ::firebolt::rialto::GetSupportedMimeTypesResponse getSupportedMimeTypesResponse(const std::vector<std::string> &values)
    {
        firebolt::rialto::GetSupportedMimeTypesResponse response;

        for (const std::string &mimeType : values)
        {
            response.add_mime_types(mimeType);
        }

        return response;
    }

    ::firebolt::rialto::IsMimeTypeSupportedResponse isMimeTypeSupportedResponse(const bool value)
    {
        firebolt::rialto::IsMimeTypeSupportedResponse response;
        response.set_is_supported(value);
        return response;
    }

    ::firebolt::rialto::GetSupportedPropertiesResponse getSupportedPropertiesResponse(const std::vector<std::string> &values)
    {
        firebolt::rialto::GetSupportedPropertiesResponse response;

        for (const std::string &property : values)
        {
            response.add_supported_properties(property);
        }

        return response;
    }

    MediaPipelineCapabilitiesModuleMock() {}
    virtual ~MediaPipelineCapabilitiesModuleMock() = default;
};

#endif // MEDIA_PIPELINE_CAPABILITIES_MODULE_MOCK_H_
