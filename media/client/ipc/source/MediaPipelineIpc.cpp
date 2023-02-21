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
#include "MediaPipelineIpc.h"
#include "IIpcClient.h"
#include "RialtoClientLogging.h"
#include "RialtoCommonIpc.h"
#include "mediapipelinemodule.pb.h"
#include <IMediaPipeline.h>

namespace firebolt::rialto::client
{
std::weak_ptr<IMediaPipelineIpcFactory> MediaPipelineIpcFactory::m_factory;

std::shared_ptr<IMediaPipelineIpcFactory> IMediaPipelineIpcFactory::getFactory()
{
    std::shared_ptr<IMediaPipelineIpcFactory> factory = MediaPipelineIpcFactory::m_factory.lock();

    if (!factory)
    {
        try
        {
            factory = std::make_shared<MediaPipelineIpcFactory>();
        }
        catch (const std::exception &e)
        {
            RIALTO_CLIENT_LOG_ERROR("Failed to create the media player ipc factory, reason: %s", e.what());
        }

        MediaPipelineIpcFactory::m_factory = factory;
    }

    return factory;
}

std::unique_ptr<IMediaPipelineIpc>
MediaPipelineIpcFactory::createMediaPipelineIpc(IMediaPipelineIpcClient *client,
                                                const VideoRequirements &videoRequirements)
{
    std::unique_ptr<IMediaPipelineIpc> mediaPipelineIpc;
    try
    {
        mediaPipelineIpc =
            std::make_unique<MediaPipelineIpc>(client, videoRequirements, IIpcClientFactory::createFactory(),
                                               firebolt::rialto::common::IEventThreadFactory::createFactory());
    }
    catch (const std::exception &e)
    {
        RIALTO_CLIENT_LOG_ERROR("Failed to create the media player ipc, reason: %s", e.what());
    }

    return mediaPipelineIpc;
}

MediaPipelineIpc::MediaPipelineIpc(IMediaPipelineIpcClient *client, const VideoRequirements &videoRequirements,
                                   const std::shared_ptr<IIpcClientFactory> &ipcClientFactory,
                                   const std::shared_ptr<common::IEventThreadFactory> &eventThreadFactory)
    : IpcModule(ipcClientFactory), m_mediaPipelineIpcClient(client),
      m_eventThread(eventThreadFactory->createEventThread("rialto-media-player-events"))
{
    if (!attachChannel())
    {
        throw std::runtime_error("Failed attach to the ipc channel");
    }

    // create media player session
    if (!createSession(videoRequirements))
    {
        throw std::runtime_error("Could not create the media player session");
    }
}

MediaPipelineIpc::~MediaPipelineIpc()
{
    // destroy media player session
    destroySession();

    // detach the Ipc channel
    detachChannel();

    // destroy the thread processing async notifications
    m_eventThread.reset();
}

bool MediaPipelineIpc::createRpcStubs()
{
    m_mediaPipelineStub = std::make_unique<::firebolt::rialto::MediaPipelineModule_Stub>(m_ipcChannel.get());
    if (!m_mediaPipelineStub)
    {
        return false;
    }
    return true;
}

bool MediaPipelineIpc::subscribeToEvents()
{
    if (!m_ipcChannel)
    {
        return false;
    }

    int eventTag = m_ipcChannel->subscribe<firebolt::rialto::PlaybackStateChangeEvent>(
        [this](const std::shared_ptr<firebolt::rialto::PlaybackStateChangeEvent> &event)
        { m_eventThread->add(&MediaPipelineIpc::onPlaybackStateUpdated, this, event); });
    if (eventTag < 0)
        return false;
    m_eventTags.push_back(eventTag);

    eventTag = m_ipcChannel->subscribe<firebolt::rialto::PositionChangeEvent>(
        [this](const std::shared_ptr<firebolt::rialto::PositionChangeEvent> &event)
        { m_eventThread->add(&MediaPipelineIpc::onPositionUpdated, this, event); });
    if (eventTag < 0)
        return false;
    m_eventTags.push_back(eventTag);

    eventTag = m_ipcChannel->subscribe<firebolt::rialto::NetworkStateChangeEvent>(
        [this](const std::shared_ptr<firebolt::rialto::NetworkStateChangeEvent> &event)
        { m_eventThread->add(&MediaPipelineIpc::onNetworkStateUpdated, this, event); });
    if (eventTag < 0)
        return false;
    m_eventTags.push_back(eventTag);

    eventTag = m_ipcChannel->subscribe<firebolt::rialto::NeedMediaDataEvent>(
        [this](const std::shared_ptr<firebolt::rialto::NeedMediaDataEvent> &event)
        { m_eventThread->add(&MediaPipelineIpc::onNeedMediaData, this, event); });
    if (eventTag < 0)
        return false;
    m_eventTags.push_back(eventTag);

    eventTag = m_ipcChannel->subscribe<firebolt::rialto::QosEvent>(
        [this](const std::shared_ptr<firebolt::rialto::QosEvent> &event)
        { m_eventThread->add(&MediaPipelineIpc::onQos, this, event); });
    if (eventTag < 0)
        return false;
    m_eventTags.push_back(eventTag);

    return true;
}

bool MediaPipelineIpc::load(MediaType type, const std::string &mimeType, const std::string &url)
{
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return false;
    }

