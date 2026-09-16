/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
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

#ifndef MEDIA_CAPABILITIES_MODULE_MOCK_H_
#define MEDIA_CAPABILITIES_MODULE_MOCK_H_

#include "AudioDecoderCapabilities.h"
#include "VideoDecoderCapabilities.h"
#include "mediacapabilitiesmodule.pb.h"
#include <gmock/gmock.h>

class MediaCapabilitiesModuleMock : public ::firebolt::rialto::MediaCapabilitiesModule
{
public:
    MOCK_METHOD(void, getSupportedAudioCapabilities,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::GetSupportedAudioCapabilitiesRequest *request,
                 ::firebolt::rialto::AudioCapabilities *response, ::google::protobuf::Closure *done),
                (override));

    MOCK_METHOD(void, getSupportedVideoCapabilities,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::GetSupportedVideoCapabilitiesRequest *request,
                 ::firebolt::rialto::VideoCapabilities *response, ::google::protobuf::Closure *done),
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

    ::firebolt::rialto::AudioCapabilities
    getSupportedAudioCapabilitiesResponse(const firebolt::rialto::common::AudioDecoderCapabilities &audioCapabilities)
    {
        firebolt::rialto::AudioCapabilities response;
        response.set_interface_version(audioCapabilities.interfaceVersion);
        response.set_schema_version(audioCapabilities.schemaVersion);
        return response;
    }

    ::firebolt::rialto::VideoCapabilities
    getSupportedVideoCapabilitiesResponse(const firebolt::rialto::common::VideoDecoderCapabilities &videoCapabilities)
    {
        firebolt::rialto::VideoCapabilities response;
        response.set_interface_version(videoCapabilities.interfaceVersion);
        response.set_schema_version(videoCapabilities.schemaVersion);
        return response;
    }

    MediaCapabilitiesModuleMock() {}
    virtual ~MediaCapabilitiesModuleMock() = default;
};

#endif // MEDIA_CAPABILITIES_MODULE_MOCK_H_
