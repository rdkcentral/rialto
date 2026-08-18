/*
 * Copyright 2026 Sky UK
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 */

#include "MediaCapabilitiesIpc.h"
#include "MediaCapabilitiesIpcConverters.h"
#include "RialtoClientLogging.h"

namespace firebolt::rialto
{
std::shared_ptr<IMediaCapabilitiesFactory> IMediaCapabilitiesFactory::createFactory()
{
    // TODO: factory implementation lives in media/client/main/; left as an integration point.
    return nullptr;
}
} // namespace firebolt::rialto

namespace firebolt::rialto::client
{
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
    m_stub = std::make_unique<::firebolt::rialto::MediaPipelineCapabilitiesModule_Stub>(ipcChannel.get());
    return m_stub != nullptr;
}

firebolt::rialto::common::AudioDecoderCapabilities MediaCapabilitiesIpc::getSupportedAudioCapabilities()
{
    RIALTO_CLIENT_LOG_ERROR("USHA: MediaCapabilitiesIpc: Client: ipc: calling getSupportedAudioCapabilities");
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("IPC channel reattachment failed");
        return firebolt::rialto::common::AudioDecoderCapabilities{};
    }

    firebolt::rialto::GetSupportedAudioCapabilitiesRequest request;
    firebolt::rialto::GetSupportedAudioCapabilitiesResponse response;
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
    return convertAudioDecoderCapabilities(response);
}

firebolt::rialto::common::VideoDecoderCapabilities MediaCapabilitiesIpc::getSupportedVideoCapabilities()
{
    RIALTO_CLIENT_LOG_ERROR("USHA: MediaCapabilitiesIpc: Client: ipc: calling getSupportedVideoCapabilities");
    if (!reattachChannelIfRequired())
    {
        RIALTO_CLIENT_LOG_ERROR("IPC channel reattachment failed");
        return firebolt::rialto::common::VideoDecoderCapabilities{};
    }

    firebolt::rialto::GetSupportedVideoCapabilitiesRequest request;
    firebolt::rialto::GetSupportedVideoCapabilitiesResponse response;
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
    return convertVideoDecoderCapabilities(response);
}

} // namespace firebolt::rialto::client