    firebolt::rialto::LoadRequest request;

    request.set_session_id(m_sessionId);
    request.set_type(convertLoadRequestMediaType(type));
    request.set_mime_type(mimeType);
    request.set_url(url);

    firebolt::rialto::LoadResponse response;
    auto ipcController = m_ipc->createRpcController();
    auto blockingClosure = m_ipc->createBlockingClosure();
    m_mediaPipelineStub->load(ipcController.get(), &request, &response, blockingClosure.get());

    // wait for the call to complete
    blockingClosure->wait();

    // check the result
    if (ipcController->Failed())
    {
        RIALTO_CLIENT_LOG_ERROR("failed to load media due to '%s'", ipcController->ErrorText().c_str());
        return false;
    }

    return true;
}

bool MediaPipelineIpc::attachSource(const std::unique_ptr<IMediaPipeline::MediaSource> &source, int32_t &sourceId)
{
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return false;
    }

    firebolt::rialto::AttachSourceRequest request;

    request.set_session_id(m_sessionId);
    SourceConfigType configType = source->getConfigType();
    request.set_config_type(convertConfigType(configType));
    request.set_mime_type(source->getMimeType());
    request.set_segment_alignment(convertSegmentAlignment(source->getSegmentAlignment()));

    if (source->getCodecData())
    {
        request.set_codec_data(source->getCodecData()->data(), source->getCodecData()->size());
    }
    request.set_stream_format(convertStreamFormat(source->getStreamFormat()));

    if (configType == SourceConfigType::VIDEO_DOLBY_VISION)
    {
        IMediaPipeline::MediaSourceVideoDolbyVision &mediaSourceDolby =
            dynamic_cast<IMediaPipeline::MediaSourceVideoDolbyVision &>(*source);

        request.set_dolby_vision_profile(mediaSourceDolby.getDolbyVisionProfile());
    }
    else if (configType == SourceConfigType::AUDIO)
    {
        IMediaPipeline::MediaSourceAudio &mediaSourceAudio = dynamic_cast<IMediaPipeline::MediaSourceAudio &>(*source);
        request.mutable_audio_config()->set_number_of_channels(mediaSourceAudio.getAudioConfig().numberOfChannels);
        request.mutable_audio_config()->set_sample_rate(mediaSourceAudio.getAudioConfig().sampleRate);
        if (!mediaSourceAudio.getAudioConfig().codecSpecificConfig.empty())
        {
            request.mutable_audio_config()
                ->set_codec_specific_config(mediaSourceAudio.getAudioConfig().codecSpecificConfig.data(),
                                            mediaSourceAudio.getAudioConfig().codecSpecificConfig.size());
        }
    }

    firebolt::rialto::AttachSourceResponse response;
    auto ipcController = m_ipc->createRpcController();
    auto blockingClosure = m_ipc->createBlockingClosure();
    m_mediaPipelineStub->attachSource(ipcController.get(), &request, &response, blockingClosure.get());

    // wait for the call to complete
    blockingClosure->wait();

    // check the result
    if (ipcController->Failed())
    {
        RIALTO_CLIENT_LOG_ERROR("failed to attach source due to '%s'", ipcController->ErrorText().c_str());
        return false;
    }

    sourceId = response.source_id();

    return true;
}

