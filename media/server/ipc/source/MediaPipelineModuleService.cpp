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

#include "MediaPipelineModuleService.h"
#include "IMediaPipelineService.h"
#include "MediaPipelineClient.h"
#include "RialtoCommonModule.h"
#include "RialtoServerLogging.h"
#include <IIpcController.h>
#include <algorithm>
#include <cstdint>

namespace
{
int generateSessionId()
{
    static int sessionId{0};
    return sessionId++;
}

firebolt::rialto::MediaType convertMediaType(const firebolt::rialto::LoadRequest_MediaType &mediaType)
{
    switch (mediaType)
    {
    case firebolt::rialto::LoadRequest_MediaType::LoadRequest_MediaType_UNKNOWN:
    {
        return firebolt::rialto::MediaType::UNKNOWN;
    }
    case firebolt::rialto::LoadRequest_MediaType::LoadRequest_MediaType_MSE:
    {
        return firebolt::rialto::MediaType::MSE;
    }
    }
    return firebolt::rialto::MediaType::UNKNOWN;
}

firebolt::rialto::MediaSourceStatus
convertMediaSourceStatus(const firebolt::rialto::HaveDataRequest_MediaSourceStatus &status)
{
    switch (status)
    {
    case firebolt::rialto::HaveDataRequest_MediaSourceStatus_UNKNOWN:
    {
        return firebolt::rialto::MediaSourceStatus::ERROR;
    }
    case firebolt::rialto::HaveDataRequest_MediaSourceStatus_OK:
    {
        return firebolt::rialto::MediaSourceStatus::OK;
    }
    case firebolt::rialto::HaveDataRequest_MediaSourceStatus_EOS:
    {
        return firebolt::rialto::MediaSourceStatus::EOS;
    }
    case firebolt::rialto::HaveDataRequest_MediaSourceStatus_ERROR:
    {
        return firebolt::rialto::MediaSourceStatus::ERROR;
    }
    case firebolt::rialto::HaveDataRequest_MediaSourceStatus_CODEC_CHANGED:
    {
        return firebolt::rialto::MediaSourceStatus::CODEC_CHANGED;
    }
    case firebolt::rialto::HaveDataRequest_MediaSourceStatus_NO_AVAILABLE_SAMPLES:
    {
        return firebolt::rialto::MediaSourceStatus::NO_AVAILABLE_SAMPLES;
    }
    }
    return firebolt::rialto::MediaSourceStatus::ERROR;
}

firebolt::rialto::SourceConfigType convertConfigType(const firebolt::rialto::AttachSourceRequest_ConfigType &configType)
{
    switch (configType)
    {
    case firebolt::rialto::AttachSourceRequest_ConfigType_CONFIG_TYPE_UNKNOWN:
    {
        return firebolt::rialto::SourceConfigType::UNKNOWN;
    }
    case firebolt::rialto::AttachSourceRequest_ConfigType_CONFIG_TYPE_AUDIO:
    {
        return firebolt::rialto::SourceConfigType::AUDIO;
    }
    case firebolt::rialto::AttachSourceRequest_ConfigType_CONFIG_TYPE_VIDEO:
    {
        return firebolt::rialto::SourceConfigType::VIDEO;
    }
    case firebolt::rialto::AttachSourceRequest_ConfigType_CONFIG_TYPE_VIDEO_DOLBY_VISION:
    {
        return firebolt::rialto::SourceConfigType::VIDEO_DOLBY_VISION;
    }
    }
    return firebolt::rialto::SourceConfigType::UNKNOWN;
}

firebolt::rialto::SegmentAlignment
convertSegmentAlignment(const firebolt::rialto::AttachSourceRequest_SegmentAlignment &alignment)
{
    switch (alignment)
    {
    case firebolt::rialto::AttachSourceRequest_SegmentAlignment_ALIGNMENT_UNDEFINED:
    {
        return firebolt::rialto::SegmentAlignment::UNDEFINED;
    }
    case firebolt::rialto::AttachSourceRequest_SegmentAlignment_ALIGNMENT_NAL:
    {
        return firebolt::rialto::SegmentAlignment::NAL;
    }
    case firebolt::rialto::AttachSourceRequest_SegmentAlignment_ALIGNMENT_AU:
    {
        return firebolt::rialto::SegmentAlignment::AU;
    }
    }

    return firebolt::rialto::SegmentAlignment::UNDEFINED;
}

firebolt::rialto::StreamFormat convertStreamFormat(const firebolt::rialto::AttachSourceRequest_StreamFormat &streamFormat)
{
    switch (streamFormat)
    {
    case firebolt::rialto::AttachSourceRequest_StreamFormat_STREAM_FORMAT_RAW:
    {
        return firebolt::rialto::StreamFormat::RAW;
    }
    case firebolt::rialto::AttachSourceRequest_StreamFormat_STREAM_FORMAT_AVC:
    {
        return firebolt::rialto::StreamFormat::AVC;
    }
    case firebolt::rialto::AttachSourceRequest_StreamFormat_STREAM_FORMAT_BYTE_STREAM:
    {
        return firebolt::rialto::StreamFormat::BYTE_STREAM;
    }
    default:
        return firebolt::rialto::StreamFormat::UNDEFINED;
    }
}

} // namespace

