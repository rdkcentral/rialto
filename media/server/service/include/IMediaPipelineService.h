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

#ifndef FIREBOLT_RIALTO_SERVER_SERVICE_I_MEDIA_PIPELINE_SERVICE_H_
#define FIREBOLT_RIALTO_SERVER_SERVICE_I_MEDIA_PIPELINE_SERVICE_H_

#include "IHeartbeatProcedure.h"
#include "IMediaPipeline.h"
#include "MediaCommon.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace firebolt::rialto::server::service
{
class IMediaPipelineService
{
public:
    IMediaPipelineService() = default;
    virtual ~IMediaPipelineService() = default;

    IMediaPipelineService(const IMediaPipelineService &) = delete;
    IMediaPipelineService(IMediaPipelineService &&) = delete;
    IMediaPipelineService &operator=(const IMediaPipelineService &) = delete;
    IMediaPipelineService &operator=(IMediaPipelineService &&) = delete;

    virtual bool createSession(int sessionId, const std::shared_ptr<IMediaPipelineClient> &mediaPipelineClient,
                               std::uint32_t maxWidth, std::uint32_t maxHeight) = 0;
    virtual bool destroySession(int sessionId) = 0;
    virtual bool load(int sessionId, MediaType type, const std::string &mimeType, const std::string &url) = 0;
    virtual bool attachSource(int sessionId, const std::unique_ptr<IMediaPipeline::MediaSource> &source) = 0;
    virtual bool removeSource(int sessionId, std::int32_t sourceId) = 0;
    virtual bool allSourcesAttached(int sessionId) = 0;
    virtual bool play(int sessionId) = 0;
    virtual bool pause(int sessionId) = 0;
    virtual bool stop(int sessionId) = 0;
    virtual bool setPlaybackRate(int sessionId, double rate) = 0;
    virtual bool setPosition(int sessionId, std::int64_t position) = 0;
    virtual bool getPosition(int sessionId, std::int64_t &position) = 0;
    virtual bool setVideoWindow(int sessionId, std::uint32_t x, std::uint32_t y, std::uint32_t width,
                                std::uint32_t height) = 0;
    virtual bool haveData(int sessionId, MediaSourceStatus status, std::uint32_t numFrames,
                          std::uint32_t needDataRequestId) = 0;
    virtual bool renderFrame(int sessionId) = 0;
    virtual bool setVolume(int sessionId, double volume) = 0;
    virtual bool getVolume(int sessionId, double &volume) = 0;
    virtual bool setMute(int sessionId, bool mute) = 0;
    virtual bool getMute(int sessionId, bool &mute) = 0;
    virtual std::vector<std::string> getSupportedMimeTypes(MediaSourceType type) = 0;
    virtual bool isMimeTypeSupported(const std::string &mimeType) = 0;
    virtual void ping(const std::shared_ptr<IHeartbeatProcedure> &heartbeatProcedure) = 0;
};
} // namespace firebolt::rialto::server::service

#endif // FIREBOLT_RIALTO_SERVER_SERVICE_I_MEDIA_PIPELINE_SERVICE_H_
