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
#include "WebAudioPlayerIpc.h"
#include "IIpcClient.h"
#include "RialtoClientLogging.h"
#include "RialtoCommonIpc.h"
#include "webaudioplayermodule.pb.h"
#include <IWebAudioPlayer.h>

namespace firebolt::rialto::client
{

std::shared_ptr<IWebAudioPlayerIpcFactory> IWebAudioPlayerIpcFactory::getFactory()
{
    std::shared_ptr<IWebAudioPlayerIpcFactory> factory;

    try
    {
        factory = std::make_shared<WebAudioPlayerIpcFactory>();
    }
    catch (const std::exception &e)
    {
        RIALTO_CLIENT_LOG_ERROR("Failed to create the web audio player ipc factory, reason: %s", e.what());
    }

    return factory;
}

std::unique_ptr<IWebAudioPlayerIpc> WebAudioPlayerIpcFactory::createWebAudioPlayerIpc(IWebAudioPlayerIpcClient *client,
                                                                                      const std::string &audioMimeType,
                                                                                      const uint32_t priority,
                                                                                      const WebAudioConfig *config)
{
    std::unique_ptr<IWebAudioPlayerIpc> webAudioPlayerIpc;
    try
    {
        webAudioPlayerIpc =
            std::make_unique<WebAudioPlayerIpc>(client, audioMimeType, priority, config,
                                                IIpcClientFactory::createFactory(),
                                                firebolt::rialto::common::IEventThreadFactory::createFactory());
    }
    catch (const std::exception &e)
    {
        RIALTO_CLIENT_LOG_ERROR("Failed to create the web audio player ipc, reason: %s", e.what());
    }

    return webAudioPlayerIpc;
}

WebAudioPlayerIpc::WebAudioPlayerIpc(IWebAudioPlayerIpcClient *client, const std::string &audioMimeType,
                                     const uint32_t priority, const WebAudioConfig *config,
                                     const std::shared_ptr<IIpcClientFactory> &ipcClientFactory,
                                     const std::shared_ptr<common::IEventThreadFactory> &eventThreadFactory)
    : IpcModule(ipcClientFactory), m_WebAudioPlayerIpcClient(client),
      m_eventThread(eventThreadFactory->createEventThread("rialto-web-audio-player-events")), m_webAudioPlayerHandle(-1)
{
    if (!attachChannel())
    {
        throw std::runtime_error("Failed attach to the ipc channel");
    }

    if (!createWebAudioPlayer(audioMimeType, priority, config))
    {
        throw std::runtime_error("Could not create the web audio playersession");
    }
}

WebAudioPlayerIpc::~WebAudioPlayerIpc()
{
    // destroy web audio player session
    destroyWebAudioPlayer();

    // detach the Ipc channel
    detachChannel();

    // destroy the thread processing async notifications
    m_eventThread.reset();
}

bool WebAudioPlayerIpc::createRpcStubs()
{
    m_WebAudioPlayerStub = std::make_unique<::firebolt::rialto::WebAudioPlayerModule_Stub>(m_ipcChannel.get());
    if (!m_WebAudioPlayerStub)
    {
        return false;
    }
    return true;
}

bool WebAudioPlayerIpc::subscribeToEvents()
{
    if (!m_ipcChannel)
    {
        return false;
    }

    int eventTag = m_ipcChannel->subscribe<firebolt::rialto::WebAudioPlayerStateEvent>(
        [this](const std::shared_ptr<firebolt::rialto::WebAudioPlayerStateEvent> &event)
        { m_eventThread->add(&WebAudioPlayerIpc::onPlaybackStateUpdated, this, event); });
    if (eventTag < 0)
        return false;
    m_eventTags.push_back(eventTag);

    return true;
}

void WebAudioPlayerIpc::onPlaybackStateUpdated(const std::shared_ptr<firebolt::rialto::WebAudioPlayerStateEvent> &event)
{
    if (event->web_audio_player_handle() == m_webAudioPlayerHandle)
    {
        firebolt::rialto::WebAudioPlayerState playerState = firebolt::rialto::WebAudioPlayerState::UNKNOWN;
        switch (event->state())
        {
        case firebolt::rialto::WebAudioPlayerStateEvent_WebAudioPlayerState_UNKNOWN:
            playerState = firebolt::rialto::WebAudioPlayerState::UNKNOWN;
            break;
        case firebolt::rialto::WebAudioPlayerStateEvent_WebAudioPlayerState_IDLE:
            playerState = firebolt::rialto::WebAudioPlayerState::IDLE;
            break;
        case firebolt::rialto::WebAudioPlayerStateEvent_WebAudioPlayerState_PLAYING:
            playerState = firebolt::rialto::WebAudioPlayerState::PLAYING;
            break;
        case firebolt::rialto::WebAudioPlayerStateEvent_WebAudioPlayerState_PAUSED:
            playerState = firebolt::rialto::WebAudioPlayerState::PAUSED;
            break;
        case firebolt::rialto::WebAudioPlayerStateEvent_WebAudioPlayerState_END_OF_STREAM:
            playerState = firebolt::rialto::WebAudioPlayerState::END_OF_STREAM;
            break;
        case firebolt::rialto::WebAudioPlayerStateEvent_WebAudioPlayerState_FAILURE:
            playerState = firebolt::rialto::WebAudioPlayerState::FAILURE;
            break;
        default:
            RIALTO_CLIENT_LOG_WARN("Recieved unknown web audio player state");
            break;
        }

        m_WebAudioPlayerIpcClient->notifyState(playerState);
    }
}

bool WebAudioPlayerIpc::play()
{
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return false;
    }