namespace firebolt::rialto::server::ipc
{
std::shared_ptr<IMediaPipelineModuleServiceFactory> IMediaPipelineModuleServiceFactory::createFactory()
{
    std::shared_ptr<IMediaPipelineModuleServiceFactory> factory;

    try
    {
        factory = std::make_shared<MediaPipelineModuleServiceFactory>();
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the media player module service factory, reason: %s", e.what());
    }

    return factory;
}

std::shared_ptr<IMediaPipelineModuleService>
MediaPipelineModuleServiceFactory::create(service::IMediaPipelineService &mediaPipelineService) const
{
    std::shared_ptr<IMediaPipelineModuleService> mediaPipelineModule;

    try
    {
        mediaPipelineModule = std::make_shared<MediaPipelineModuleService>(mediaPipelineService);
    }
    catch (const std::exception &e)
    {
        RIALTO_SERVER_LOG_ERROR("Failed to create the media player module service, reason: %s", e.what());
    }

    return mediaPipelineModule;
}

MediaPipelineModuleService::MediaPipelineModuleService(service::IMediaPipelineService &mediaPipelineService)
    : m_mediaPipelineService{mediaPipelineService}
{
}

MediaPipelineModuleService::~MediaPipelineModuleService() {}

void MediaPipelineModuleService::clientConnected(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient)
{
    RIALTO_SERVER_LOG_INFO("Client Connected!");
    {
        m_clientSessions.emplace(ipcClient, std::set<int>());
    }
    ipcClient->exportService(shared_from_this());
}

void MediaPipelineModuleService::clientDisconnected(const std::shared_ptr<::firebolt::rialto::ipc::IClient> &ipcClient)
{
    RIALTO_SERVER_LOG_INFO("Client disconnected!");
    std::set<int> sessionIds;
    {
        auto sessionIter = m_clientSessions.find(ipcClient);
        if (sessionIter == m_clientSessions.end())
        {
            RIALTO_SERVER_LOG_ERROR("unknown client disconnected");
            return;
        }
        sessionIds = sessionIter->second; // copy to avoid deadlock
        m_clientSessions.erase(sessionIter);
    }
    for (const auto &sessionId : sessionIds)
    {
        m_mediaPipelineService.destroySession(sessionId);
    }
}

void MediaPipelineModuleService::createSession(::google::protobuf::RpcController *controller,
                                               const ::firebolt::rialto::CreateSessionRequest *request,
                                               ::firebolt::rialto::CreateSessionResponse *response,
                                               ::google::protobuf::Closure *done)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
    auto ipcController = dynamic_cast<firebolt::rialto::ipc::IController *>(controller);
    if (!ipcController)
    {
        RIALTO_SERVER_LOG_ERROR("ipc library provided incompatible controller object");
        controller->SetFailed("ipc library provided incompatible controller object");
        done->Run();
        return;
    }
    int sessionId = generateSessionId();
    bool sessionCreated =
        m_mediaPipelineService.createSession(sessionId,
                                             std::make_shared<MediaPipelineClient>(sessionId, ipcController->getClient()),
                                             request->max_width(), request->max_height());
    if (sessionCreated)
    {
        // Assume that IPC library works well and client is present
        m_clientSessions[ipcController->getClient()].insert(sessionId);
        response->set_session_id(sessionId);
    }
    else
    {
        RIALTO_SERVER_LOG_ERROR("Create session failed");
        controller->SetFailed("Operation failed");
    }
    done->Run();
}

