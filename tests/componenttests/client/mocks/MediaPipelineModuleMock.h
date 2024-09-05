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

#ifndef MEDIA_PIPELINE_MODULE_MOCK_H_
#define MEDIA_PIPELINE_MODULE_MOCK_H_

#include "SchemaVersion.h"
#include "mediapipelinemodule.pb.h"
#include <gmock/gmock.h>

class MediaPipelineModuleMock : public ::firebolt::rialto::MediaPipelineModule
{
public:
    MOCK_METHOD(void, createSession,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::CreateSessionRequest *request,
                 ::firebolt::rialto::CreateSessionResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, destroySession,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::DestroySessionRequest *request,
                 ::firebolt::rialto::DestroySessionResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, load,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::LoadRequest *request,
                 ::firebolt::rialto::LoadResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, attachSource,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::AttachSourceRequest *request,
                 ::firebolt::rialto::AttachSourceResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, removeSource,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::RemoveSourceRequest *request,
                 ::firebolt::rialto::RemoveSourceResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, allSourcesAttached,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::AllSourcesAttachedRequest *request,
                 ::firebolt::rialto::AllSourcesAttachedResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, setVideoWindow,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::SetVideoWindowRequest *request,
                 ::firebolt::rialto::SetVideoWindowResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, play,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::PlayRequest *request,
                 ::firebolt::rialto::PlayResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, pause,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::PauseRequest *request,
                 ::firebolt::rialto::PauseResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, stop,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::StopRequest *request,
                 ::firebolt::rialto::StopResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, setPosition,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::SetPositionRequest *request,
                 ::firebolt::rialto::SetPositionResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, getPosition,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::GetPositionRequest *request,
                 ::firebolt::rialto::GetPositionResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, setImmediateOutput,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::SetImmediateOutputRequest *request,
                 ::firebolt::rialto::SetImmediateOutputResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, getImmediateOutput,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::GetImmediateOutputRequest *request,
                 ::firebolt::rialto::GetImmediateOutputResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, getStats,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::GetStatsRequest *request,
                 ::firebolt::rialto::GetStatsResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, setPlaybackRate,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::SetPlaybackRateRequest *request,
                 ::firebolt::rialto::SetPlaybackRateResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, haveData,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::HaveDataRequest *request,
                 ::firebolt::rialto::HaveDataResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, renderFrame,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::RenderFrameRequest *request,
                 ::firebolt::rialto::RenderFrameResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, setVolume,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::SetVolumeRequest *request,
                 ::firebolt::rialto::SetVolumeResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, getVolume,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::GetVolumeRequest *request,
                 ::firebolt::rialto::GetVolumeResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, setMute,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::SetMuteRequest *request,
                 ::firebolt::rialto::SetMuteResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, getMute,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::GetMuteRequest *request,
                 ::firebolt::rialto::GetMuteResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, setLowLatency,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::SetLowLatencyRequest *request,
                 ::firebolt::rialto::SetLowLatencyResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, setSync,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::SetSyncRequest *request,
                 ::firebolt::rialto::SetSyncResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, getSync,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::GetSyncRequest *request,
                 ::firebolt::rialto::GetSyncResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, setSyncOff,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::SetSyncOffRequest *request,
                 ::firebolt::rialto::SetSyncOffResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, setStreamSyncMode,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::SetStreamSyncModeRequest *request,
                 ::firebolt::rialto::SetStreamSyncModeResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, getStreamSyncMode,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::GetStreamSyncModeRequest *request,
                 ::firebolt::rialto::GetStreamSyncModeResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, flush,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::FlushRequest *request,
                 ::firebolt::rialto::FlushResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, setSourcePosition,
                (::google::protobuf::RpcController * controller,
                 const ::firebolt::rialto::SetSourcePositionRequest *request,
                 ::firebolt::rialto::SetSourcePositionResponse *response, ::google::protobuf::Closure *done));
    MOCK_METHOD(void, processAudioGap,
                (::google::protobuf::RpcController * controller, const ::firebolt::rialto::ProcessAudioGapRequest *request,
                 ::firebolt::rialto::ProcessAudioGapResponse *response, ::google::protobuf::Closure *done));

    void defaultReturn(::google::protobuf::RpcController *controller, ::google::protobuf::Closure *done)
    {
        done->Run();
    }

    void failureReturn(::google::protobuf::RpcController *controller, ::google::protobuf::Closure *done)
    {
        controller->SetFailed("Failed for some reason ...");
        done->Run();
    }

    ::firebolt::rialto::CreateSessionResponse createSessionResponse(const int32_t sessionId)
    {
        firebolt::rialto::CreateSessionResponse response;
        response.set_session_id(sessionId);
        return response;
    }

    ::firebolt::rialto::AttachSourceResponse attachSourceResponse(const int32_t sourceId)
    {
        firebolt::rialto::AttachSourceResponse response;
        response.set_source_id(sourceId);
        return response;
    }

    ::firebolt::rialto::GetVolumeResponse getVolumeResponse(const double volume)
    {
        firebolt::rialto::GetVolumeResponse response;
        response.set_volume(volume);
        return response;
    }

    ::firebolt::rialto::GetMuteResponse getMuteResponse(const bool mute)
    {
        firebolt::rialto::GetMuteResponse response;
        response.set_mute(mute);
        return response;
    }

    ::firebolt::rialto::GetSyncResponse getSyncResponse(const bool sync)
    {
        firebolt::rialto::GetSyncResponse response;
        response.set_sync(sync);
        return response;
    }

    ::firebolt::rialto::GetStreamSyncModeResponse getStreamSyncModeResponse(const int32_t streamSyncMode)
    {
        firebolt::rialto::GetStreamSyncModeResponse response;
        response.set_stream_sync_mode(streamSyncMode);
        return response;
    }

    ::firebolt::rialto::GetPositionResponse getPositionResponse(const int64_t position)
    {
        firebolt::rialto::GetPositionResponse response;
        response.set_position(position);
        return response;
    }

    ::firebolt::rialto::SetImmediateOutputResponse setImmediateOutputResponse()
    {
        firebolt::rialto::SetImmediateOutputResponse response;
        return response;
    }

    ::firebolt::rialto::GetImmediateOutputResponse getImmediateOutputResponse(bool immediateOutputResponse)
    {
        firebolt::rialto::GetImmediateOutputResponse response;
        response.set_immediate_output(immediateOutputResponse);
        return response;
    }

    ::firebolt::rialto::GetStatsResponse getStatsResponse(const uint64_t renderedFrames, const uint64_t droppedFrames)
    {
        firebolt::rialto::GetStatsResponse response;
        response.set_rendered_frames(renderedFrames);
        response.set_dropped_frames(droppedFrames);
        return response;
    }

    MediaPipelineModuleMock() {}
    virtual ~MediaPipelineModuleMock() = default;
};

#endif // MEDIA_PIPELINE_MODULE_MOCK_H_