bool MediaPipelineIpc::removeSource(int32_t sourceId)
{
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return false;
    }

    firebolt::rialto::RemoveSourceRequest request;

    request.set_session_id(m_sessionId);
    request.set_source_id(sourceId);

    firebolt::rialto::RemoveSourceResponse response;
    auto ipcController = m_ipc->createRpcController();
    auto blockingClosure = m_ipc->createBlockingClosure();
    m_mediaPipelineStub->removeSource(ipcController.get(), &request, &response, blockingClosure.get());

    // wait for the call to complete
    blockingClosure->wait();

    // check the result
    if (ipcController->Failed())
    {
        RIALTO_CLIENT_LOG_ERROR("failed to remove source due to '%s'", ipcController->ErrorText().c_str());
        return false;
    }

    return true;
}

bool MediaPipelineIpc::setVideoWindow(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return false;
    }

    firebolt::rialto::SetVideoWindowRequest request;

    request.set_session_id(m_sessionId);
    request.set_x(x);
    request.set_y(y);
    request.set_width(width);
    request.set_height(height);

    firebolt::rialto::SetVideoWindowResponse response;
    auto ipcController = m_ipc->createRpcController();
    auto blockingClosure = m_ipc->createBlockingClosure();
    m_mediaPipelineStub->setVideoWindow(ipcController.get(), &request, &response, blockingClosure.get());

    // wait for the call to complete
    blockingClosure->wait();

    // check the result
    if (ipcController->Failed())
    {
        RIALTO_CLIENT_LOG_ERROR("failed to set the video window due to '%s'", ipcController->ErrorText().c_str());
        return false;
    }

    return true;
}

bool MediaPipelineIpc::play()
{
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return false;
    }

    firebolt::rialto::PlayRequest request;

    request.set_session_id(m_sessionId);

    firebolt::rialto::PlayResponse response;
    auto ipcController = m_ipc->createRpcController();
    auto blockingClosure = m_ipc->createBlockingClosure();
    m_mediaPipelineStub->play(ipcController.get(), &request, &response, blockingClosure.get());

    // wait for the call to complete
    blockingClosure->wait();

    // check the result
    if (ipcController->Failed())
    {
        RIALTO_CLIENT_LOG_ERROR("failed to play due to '%s'", ipcController->ErrorText().c_str());
        return false;
    }

    return true;
}

bool MediaPipelineIpc::pause()
{
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return false;
    }

    firebolt::rialto::PauseRequest request;

    request.set_session_id(m_sessionId);

    firebolt::rialto::PauseResponse response;
    auto ipcController = m_ipc->createRpcController();
    auto blockingClosure = m_ipc->createBlockingClosure();
    m_mediaPipelineStub->pause(ipcController.get(), &request, &response, blockingClosure.get());

    // wait for the call to complete
    blockingClosure->wait();

    // check the result
    if (ipcController->Failed())
    {
        RIALTO_CLIENT_LOG_ERROR("failed to pause due to '%s'", ipcController->ErrorText().c_str());
        return false;
    }

    return true;
}

bool MediaPipelineIpc::stop()
{
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return false;
    }

    firebolt::rialto::StopRequest request;

    request.set_session_id(m_sessionId);

    firebolt::rialto::StopResponse response;
    auto ipcController = m_ipc->createRpcController();
    auto blockingClosure = m_ipc->createBlockingClosure();
    m_mediaPipelineStub->stop(ipcController.get(), &request, &response, blockingClosure.get());

    // wait for the call to complete
    blockingClosure->wait();

    // check the result
    if (ipcController->Failed())
    {
        RIALTO_CLIENT_LOG_ERROR("failed to stop due to '%s'", ipcController->ErrorText().c_str());
        return false;
    }

    return true;
}