void MediaPipelineModuleService::destroySession(::google::protobuf::RpcController *controller,
                                                const ::firebolt::rialto::DestroySessionRequest *request,
                                                ::firebolt::rialto::DestroySessionResponse *response,
                                                ::google::protobuf::Closure *done)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
    auto ipcController = dynamic_cast<firebolt::rialto::ipc::IController *>(controller);
    if (!ipcController)
    {
        RIALTO_SERVER_LOG_ERROR("ipc library provided incompatible controller object");
        controller->SetFailed("ipc library provided incompatible controller object");
        done->Run();
        return;
    }
    if (!m_mediaPipelineService.destroySession(request->session_id()))
    {
        RIALTO_SERVER_LOG_ERROR("Destroy session failed");
        controller->SetFailed("Operation failed");
        done->Run();
        return;
    }
    auto sessionIter = m_clientSessions.find(ipcController->getClient());
    if (sessionIter != m_clientSessions.end())
    {
        sessionIter->second.erase(request->session_id());
    }
    done->Run();
}

void MediaPipelineModuleService::load(::google::protobuf::RpcController *controller,
                                      const ::firebolt::rialto::LoadRequest *request,
                                      ::firebolt::rialto::LoadResponse *response, ::google::protobuf::Closure *done)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
    if (!m_mediaPipelineService.load(request->session_id(), convertMediaType(request->type()), request->mime_type(),
                                     request->url()))
    {
        RIALTO_SERVER_LOG_ERROR("Load failed");
        controller->SetFailed("Operation failed");
    }
    done->Run();
}

void MediaPipelineModuleService::setVideoWindow(::google::protobuf::RpcController *controller,
                                                const ::firebolt::rialto::SetVideoWindowRequest *request,
                                                ::firebolt::rialto::SetVideoWindowResponse *response,
                                                ::google::protobuf::Closure *done)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
    if (!m_mediaPipelineService.setVideoWindow(request->session_id(), request->x(), request->y(), request->width(),
                                               request->height()))
    {
        RIALTO_SERVER_LOG_ERROR("Set Video Window failed");
        controller->SetFailed("Operation failed");
    }
    done->Run();
}

