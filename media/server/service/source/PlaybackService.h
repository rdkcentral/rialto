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

#ifndef FIREBOLT_RIALTO_SERVER_SERVICE_PLAYBACK_SERVICE_H_
#define FIREBOLT_RIALTO_SERVER_SERVICE_PLAYBACK_SERVICE_H_

#include "IDecryptionService.h"
#include "IMainThread.h"
#include "IMediaPipelineServerInternal.h"
#include "IPlaybackService.h"
#include "ISharedMemoryBuffer.h"
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

namespace firebolt::rialto::server::service
{
class PlaybackService : public IPlaybackService
{
public:
    PlaybackService(IMainThread &mainThread, std::shared_ptr<IMediaPipelineServerInternalFactory> &&mediaPipelineFactory,
                    std::unique_ptr<ISharedMemoryBufferFactory> &&shmBufferFactory,
                    IDecryptionService &decryptionService);
    ~PlaybackService() override;
    PlaybackService(const PlaybackService &) = delete;
    PlaybackService(PlaybackService &&) = delete;
    PlaybackService &operator=(const PlaybackService &) = delete;
    PlaybackService &operator=(PlaybackService &&) = delete;

    bool switchToActive() override;
    void switchToInactive() override;
    void setMaxPlaybacks(int maxPlaybacks) override;

    bool createSession(int sessionId, const std::shared_ptr<IMediaPipelineClient> &mediaPipelineClient,
                       std::uint32_t maxWidth, std::uint32_t maxHeight) override;
    bool destroySession(int sessionId) override;
    bool load(int sessionId, MediaType type, const std::string &mimeType, const std::string &url) override;
    bool attachSource(int sessionId, IMediaPipeline::MediaSource &source) override;
    bool removeSource(int sessionId, std::int32_t sourceId) override;
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
    bool getSharedMemory(int32_t &fd, uint32_t &size) override;

private:
    IMainThread &m_mainThread;
    std::shared_ptr<IMediaPipelineServerInternalFactory> m_mediaPipelineFactory;
    std::unique_ptr<ISharedMemoryBufferFactory> m_shmBufferFactory;
    IDecryptionService &m_decryptionService;
    bool m_isActive;
    int m_maxPlaybacks;
    std::map<int, std::unique_ptr<IMediaPipelineServerInternal>> m_mediaPipelines;
    std::shared_ptr<ISharedMemoryBuffer> m_shmBuffer;
};
} // namespace firebolt::rialto::server::service

#endif // FIREBOLT_RIALTO_SERVER_SERVICE_PLAYBACK_SERVICE_H_