bool MediaPipelineIpc::haveData(MediaSourceStatus status, uint32_t numFrames, uint32_t requestId)
{
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return false;
    }

    firebolt::rialto::HaveDataRequest request;

    request.set_session_id(m_sessionId);
    request.set_status(convertHaveDataRequestMediaSourceStatus(status));
    request.set_num_frames(numFrames);
    request.set_request_id(requestId);

    firebolt::rialto::HaveDataResponse response;
    auto ipcController = m_ipc->createRpcController();
    auto blockingClosure = m_ipc->createBlockingClosure();
    m_mediaPipelineStub->haveData(ipcController.get(), &request, &response, blockingClosure.get());

    // wait for the call to complete
    blockingClosure->wait();

    // check the result
    if (ipcController->Failed())
    {
        RIALTO_CLIENT_LOG_ERROR("failed to stop due to '%s'", ipcController->ErrorText().c_str());
        return false;
    }

    return true;
}

bool MediaPipelineIpc::setPosition(int64_t position)
{
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return false;
    }

    firebolt::rialto::SetPositionRequest request;

    request.set_session_id(m_sessionId);
    request.set_position(position);

    firebolt::rialto::SetPositionResponse response;
    auto ipcController = m_ipc->createRpcController();
    auto blockingClosure = m_ipc->createBlockingClosure();
    m_mediaPipelineStub->setPosition(ipcController.get(), &request, &response, blockingClosure.get());

    // wait for the call to complete
    blockingClosure->wait();

    // check the result
    if (ipcController->Failed())
    {
        RIALTO_CLIENT_LOG_ERROR("failed to set position due to '%s'", ipcController->ErrorText().c_str());
        return false;
    }

    return true;
}

bool MediaPipelineIpc::getPosition(int64_t &position)
{
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return false;
    }

    firebolt::rialto::GetPositionRequest request;

    request.set_session_id(m_sessionId);

    firebolt::rialto::GetPositionResponse response;
    auto ipcController = m_ipc->createRpcController();
    auto blockingClosure = m_ipc->createBlockingClosure();
    m_mediaPipelineStub->getPosition(ipcController.get(), &request, &response, blockingClosure.get());

    // wait for the call to complete
    blockingClosure->wait();

    // check the result
    if (ipcController->Failed())
    {
        RIALTO_CLIENT_LOG_ERROR("failed to set position due to '%s'", ipcController->ErrorText().c_str());
        return false;
    }

    position = response.position();
    return true;
}

bool MediaPipelineIpc::setPlaybackRate(double rate)
{
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return false;
    }

    firebolt::rialto::SetPlaybackRateRequest request;

    request.set_session_id(m_sessionId);
    request.set_rate(rate);

    firebolt::rialto::SetPlaybackRateResponse response;
    auto ipcController = m_ipc->createRpcController();
    auto blockingClosure = m_ipc->createBlockingClosure();
    m_mediaPipelineStub->setPlaybackRate(ipcController.get(), &request, &response, blockingClosure.get());

    // wait for the call to complete
    blockingClosure->wait();

    // check the result
    if (ipcController->Failed())
    {
        RIALTO_CLIENT_LOG_ERROR("failed to set playback rate due to '%s'", ipcController->ErrorText().c_str());
        return false;
    }

    return true;
}

bool MediaPipelineIpc::renderFrame()
{
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return false;
    }

    firebolt::rialto::RenderFrameRequest request;
    request.set_session_id(m_sessionId);

    firebolt::rialto::RenderFrameResponse response;
    auto ipcController = m_ipc->createRpcController();
    auto blockingClosure = m_ipc->createBlockingClosure();
    m_mediaPipelineStub->renderFrame(ipcController.get(), &request, &response, blockingClosure.get());

    // wait for the call to complete
    blockingClosure->wait();

    // check the result
    if (ipcController->Failed())
    {
        RIALTO_CLIENT_LOG_ERROR("failed to render frame due to '%s'", ipcController->ErrorText().c_str());
        return false;
    }

    return true;
}

bool MediaPipelineIpc::setVolume(double volume)
{
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return false;
    }

    firebolt::rialto::SetVolumeRequest request;

    request.set_session_id(m_sessionId);
    request.set_volume(volume);

    firebolt::rialto::SetVolumeResponse response;
    auto ipcController = m_ipc->createRpcController();
    auto blockingClosure = m_ipc->createBlockingClosure();
    m_mediaPipelineStub->setVolume(ipcController.get(), &request, &response, blockingClosure.get());

    // wait for the call to complete
    blockingClosure->wait();

    // check the result
    if (ipcController->Failed())
    {
        RIALTO_CLIENT_LOG_ERROR("failed to set volume due to '%s'", ipcController->ErrorText().c_str());
        return false;
    }

    return true;
}

