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

#ifndef FIREBOLT_RIALTO_CT_STUB_MEDIA_PIPELINE_MODULE_STUB_H_
#define FIREBOLT_RIALTO_CT_STUB_MEDIA_PIPELINE_MODULE_STUB_H_

#include "IIpcServer.h"
#include "MediaCommon.h"
#include "RialtoLogging.h"
#include "mediapipelinemodule.pb.h"
#include <memory>

namespace firebolt::rialto::ct::stub
{
class MediaPipelineModuleStub
{
public:
    explicit MediaPipelineModuleStub(const std::shared_ptr<::firebolt::rialto::MediaPipelineModule> &mediaPipelineModuleMock);
    ~MediaPipelineModuleStub();

    void notifyPlaybackStateChangeEvent(int sessionId, PlaybackState state);
    void notifyNetworkStateChangeEvent(int sessionId, NetworkState state);
    void notifyPositionChangeEvent(int sessionId, int64_t position);
    void notifyNeedMediaDataEvent(int sessionId, int32_t sourceId, size_t frameCount, uint32_t needDataRequestId,
                                  const std::shared_ptr<MediaPlayerShmInfo> &shmInfo);
    void notifyQosEvent(int sessionId, int32_t sourceId, const QosInfo &qosInfo);
    void notifyBufferUnderflowEvent(int sessionId, int32_t sourceId);
    void notifySetLogLevelsEvent(RIALTO_DEBUG_LEVEL defaultLogLevels, RIALTO_DEBUG_LEVEL clientLogLevels,
                                 RIALTO_DEBUG_LEVEL ipcLogLevels, RIALTO_DEBUG_LEVEL commonLogLevels);

    // Client helpers
    virtual void waitForClientConnect() = 0;
    virtual std::shared_ptr<::firebolt::rialto::ipc::IClient> &getClient() = 0;

protected:
    std::shared_ptr<::firebolt::rialto::MediaPipelineModule> m_mediaPipelineModuleMock;
};
} // namespace firebolt::rialto::ct::stub

#endif // FIREBOLT_RIALTO_CT_STUB_MEDIA_PIPELINE_MODULE_STUB_H_
