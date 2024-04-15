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
#include <memory>
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
    MOCK_METHOD(void, play,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::WebAudioPlayRequest *request,
                 ::firebolt::rialto::WebAudioPlayResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, pause,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::WebAudioPauseRequest *request,
                 ::firebolt::rialto::WebAudioPauseResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, setEos,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::WebAudioSetEosRequest *request,
                 ::firebolt::rialto::WebAudioSetEosResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, getBufferAvailable,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::WebAudioGetBufferAvailableRequest *request,
                 ::firebolt::rialto::WebAudioGetBufferAvailableResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, writeBuffer,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::WebAudioWriteBufferRequest *request,
                 ::firebolt::rialto::WebAudioWriteBufferResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, getBufferDelay,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::WebAudioGetBufferDelayRequest *request,
                 ::firebolt::rialto::WebAudioGetBufferDelayResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, setVolume,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::WebAudioSetVolumeRequest *request,
                 ::firebolt::rialto::WebAudioSetVolumeResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, getVolume,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::WebAudioGetVolumeRequest *request,
                 ::firebolt::rialto::WebAudioGetVolumeResponse *response, ::google::protobuf::Closure *done));

    void defaultReturn(::google::protobuf::RpcController *controller, ::google::protobuf::Closure *done)
    {
        done->Run();
    }

    void failureReturn(::google::protobuf::RpcController *controller, ::google::protobuf::Closure *done)
    {
        controller->SetFailed("Failed for some reason ...");
        done->Run();
    }

    ::firebolt::rialto::CreateWebAudioPlayerResponse createWebAudioPlayerResponse(const int value)
    {
        firebolt::rialto::CreateWebAudioPlayerResponse response;
        response.set_web_audio_player_handle(value);
        return response;
    }

    ::firebolt::rialto::WebAudioGetDeviceInfoResponse webAudioGetDeviceInfoResponse(const uint32_t preferredFrames,
                                                                                    const uint32_t maximumFrames,
                                                                                    const bool supportDeferredPlay)
    {
        firebolt::rialto::WebAudioGetDeviceInfoResponse response;
        response.set_preferred_frames(preferredFrames);
        response.set_maximum_frames(maximumFrames);
        response.set_support_deferred_play(supportDeferredPlay);
        return response;
    }

    ::firebolt::rialto::WebAudioGetBufferAvailableResponse
    webAudioGetBufferAvailableResponse(const uint32_t availableFrames,
                                       const std::shared_ptr<firebolt::rialto::WebAudioShmInfo> &webAudioShmInfo)
    {
        firebolt::rialto::WebAudioGetBufferAvailableResponse response;
        response.set_available_frames(availableFrames);
        response.mutable_shm_info()->set_offset_main(webAudioShmInfo->offsetMain);
        response.mutable_shm_info()->set_length_main(webAudioShmInfo->lengthMain);
        response.mutable_shm_info()->set_offset_wrap(webAudioShmInfo->offsetWrap);
        response.mutable_shm_info()->set_length_wrap(webAudioShmInfo->lengthWrap);
        return response;
    }

    ::firebolt::rialto::WebAudioGetBufferDelayResponse webAudioGetBufferDelayResponse(const uint32_t delayFrames)
    {
        firebolt::rialto::WebAudioGetBufferDelayResponse response;
        response.set_delay_frames(delayFrames);
        return response;
    }

    ::firebolt::rialto::WebAudioGetVolumeResponse webAudioGetVolumeResponse(const double volume)
    {
        firebolt::rialto::WebAudioGetVolumeResponse response;
        response.set_volume(volume);
        return response;
    }

    WebAudioPlayerModuleMock() {}
    virtual ~WebAudioPlayerModuleMock() = default;
};

#endif // WEB_AUDIO_PLAYER_MODULE_MOCK_H_
