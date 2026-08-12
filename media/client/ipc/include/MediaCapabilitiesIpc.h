/*
 * Copyright 2026 Sky UK
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef FIREBOLT_RIALTO_CLIENT_MEDIA_CAPABILITIES_IPC_H_
#define FIREBOLT_RIALTO_CLIENT_MEDIA_CAPABILITIES_IPC_H_

#include "IMediaCapabilities.h"
#include "IpcModule.h"
#include "mediapipelinecapabilitiesmodule.pb.h"
#include <memory>

namespace firebolt::rialto::client
{
class MediaCapabilitiesIpc : public IMediaCapabilities, public IpcModule
{
public:
    explicit MediaCapabilitiesIpc(IIpcClient &ipcClient);
    ~MediaCapabilitiesIpc() override;

    AudioDecoderCapabilities getSupportedAudioCapabilities() override;
    VideoDecoderCapabilities getSupportedVideoCapabilities() override;

protected:
    bool createRpcStubs(const std::shared_ptr<ipc::IChannel> &ipcChannel) override;
    bool subscribeToEvents(const std::shared_ptr<ipc::IChannel> &ipcChannel) override { return true; }

private:
    std::unique_ptr<::firebolt::rialto::MediaPipelineCapabilitiesModule_Stub> m_stub;
};

} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_MEDIA_CAPABILITIES_IPC_H_
