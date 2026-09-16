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

#include "MediaCapabilitiesIpc.h"
#include "CapabilityConverters.h"
#include "IIpcClient.h"
#include "RialtoClientLogging.h"

namespace firebolt::rialto::client
{
std::shared_ptr<IMediaCapabilitiesIpcFactory> IMediaCapabilitiesIpcFactory::createFactory()
{
    std::shared_ptr<IMediaCapabilitiesIpcFactory> factory;

    try
    {
        factory = std::make_shared<MediaCapabilitiesIpcFactory>();
    }
    catch (const std::exception &e)
    {
        RIALTO_CLIENT_LOG_ERROR("Failed to create the media capabilities ipc factory, reason: %s", e.what());
    }

    return factory;
}

std::unique_ptr<IMediaCapabilities> MediaCapabilitiesIpcFactory::createMediaCapabilitiesIpc() const
{
    std::unique_ptr<IMediaCapabilities> mediaCapabilitiesIpc;

    try
    {
        mediaCapabilitiesIpc =
            std::make_unique<client::MediaCapabilitiesIpc>(IIpcClientAccessor::instance().getIpcClient());
    }
    catch (const std::exception &e)
    {
        RIALTO_CLIENT_LOG_ERROR("Failed to create the media capabilities ipc, reason: %s", e.what());
    }

    return mediaCapabilitiesIpc;
}

MediaCapabilitiesIpc::MediaCapabilitiesIpc(IIpcClient &ipcClient) : IpcModule(ipcClient)
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");
    if (!attachChannel())
        throw std::runtime_error("Failed to attach to the IPC channel");
}

MediaCapabilitiesIpc::~MediaCapabilitiesIpc()
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");
    detachChannel();
}

bool MediaCapabilitiesIpc::createRpcStubs(const std::shared_ptr<ipc::IChannel> &ipcChannel)
{
    m_stub = std::make_unique<::firebolt::rialto::MediaCapabilitiesModule_Stub>(ipcChannel.get());
    return m_stub != nullptr;
}

firebolt::rialto::common::AudioDecoderCapabilities MediaCapabilitiesIpc::getSupportedAudioCapabilities()
{
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("IPC channel reattachment failed");
        return firebolt::rialto::common::AudioDecoderCapabilities{};
    }

    firebolt::rialto::GetSupportedAudioCapabilitiesRequest request;
    firebolt::rialto::AudioCapabilities response;
    auto ipcController = m_ipc.createRpcController();
    auto blockingClosure = m_ipc.createBlockingClosure();
    m_stub->getSupportedAudioCapabilities(ipcController.get(), &request, &response, blockingClosure.get());
    blockingClosure->wait();

    if (ipcController->Failed())
    {
        RIALTO_CLIENT_LOG_ERROR("IMediaCapabilities::getSupportedAudioCapabilities failed: %s",
                                ipcController->ErrorText().c_str());
        return firebolt::rialto::common::AudioDecoderCapabilities{};
    }

    RIALTO_CLIENT_LOG_INFO("IMediaCapabilities: audio capabilities received");
    return firebolt::rialto::ipc::common::deserialiseAudioCapabilities(response);
}

firebolt::rialto::common::VideoDecoderCapabilities MediaCapabilitiesIpc::getSupportedVideoCapabilities()
{
    RIALTO_CLIENT_LOG_DEBUG("entry:");
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("IPC channel reattachment failed");
        return firebolt::rialto::common::VideoDecoderCapabilities{};
    }

    firebolt::rialto::GetSupportedVideoCapabilitiesRequest request;
    firebolt::rialto::VideoCapabilities response;
    auto ipcController = m_ipc.createRpcController();
    auto blockingClosure = m_ipc.createBlockingClosure();
    m_stub->getSupportedVideoCapabilities(ipcController.get(), &request, &response, blockingClosure.get());
    blockingClosure->wait();

    if (ipcController->Failed())
    {
        RIALTO_CLIENT_LOG_ERROR("IMediaCapabilities::getSupportedVideoCapabilities failed: %s",
                                ipcController->ErrorText().c_str());
        return firebolt::rialto::common::VideoDecoderCapabilities{};
    }

    RIALTO_CLIENT_LOG_INFO("IMediaCapabilities: video capabilities received");
    return firebolt::rialto::ipc::common::deserialiseVideoCapabilities(response);
}

} // namespace firebolt::rialto::client
