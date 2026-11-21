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

#ifndef FIREBOLT_RIALTO_CLIENT_VIDEO_FRAME_CAPTURE_IPC_H_
#define FIREBOLT_RIALTO_CLIENT_VIDEO_FRAME_CAPTURE_IPC_H_

#include "IVideoFrameCaptureIpc.h"
#include "IpcModule.h"
#include "videoframecapturemodule.pb.h"

#include <memory>

namespace firebolt::rialto::client
{
class VideoFrameCaptureIpcFactory : public IVideoFrameCaptureIpcFactory
{
public:
    std::shared_ptr<IVideoFrameCaptureIpc> createVideoFrameCaptureIpc() const override;
};

class VideoFrameCaptureIpc : public IVideoFrameCaptureIpc, public IpcModule
{
public:
    explicit VideoFrameCaptureIpc(IIpcClient &ipcClient);
    ~VideoFrameCaptureIpc() override;

    bool createVideoFrameCapture(GraphicsAPIType api, std::uint32_t &captureId) override;
    bool destroyVideoFrameCapture(std::uint32_t captureId) override;
    CaptureAcquireResult acquireCurrentFrame(std::uint32_t captureId) override;
    bool releaseFrame(std::uint32_t captureId, std::uint32_t poolId, std::uint32_t slotIndex,
                      std::uint64_t acquisitionToken) override;
    bool getPool(std::uint32_t captureId, CapturePoolDescription &pool) override;

private:
    bool createRpcStubs(const std::shared_ptr<ipc::IChannel> &ipcChannel) override;
    bool subscribeToEvents(const std::shared_ptr<ipc::IChannel> &ipcChannel) override { return true; }

    std::unique_ptr<::firebolt::rialto::VideoFrameCaptureModule_Stub> m_videoFrameCaptureStub;
};
} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_VIDEO_FRAME_CAPTURE_IPC_H_
