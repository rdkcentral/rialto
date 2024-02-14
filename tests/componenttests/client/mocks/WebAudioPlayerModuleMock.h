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

#ifndef WEB_AUDIO_PLAYER_MODULE_MOCK_H_
#define WEB_AUDIO_PLAYER_MODULE_MOCK_H_

#include "webaudioplayermodule.pb.h"
#include <gmock/gmock.h>
#include <string>
#include <vector>

class WebAudioPlayerModuleMock : public ::firebolt::rialto::WebAudioPlayerModule
{
public:
    MOCK_METHOD(void, createWebAudioPlayer,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::CreateWebAudioPlayerRequest *request,
                 ::firebolt::rialto::CreateWebAudioPlayerResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, destroyWebAudioPlayer,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::DestroyWebAudioPlayerRequest *request,
                 ::firebolt::rialto::DestroyWebAudioPlayerResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, getDeviceInfo,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::WebAudioGetDeviceInfoRequest *request,
                 ::firebolt::rialto::WebAudioGetDeviceInfoResponse *response, ::google::protobuf::Closure *done));

    void defaultReturn(::google::protobuf::RpcController *controller, ::google::protobuf::Closure *done)
    {
        done->Run();
    }

    void failureReturn(::google::protobuf::RpcController *controller, ::google::protobuf::Closure *done)
    {
        controller->SetFailed("Failed for some reason ...");
        done->Run();
    }

    ::firebolt::rialto::CreateWebAudioPlayerResponse createWebAudioPlayerResponse(const int &value)
    {
        firebolt::rialto::CreateWebAudioPlayerResponse response;
        response.set_web_audio_player_handle(value);
        return response;
    }

    ::firebolt::rialto::WebAudioGetDeviceInfoResponse webAudioGetDeviceInfoResponse(const uint32_t &preferredFrames,
                                                                                    const uint32_t &maximumFrames,
                                                                                    const bool &supportDeferredPlay)
    {
        firebolt::rialto::WebAudioGetDeviceInfoResponse response;
        response.set_preferred_frames(preferredFrames);
        response.set_maximum_frames(maximumFrames);
        response.set_support_deferred_play(supportDeferredPlay);
        return response;
    }

    WebAudioPlayerModuleMock() {}
    virtual ~WebAudioPlayerModuleMock() = default;
};

#endif // WEB_AUDIO_PLAYER_MODULE_MOCK_H_