void MediaPipelineModuleService::attachSource(::google::protobuf::RpcController *controller,
                                              const ::firebolt::rialto::AttachSourceRequest *request,
                                              ::firebolt::rialto::AttachSourceResponse *response,
                                              ::google::protobuf::Closure *done)
{
    RIALTO_SERVER_LOG_DEBUG("mime_type: %s", request->mime_type().c_str());

    CodecData codecData{};
    if (request->has_codec_data())
    {
        auto codecDataProto = request->codec_data();
        codecData = CodecData{{codecDataProto.begin(), codecDataProto.end()}};
    }
    std::unique_ptr<IMediaPipeline::MediaSource> mediaSource;
    firebolt::rialto::SourceConfigType configType = convertConfigType(request->config_type());

    if (configType == firebolt::rialto::SourceConfigType::AUDIO)
    {
        const auto &config = request->audio_config();
        uint32_t numberofchannels = config.number_of_channels();
        uint32_t sampleRate = config.sample_rate();

        std::vector<uint8_t> codecSpecificConfig;
        if (config.has_codec_specific_config())
        {
            auto codecSpecificConfigStr = config.codec_specific_config();
            codecSpecificConfig.assign(codecSpecificConfigStr.begin(), codecSpecificConfigStr.end());
        }
        AudioConfig audioConfig{numberofchannels, sampleRate, codecSpecificConfig};

        mediaSource =
            std::make_unique<IMediaPipeline::MediaSourceAudio>(0, request->mime_type(), audioConfig,
                                                               convertSegmentAlignment(request->segment_alignment()),
                                                               convertStreamFormat(request->stream_format()), codecData);
    }
    else if (configType == firebolt::rialto::SourceConfigType::VIDEO)
    {
        mediaSource =
            std::make_unique<IMediaPipeline::MediaSourceVideo>(0, request->mime_type().c_str(),
                                                               convertSegmentAlignment(request->segment_alignment()),
                                                               convertStreamFormat(request->stream_format()), codecData);
    }
    else if (configType == firebolt::rialto::SourceConfigType::VIDEO_DOLBY_VISION)
    {
        mediaSource =
            std::make_unique<IMediaPipeline::MediaSourceVideoDolbyVision>(0, request->mime_type().c_str(),
                                                                          request->dolby_vision_profile(),
                                                                          convertSegmentAlignment(
                                                                              request->segment_alignment()),
                                                                          convertStreamFormat(request->stream_format()),
                                                                          codecData);
    }

    if (!m_mediaPipelineService.attachSource(request->session_id(), mediaSource))
    {
        RIALTO_SERVER_LOG_ERROR("Attach source failed");
        controller->SetFailed("Operation failed");
    }
    response->set_source_id(mediaSource->getId());
    done->Run();
}

void MediaPipelineModuleService::removeSource(::google::protobuf::RpcController *controller,
                                              const ::firebolt::rialto::RemoveSourceRequest *request,
                                              ::firebolt::rialto::RemoveSourceResponse *response,
                                              ::google::protobuf::Closure *done)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
    if (!m_mediaPipelineService.removeSource(request->session_id(), request->source_id()))
    {
        RIALTO_SERVER_LOG_ERROR("Remove source failed");
        controller->SetFailed("Operation failed");
    }
    done->Run();
}

void MediaPipelineModuleService::play(::google::protobuf::RpcController *controller,
                                      const ::firebolt::rialto::PlayRequest *request,
                                      ::firebolt::rialto::PlayResponse *response, ::google::protobuf::Closure *done)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
    if (!m_mediaPipelineService.play(request->session_id()))
    {
        RIALTO_SERVER_LOG_ERROR("Play failed");
        controller->SetFailed("Operation failed");
    }
    done->Run();
}

void MediaPipelineModuleService::pause(::google::protobuf::RpcController *controller,
                                       const ::firebolt::rialto::PauseRequest *request,
                                       ::firebolt::rialto::PauseResponse *response, ::google::protobuf::Closure *done)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
    if (!m_mediaPipelineService.pause(request->session_id()))
    {
        RIALTO_SERVER_LOG_ERROR("pause failed");
        controller->SetFailed("Operation failed");
    }
    done->Run();
}

void MediaPipelineModuleService::stop(::google::protobuf::RpcController *controller,
                                      const ::firebolt::rialto::StopRequest *request,
                                      ::firebolt::rialto::StopResponse *response, ::google::protobuf::Closure *done)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
    if (!m_mediaPipelineService.stop(request->session_id()))
    {
        RIALTO_SERVER_LOG_ERROR("Stop failed");
        controller->SetFailed("Operation failed");
    }
    done->Run();
}