    firebolt::rialto::WebAudioPlayRequest request;
    request.set_web_audio_player_handle(m_webAudioPlayerHandle);

    firebolt::rialto::WebAudioPlayResponse response;
    auto ipcController = m_ipc->createRpcController();
    auto blockingClosure = m_ipc->createBlockingClosure();
    m_WebAudioPlayerStub->play(ipcController.get(), &request, &response, blockingClosure.get());

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

bool WebAudioPlayerIpc::pause()
{
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return false;
    }

    firebolt::rialto::WebAudioPauseRequest request;
    request.set_web_audio_player_handle(m_webAudioPlayerHandle);

    firebolt::rialto::WebAudioPauseResponse response;
    auto ipcController = m_ipc->createRpcController();
    auto blockingClosure = m_ipc->createBlockingClosure();
    m_WebAudioPlayerStub->pause(ipcController.get(), &request, &response, blockingClosure.get());

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

bool WebAudioPlayerIpc::setEos()
{
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return false;
    }

    firebolt::rialto::WebAudioSetEosRequest request;
    request.set_web_audio_player_handle(m_webAudioPlayerHandle);

    firebolt::rialto::WebAudioSetEosResponse response;
    auto ipcController = m_ipc->createRpcController();
    auto blockingClosure = m_ipc->createBlockingClosure();
    m_WebAudioPlayerStub->setEos(ipcController.get(), &request, &response, blockingClosure.get());

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

bool WebAudioPlayerIpc::getBufferAvailable(uint32_t &availableFrames,
                                           const std::shared_ptr<WebAudioShmInfo> &webAudioShmInfo)
{

    if (!webAudioShmInfo)
    {
        RIALTO_CLIENT_LOG_ERROR("webAudioShmInfo parameter can't be null!");
        return false;
    }

    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return false;
    }

    firebolt::rialto::WebAudioGetBufferAvailableRequest request;
    request.set_web_audio_player_handle(m_webAudioPlayerHandle);

    firebolt::rialto::WebAudioGetBufferAvailableResponse response;
    auto ipcController = m_ipc->createRpcController();
    auto blockingClosure = m_ipc->createBlockingClosure();
    m_WebAudioPlayerStub->getBufferAvailable(ipcController.get(), &request, &response, blockingClosure.get());

    // wait for the call to complete
    blockingClosure->wait();

    // check the result
    if (ipcController->Failed())
    {
        RIALTO_CLIENT_LOG_ERROR("failed to attach source due to '%s'", ipcController->ErrorText().c_str());
        return false;
    }

    availableFrames = response.available_frames();
    webAudioShmInfo->offsetMain = response.shm_info().offset_main();
    webAudioShmInfo->lengthMain = response.shm_info().length_main();
    webAudioShmInfo->offsetWrap = response.shm_info().offset_wrap();
    webAudioShmInfo->lengthWrap = response.shm_info().length_wrap();

    return true;
}

bool WebAudioPlayerIpc::getBufferDelay(uint32_t &delayFrames)
{
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return false;
    }

    firebolt::rialto::WebAudioGetBufferDelayRequest request;
    request.set_web_audio_player_handle(m_webAudioPlayerHandle);

    firebolt::rialto::WebAudioGetBufferDelayResponse response;
    auto ipcController = m_ipc->createRpcController();
    auto blockingClosure = m_ipc->createBlockingClosure();
    m_WebAudioPlayerStub->getBufferDelay(ipcController.get(), &request, &response, blockingClosure.get());

    // wait for the call to complete
    blockingClosure->wait();

    // check the result
    if (ipcController->Failed())
    {
        RIALTO_CLIENT_LOG_ERROR("failed to attach source due to '%s'", ipcController->ErrorText().c_str());
        return false;
    }

    delayFrames = response.delay_frames();

    return true;
}

bool WebAudioPlayerIpc::writeBuffer(const uint32_t numberOfFrames)
{
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return false;
    }

    firebolt::rialto::WebAudioWriteBufferRequest request;

    request.set_number_of_frames(numberOfFrames);
    request.set_web_audio_player_handle(m_webAudioPlayerHandle);

    firebolt::rialto::WebAudioWriteBufferResponse response;
    auto ipcController = m_ipc->createRpcController();
    auto blockingClosure = m_ipc->createBlockingClosure();
    m_WebAudioPlayerStub->writeBuffer(ipcController.get(), &request, &response, blockingClosure.get());

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