bool MediaPipelineIpc::getVolume(double &volume)
{
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return false;
    }

    firebolt::rialto::GetVolumeRequest request;

    request.set_session_id(m_sessionId);

    firebolt::rialto::GetVolumeResponse response;
    auto ipcController = m_ipc->createRpcController();
    auto blockingClosure = m_ipc->createBlockingClosure();
    m_mediaPipelineStub->getVolume(ipcController.get(), &request, &response, blockingClosure.get());

    // wait for the call to complete
    blockingClosure->wait();

    // check the result
    if (ipcController->Failed())
    {
        RIALTO_CLIENT_LOG_ERROR("failed to get volume due to '%s'", ipcController->ErrorText().c_str());
        return false;
    }
    volume = response.volume();

    return true;
}

void MediaPipelineIpc::onPlaybackStateUpdated(const std::shared_ptr<firebolt::rialto::PlaybackStateChangeEvent> &event)
{
    /* Ignore event if not for this session */
    if (event->session_id() == m_sessionId)
    {
        PlaybackState playbackState = PlaybackState::UNKNOWN;
        switch (event->state())
        {
        case firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_IDLE:
            playbackState = PlaybackState::IDLE;
            break;
        case firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_PLAYING:
            playbackState = PlaybackState::PLAYING;
            break;
        case firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_PAUSED:
            playbackState = PlaybackState::PAUSED;
            break;
        case firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_SEEKING:
            playbackState = PlaybackState::SEEKING;
            break;
        case firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_FLUSHED:
            playbackState = PlaybackState::FLUSHED;
            break;
        case firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_STOPPED:
            playbackState = PlaybackState::STOPPED;
            break;
        case firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_END_OF_STREAM:
            playbackState = PlaybackState::END_OF_STREAM;
            break;
        case firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_FAILURE:
            playbackState = PlaybackState::FAILURE;
            break;
        default:
            RIALTO_CLIENT_LOG_WARN("Recieved unknown playback state");
            break;
        }

        m_mediaPipelineIpcClient->notifyPlaybackState(playbackState);
    }
}

void MediaPipelineIpc::onPositionUpdated(const std::shared_ptr<firebolt::rialto::PositionChangeEvent> &event)
{
    // Ignore event if not for this session
    if (event->session_id() == m_sessionId)
    {
        int64_t position = event->position();
        m_mediaPipelineIpcClient->notifyPosition(position);
    }
}

void MediaPipelineIpc::onNetworkStateUpdated(const std::shared_ptr<firebolt::rialto::NetworkStateChangeEvent> &event)
{
    // Ignore event if not for this session
    if (event->session_id() == m_sessionId)
    {
        NetworkState networkState = NetworkState::UNKNOWN;
        switch (event->state())
        {
        case firebolt::rialto::NetworkStateChangeEvent_NetworkState_IDLE:
            networkState = NetworkState::IDLE;
            break;
        case firebolt::rialto::NetworkStateChangeEvent_NetworkState_BUFFERING:
            networkState = NetworkState::BUFFERING;
            break;
        case firebolt::rialto::NetworkStateChangeEvent_NetworkState_BUFFERING_PROGRESS:
            networkState = NetworkState::BUFFERING_PROGRESS;
            break;
        case firebolt::rialto::NetworkStateChangeEvent_NetworkState_BUFFERED:
            networkState = NetworkState::BUFFERED;
            break;
        case firebolt::rialto::NetworkStateChangeEvent_NetworkState_STALLED:
            networkState = NetworkState::STALLED;
            break;
        case firebolt::rialto::NetworkStateChangeEvent_NetworkState_FORMAT_ERROR:
            networkState = NetworkState::FORMAT_ERROR;
            break;
        case firebolt::rialto::NetworkStateChangeEvent_NetworkState_NETWORK_ERROR:
            networkState = NetworkState::NETWORK_ERROR;
            break;
        default:
            RIALTO_CLIENT_LOG_WARN("Recieved unknown network state");
            break;
        }

        m_mediaPipelineIpcClient->notifyNetworkState(networkState);
    }
}

