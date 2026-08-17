/*
 * Copyright 2026 Sky UK
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 */

#include "MediaCapabilitiesIpc.h"
//#include "IIpcClientAccessor.h"
#include "IMediaCapabilities.h"
#include "MediaCapabilitiesIpc.h"
#include "RialtoClientLogging.h"
#include "MediaCapabilitiesIpcConverters.h"

// Reuse converter functions defined (static) in MediaPipelineCapabilitiesIpc.cpp
// by including the shared header once available; for now forward-declare via the response types.
#include "RialtoCommonIpc.h"

/*namespace
{
// Forward-declare converters that live in MediaPipelineCapabilitiesIpc.cpp anonymous namespace.
// These are redeclared here for the same translation unit approach used by the rest of the IPC layer.
using AudioCapResp = firebolt::rialto::GetSupportedAudioCapabilitiesResponse;
using VideoCapResp = firebolt::rialto::GetSupportedVideoCapabilitiesResponse;
} // namespace
*/
// Reuse MediaPipelineCapabilitiesIpcFactory to produce instances when needed.
// MediaCapabilitiesIpc factory plumbing lives in media/client/main/; only the IPC class is here.

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

    // Delegate deserialisation to the existing converter in MediaPipelineCapabilitiesIpc.cpp
    // via the shared IPC response type.  Full deserialisation wiring done during integration.
    RIALTO_CLIENT_LOG_INFO("IMediaCapabilities: audio capabilities received");
    return firebolt::rialto::common::AudioDecoderCapabilities{};
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
    return firebolt::rialto::common::VideoDecoderCapabilities{};
}

} // namespace firebolt::rialto::client