bool WebAudioPlayerIpc::getDeviceInfo(uint32_t &preferredFrames, uint32_t &maximumFrames, bool &supportDeferredPlay)
{
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return false;
    }

    firebolt::rialto::WebAudioGetDeviceInfoRequest request;
    request.set_web_audio_player_handle(m_webAudioPlayerHandle);

    firebolt::rialto::WebAudioGetDeviceInfoResponse response;
    auto ipcController = m_ipc->createRpcController();
    auto blockingClosure = m_ipc->createBlockingClosure();
    m_WebAudioPlayerStub->getDeviceInfo(ipcController.get(), &request, &response, blockingClosure.get());

    // wait for the call to complete
    blockingClosure->wait();

    // check the result
    if (ipcController->Failed())
    {
        RIALTO_CLIENT_LOG_ERROR("failed to attach source due to '%s'", ipcController->ErrorText().c_str());
        return false;
    }

    preferredFrames = response.preferred_frames();
    maximumFrames = response.maximum_frames();
    supportDeferredPlay = response.support_deferred_play();

    return true;
}

bool WebAudioPlayerIpc::setVolume(double volume)
{
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return false;
    }

    firebolt::rialto::WebAudioSetVolumeRequest request;
    request.set_volume(volume);
    request.set_web_audio_player_handle(m_webAudioPlayerHandle);

    firebolt::rialto::WebAudioSetVolumeResponse response;
    auto ipcController = m_ipc->createRpcController();
    auto blockingClosure = m_ipc->createBlockingClosure();
    m_WebAudioPlayerStub->setVolume(ipcController.get(), &request, &response, blockingClosure.get());

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

bool WebAudioPlayerIpc::getVolume(double &volume)
{
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return false;
    }

    firebolt::rialto::WebAudioGetVolumeRequest request;
    request.set_web_audio_player_handle(m_webAudioPlayerHandle);

    firebolt::rialto::WebAudioGetVolumeResponse response;
    auto ipcController = m_ipc->createRpcController();
    auto blockingClosure = m_ipc->createBlockingClosure();
    m_WebAudioPlayerStub->getVolume(ipcController.get(), &request, &response, blockingClosure.get());

    // wait for the call to complete
    blockingClosure->wait();

    // check the result
    if (ipcController->Failed())
    {
        RIALTO_CLIENT_LOG_ERROR("failed to stop due to '%s'", ipcController->ErrorText().c_str());
        return false;
    }

    volume = response.volume();
    return true;
}

bool WebAudioPlayerIpc::createWebAudioPlayer(const std::string &audioMimeType, const uint32_t priority,
                                             const WebAudioConfig *config)
{
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return false;
    }

    firebolt::rialto::CreateWebAudioPlayerRequest request;
    request.set_audio_mime_type(audioMimeType);
    request.set_priority(priority);
    if (config)
    {
        ::firebolt::rialto::CreateWebAudioPlayerRequest_WebAudioPcmConfig pcm_config;
        pcm_config.set_rate(config->pcm.rate);
        pcm_config.set_channels(config->pcm.channels);
        pcm_config.set_sample_size(config->pcm.sampleSize);
        pcm_config.set_is_big_endian(config->pcm.isBigEndian);
        pcm_config.set_is_signed(config->pcm.isSigned);
        pcm_config.set_is_float(config->pcm.isFloat);
        request.mutable_config()->mutable_pcm()->CopyFrom(pcm_config);
    }

    firebolt::rialto::CreateWebAudioPlayerResponse response;
    auto ipcController = m_ipc->createRpcController();
    auto blockingClosure = m_ipc->createBlockingClosure();
    m_WebAudioPlayerStub->createWebAudioPlayer(ipcController.get(), &request, &response, blockingClosure.get());

    // wait for the call to complete
    blockingClosure->wait();

    // check the result
    if (ipcController->Failed())
    {
        RIALTO_CLIENT_LOG_ERROR("failed to create session due to '%s'", ipcController->ErrorText().c_str());
        return false;
    }

    m_webAudioPlayerHandle = response.web_audio_player_handle();

    return true;
}

void WebAudioPlayerIpc::destroyWebAudioPlayer()
{
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("Reattachment of the ipc channel failed, ipc disconnected");
        return;
    }

    firebolt::rialto::DestroyWebAudioPlayerRequest request;
    request.set_web_audio_player_handle(m_webAudioPlayerHandle);

    firebolt::rialto::DestroyWebAudioPlayerResponse response;
    auto ipcController = m_ipc->createRpcController();
    auto blockingClosure = m_ipc->createBlockingClosure();
    m_WebAudioPlayerStub->destroyWebAudioPlayer(ipcController.get(), &request, &response, blockingClosure.get());

    // wait for the call to complete
    blockingClosure->wait();

    // check the result
    if (ipcController->Failed())
    {
        RIALTO_CLIENT_LOG_ERROR("failed to destroy session due to '%s'", ipcController->ErrorText().c_str());
    }
}

}; // namespace firebolt::rialto::client