void MediaPipelineIpc::onNeedMediaData(const std::shared_ptr<firebolt::rialto::NeedMediaDataEvent> &event)
{
    // Ignore event if not for this session
    if (event->session_id() == m_sessionId)
    {
        std::shared_ptr<MediaPlayerShmInfo> shmInfo;
        if (event->has_shm_info())
        {
            shmInfo = std::make_shared<MediaPlayerShmInfo>();
            shmInfo->maxMetadataBytes = event->shm_info().max_metadata_bytes();
            shmInfo->metadataOffset = event->shm_info().metadata_offset();
            shmInfo->mediaDataOffset = event->shm_info().media_data_offset();
            shmInfo->maxMediaBytes = event->shm_info().max_media_bytes();
        }
        m_mediaPipelineIpcClient->notifyNeedMediaData(event->source_id(), event->frame_count(), event->request_id(),
                                                      shmInfo);
    }
}

void MediaPipelineIpc::onQos(const std::shared_ptr<firebolt::rialto::QosEvent> &event)
{
    // Ignore event if not for this session
    if (event->session_id() == m_sessionId)
    {
        QosInfo qosInfo = {event->qos_info().processed(), event->qos_info().dropped()};
        m_mediaPipelineIpcClient->notifyQos(event->source_id(), qosInfo);
    }
}

bool MediaPipelineIpc::createSession(const VideoRequirements &videoRequirements)
{
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return false;
    }

    firebolt::rialto::CreateSessionRequest request;

    request.set_max_width(videoRequirements.maxWidth);
    request.set_max_height(videoRequirements.maxHeight);

    firebolt::rialto::CreateSessionResponse response;
    auto ipcController = m_ipc->createRpcController();
    auto blockingClosure = m_ipc->createBlockingClosure();
    m_mediaPipelineStub->createSession(ipcController.get(), &request, &response, blockingClosure.get());

    // wait for the call to complete
    blockingClosure->wait();

    // check the result
    if (ipcController->Failed())
    {
        RIALTO_CLIENT_LOG_ERROR("failed to create session due to '%s'", ipcController->ErrorText().c_str());
        return false;
    }

    m_sessionId = response.session_id();

    return true;
}

void MediaPipelineIpc::destroySession()
{
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return;
    }

    firebolt::rialto::DestroySessionRequest request;
    request.set_session_id(m_sessionId);

    firebolt::rialto::DestroySessionResponse response;
    auto ipcController = m_ipc->createRpcController();
    auto blockingClosure = m_ipc->createBlockingClosure();
    m_mediaPipelineStub->destroySession(ipcController.get(), &request, &response, blockingClosure.get());

    // wait for the call to complete
    blockingClosure->wait();

    // check the result
    if (ipcController->Failed())
    {
        RIALTO_CLIENT_LOG_ERROR("failed to destroy session due to '%s'", ipcController->ErrorText().c_str());
    }
}

firebolt::rialto::LoadRequest_MediaType MediaPipelineIpc::convertLoadRequestMediaType(MediaType mediaType)
{
    firebolt::rialto::LoadRequest_MediaType protoMediaType = firebolt::rialto::LoadRequest_MediaType_UNKNOWN;
    switch (mediaType)
    {
    case MediaType::MSE:
        protoMediaType = firebolt::rialto::LoadRequest_MediaType_MSE;
        break;
    default:
        break;
    }

    return protoMediaType;
}

