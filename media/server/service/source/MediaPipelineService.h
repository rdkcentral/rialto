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

#ifndef FIREBOLT_RIALTO_SERVER_SERVICE_MEDIA_PIPELINE_SERVICE_H_
#define FIREBOLT_RIALTO_SERVER_SERVICE_MEDIA_PIPELINE_SERVICE_H_

#include "IDecryptionService.h"
#include "IMediaPipelineCapabilities.h"
#include "IMediaPipelineServerInternal.h"
#include "IMediaPipelineService.h"
#include "IPlaybackService.h"
#include "ISharedMemoryBuffer.h"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

namespace firebolt::rialto::server::service
{
class MediaPipelineService : public IMediaPipelineService
{
public:
    MediaPipelineService(IPlaybackService &playbackService,
                         std::shared_ptr<IMediaPipelineServerInternalFactory> &&mediaPipelineFactory,
                         std::shared_ptr<IMediaPipelineCapabilitiesFactory> &&mediaPipelineCapabilitiesFactory,
                         IDecryptionService &decryptionService);
    ~MediaPipelineService() override;
    MediaPipelineService(const MediaPipelineService &) = delete;
    MediaPipelineService(MediaPipelineService &&) = delete;
    MediaPipelineService &operator=(const MediaPipelineService &) = delete;
    MediaPipelineService &operator=(MediaPipelineService &&) = delete;

    bool createSession(int sessionId, const std::shared_ptr<IMediaPipelineClient> &mediaPipelineClient,
                       std::uint32_t maxWidth, std::uint32_t maxHeight) override;
    bool destroySession(int sessionId) override;
    bool load(int sessionId, MediaType type, const std::string &mimeType, const std::string &url) override;
    bool attachSource(int sessionId, const std::unique_ptr<IMediaPipeline::MediaSource> &source) override;
    bool removeSource(int sessionId, std::int32_t sourceId) override;
    bool allSourcesAttached(int sessionId) override;
    bool play(int sessionId) override;
    bool pause(int sessionId) override;
    bool stop(int sessionId) override;
    bool setPlaybackRate(int sessionId, double rate) override;
    bool setPosition(int sessionId, std::int64_t position) override;
    bool getPosition(int sessionId, std::int64_t &position) override;
    bool setVideoWindow(int sessionId, std::uint32_t x, std::uint32_t y, std::uint32_t width,
                        std::uint32_t height) override;
    bool haveData(int sessionId, MediaSourceStatus status, std::uint32_t numFrames,
                  std::uint32_t needDataRequestId) override;
    bool renderFrame(int sessionId) override;
    bool setVolume(int sessionId, double volume) override;
    bool getVolume(int sessionId, double &volume) override;
    bool setMute(int sessionId, bool mute) override;
    bool getMute(int sessionId, bool &mute) override;
    bool flush(int sessionId, std::int32_t sourceId, bool resetTime) override;
    bool setSourcePosition(int sessionId, int32_t sourceId, int64_t position) override;
    bool processAudioGap(int sessionId, int64_t position, uint32_t duration, int64_t discontinuityGap,
                         bool audioAac) override;
    std::vector<std::string> getSupportedMimeTypes(MediaSourceType type) override;
    bool isMimeTypeSupported(const std::string &mimeType) override;
    void ping(const std::shared_ptr<IHeartbeatProcedure> &heartbeatProcedure) override;

    void clearMediaPipelines();

private:
    IPlaybackService &m_playbackService;
    std::shared_ptr<IMediaPipelineServerInternalFactory> m_mediaPipelineFactory;
    std::shared_ptr<IMediaPipelineCapabilities> m_mediaPipelineCapabilities;
    IDecryptionService &m_decryptionService;
    std::map<int, std::unique_ptr<IMediaPipelineServerInternal>> m_mediaPipelines;
    std::mutex m_mediaPipelineMutex;
};
} // namespace firebolt::rialto::server::service

#endif // FIREBOLT_RIALTO_SERVER_SERVICE_MEDIA_PIPELINE_SERVICE_H_