void MediaPipelineModuleService::setPosition(::google::protobuf::RpcController *controller,
                                             const ::firebolt::rialto::SetPositionRequest *request,
                                             ::firebolt::rialto::SetPositionResponse *response,
                                             ::google::protobuf::Closure *done)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
    if (!m_mediaPipelineService.setPosition(request->session_id(), request->position()))
    {
        RIALTO_SERVER_LOG_ERROR("Set Position failed");
        controller->SetFailed("Operation failed");
    }
    done->Run();
}

void MediaPipelineModuleService::haveData(::google::protobuf::RpcController *controller,
                                          const ::firebolt::rialto::HaveDataRequest *request,
                                          ::firebolt::rialto::HaveDataResponse *response,
                                          ::google::protobuf::Closure *done)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
    firebolt::rialto::MediaSourceStatus status{convertMediaSourceStatus(request->status())};
    if (!m_mediaPipelineService.haveData(request->session_id(), status, request->num_frames(), request->request_id()))
    {
        RIALTO_SERVER_LOG_ERROR("Have data failed");
        controller->SetFailed("Operation failed");
    }
    done->Run();
}

void MediaPipelineModuleService::setPlaybackRate(::google::protobuf::RpcController *controller,
                                                 const ::firebolt::rialto::SetPlaybackRateRequest *request,
                                                 ::firebolt::rialto::SetPlaybackRateResponse *response,
                                                 ::google::protobuf::Closure *done)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
    if (!m_mediaPipelineService.setPlaybackRate(request->session_id(), request->rate()))
    {
        RIALTO_SERVER_LOG_ERROR("Set playback rate failed");
        controller->SetFailed("Operation failed");
    }
    done->Run();
}

void MediaPipelineModuleService::getPosition(::google::protobuf::RpcController *controller,
                                             const ::firebolt::rialto::GetPositionRequest *request,
                                             ::firebolt::rialto::GetPositionResponse *response,
                                             ::google::protobuf::Closure *done)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
    int64_t position{};
    if (!m_mediaPipelineService.getPosition(request->session_id(), position))
    {
        RIALTO_SERVER_LOG_ERROR("Get position failed");
        controller->SetFailed("Operation failed");
    }
    else
    {
        response->set_position(position);
    }
    done->Run();
}

void MediaPipelineModuleService::renderFrame(::google::protobuf::RpcController *controller,
                                             const ::firebolt::rialto::RenderFrameRequest *request,
                                             ::firebolt::rialto::RenderFrameResponse *response,
                                             ::google::protobuf::Closure *done)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    if (!m_mediaPipelineService.renderFrame(request->session_id()))
    {
        RIALTO_SERVER_LOG_ERROR("Render frame");
        controller->SetFailed("Operation failed");
    }

    done->Run();
}

void MediaPipelineModuleService::setVolume(::google::protobuf::RpcController *controller,
                                           const ::firebolt::rialto::SetVolumeRequest *request,
                                           ::firebolt::rialto::SetVolumeResponse *response,
                                           ::google::protobuf::Closure *done)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");

    if (!m_mediaPipelineService.setVolume(request->session_id(), request->volume()))
    {
        RIALTO_SERVER_LOG_ERROR("Set volume failed.");
        controller->SetFailed("Operation failed");
    }

    done->Run();
}

void MediaPipelineModuleService::getVolume(::google::protobuf::RpcController *controller,
                                           const ::firebolt::rialto::GetVolumeRequest *request,
                                           ::firebolt::rialto::GetVolumeResponse *response,
                                           ::google::protobuf::Closure *done)
{
    RIALTO_SERVER_LOG_DEBUG("entry:");
    double volume{};

    if (!m_mediaPipelineService.getVolume(request->session_id(), volume))
    {
        RIALTO_SERVER_LOG_ERROR("Get volume failed.");
        controller->SetFailed("Operation failed");
    }
    else
    {
        response->set_volume(volume);
    }

    done->Run();
}
} // namespace firebolt::rialto::server::ipc