firebolt::rialto::HaveDataRequest_MediaSourceStatus
MediaPipelineIpc::convertHaveDataRequestMediaSourceStatus(MediaSourceStatus status)
{
    firebolt::rialto::HaveDataRequest_MediaSourceStatus protoMediaSourceStatus =
        firebolt::rialto::HaveDataRequest_MediaSourceStatus_UNKNOWN;
    switch (status)
    {
    case MediaSourceStatus::OK:
        protoMediaSourceStatus = firebolt::rialto::HaveDataRequest_MediaSourceStatus_OK;
        break;
    case MediaSourceStatus::EOS:
        protoMediaSourceStatus = firebolt::rialto::HaveDataRequest_MediaSourceStatus_EOS;
        break;
    case MediaSourceStatus::ERROR:
        protoMediaSourceStatus = firebolt::rialto::HaveDataRequest_MediaSourceStatus_ERROR;
        break;
    case MediaSourceStatus::CODEC_CHANGED:
        protoMediaSourceStatus = firebolt::rialto::HaveDataRequest_MediaSourceStatus_CODEC_CHANGED;
        break;
    case MediaSourceStatus::NO_AVAILABLE_SAMPLES:
        protoMediaSourceStatus = firebolt::rialto::HaveDataRequest_MediaSourceStatus_NO_AVAILABLE_SAMPLES;
        break;
    default:
        break;
    }

    return protoMediaSourceStatus;
}

firebolt::rialto::AttachSourceRequest_ConfigType
MediaPipelineIpc::convertConfigType(const firebolt::rialto::SourceConfigType &configType)
{
    switch (configType)
    {
    case firebolt::rialto::SourceConfigType::UNKNOWN:
    {
        return firebolt::rialto::AttachSourceRequest_ConfigType_CONFIG_TYPE_UNKNOWN;
    }
    case firebolt::rialto::SourceConfigType::AUDIO:
    {
        return firebolt::rialto::AttachSourceRequest_ConfigType_CONFIG_TYPE_AUDIO;
    }
    case firebolt::rialto::SourceConfigType::VIDEO:
    {
        return firebolt::rialto::AttachSourceRequest_ConfigType_CONFIG_TYPE_VIDEO;
    }
    case firebolt::rialto::SourceConfigType::VIDEO_DOLBY_VISION:
    {
        return firebolt::rialto::AttachSourceRequest_ConfigType_CONFIG_TYPE_VIDEO_DOLBY_VISION;
    }
    }
    return firebolt::rialto::AttachSourceRequest_ConfigType_CONFIG_TYPE_UNKNOWN;
}

firebolt::rialto::AttachSourceRequest_SegmentAlignment
MediaPipelineIpc::convertSegmentAlignment(const firebolt::rialto::SegmentAlignment &alignment)
{
    switch (alignment)
    {
    case firebolt::rialto::SegmentAlignment::UNDEFINED:
    {
        return firebolt::rialto::AttachSourceRequest_SegmentAlignment_ALIGNMENT_UNDEFINED;
    }
    case firebolt::rialto::SegmentAlignment::NAL:
    {
        return firebolt::rialto::AttachSourceRequest_SegmentAlignment_ALIGNMENT_NAL;
    }
    case firebolt::rialto::SegmentAlignment::AU:
    {
        return firebolt::rialto::AttachSourceRequest_SegmentAlignment_ALIGNMENT_AU;
    }
    }
    return firebolt::rialto::AttachSourceRequest_SegmentAlignment_ALIGNMENT_UNDEFINED;
}

firebolt::rialto::AttachSourceRequest_StreamFormat
MediaPipelineIpc::convertStreamFormat(const firebolt::rialto::StreamFormat &streamFormat)
{
    switch (streamFormat)
    {
    case firebolt::rialto::StreamFormat::UNDEFINED:
    {
        return firebolt::rialto::AttachSourceRequest_StreamFormat_STREAM_FORMAT_UNDEFINED;
    }
    case firebolt::rialto::StreamFormat::RAW:
    {
        return firebolt::rialto::AttachSourceRequest_StreamFormat_STREAM_FORMAT_RAW;
    }
    case firebolt::rialto::StreamFormat::AVC:
    {
        return firebolt::rialto::AttachSourceRequest_StreamFormat_STREAM_FORMAT_AVC;
    }
    case firebolt::rialto::StreamFormat::BYTE_STREAM:
    {
        return firebolt::rialto::AttachSourceRequest_StreamFormat_STREAM_FORMAT_BYTE_STREAM;
    }
    }
    return firebolt::rialto::AttachSourceRequest_StreamFormat_STREAM_FORMAT_UNDEFINED;
}

}; // namespace firebolt::rialto::client
