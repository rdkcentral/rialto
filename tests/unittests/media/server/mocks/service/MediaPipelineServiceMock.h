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

#ifndef FIREBOLT_RIALTO_SERVER_SERVICE_MEDIA_PIPELINE_SERVICE_MOCK_H_
#define FIREBOLT_RIALTO_SERVER_SERVICE_MEDIA_PIPELINE_SERVICE_MOCK_H_

#include "IMediaPipelineService.h"
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <vector>

namespace firebolt::rialto::server::service
{
class MediaPipelineServiceMock : public IMediaPipelineService
{
public:
    MOCK_METHOD(bool, createSession, (int, const std::shared_ptr<IMediaPipelineClient> &, std::uint32_t, std::uint32_t),
                (override));
    MOCK_METHOD(bool, destroySession, (int), (override));
    MOCK_METHOD(bool, load, (int, MediaType, const std::string &, const std::string &), (override));
    MOCK_METHOD(bool, attachSource, (int, const std::unique_ptr<IMediaPipeline::MediaSource> &), (override));
    MOCK_METHOD(bool, removeSource, (int, std::int32_t), (override));
    MOCK_METHOD(bool, allSourcesAttached, (int), (override));
    MOCK_METHOD(bool, play, (int), (override));
    MOCK_METHOD(bool, pause, (int), (override));
    MOCK_METHOD(bool, stop, (int), (override));
    MOCK_METHOD(bool, setPlaybackRate, (int, double), (override));
    MOCK_METHOD(bool, setPosition, (int, int64_t), (override));
    MOCK_METHOD(bool, getPosition, (int sessionId, int64_t &position), (override));
    MOCK_METHOD(bool, setImmediateOutput, (int sessionId, int32_t sourceId, bool immediateOutput), (override));
    MOCK_METHOD(bool, getImmediateOutput, (int sessionId, int32_t sourceId, bool &immediateOutput), (override));
    MOCK_METHOD(bool, getStats, (int sessionId, int32_t sourceId, uint64_t &renderedFrames, uint64_t &droppedFrames),
                (override));
    MOCK_METHOD(bool, setVideoWindow, (int, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t), (override));
    MOCK_METHOD(bool, haveData, (int, MediaSourceStatus, std::uint32_t, std::uint32_t), (override));
    MOCK_METHOD(bool, renderFrame, (int), (override));
    MOCK_METHOD(bool, setVolume, (int sessionId, double targetVolume, uint32_t volumeDuration, EaseType easeType),
                (override));
    MOCK_METHOD(bool, getVolume, (int sessionId, double &volume), (override));

    MOCK_METHOD(bool, setMute, (int sessionId, std::int32_t sourceId, bool mute), (override));
    MOCK_METHOD(bool, getMute, (int sessionId, std::int32_t sourceId, bool &mute), (override));
    MOCK_METHOD(bool, setTextTrackIdentifier, (int sessionId, const std::string &textTrackIdentifier), (override));
    MOCK_METHOD(bool, getTextTrackIdentifier, (int sessionId, std::string &textTrackIdentifier), (override));
    MOCK_METHOD(bool, setLowLatency, (int sessionId, bool lowLatency), (override));
    MOCK_METHOD(bool, setSync, (int sessionId, bool sync), (override));
    MOCK_METHOD(bool, getSync, (int sessionId, bool &sync), (override));
    MOCK_METHOD(bool, setSyncOff, (int sessionId, bool syncOff), (override));
    MOCK_METHOD(bool, setStreamSyncMode, (int sessionId, int32_t sourceId, int32_t streamSyncMode), (override));
    MOCK_METHOD(bool, getStreamSyncMode, (int sessionId, int32_t &streamSyncMode), (override));
    MOCK_METHOD(bool, flush, (int, std::int32_t, bool), (override));
    MOCK_METHOD(bool, setSourcePosition,
                (int sessionId, int32_t sourceId, int64_t position, bool resetTime, double appliedRate), (override));
    MOCK_METHOD(bool, processAudioGap,
                (int sessionId, int64_t position, uint32_t duration, int64_t discontinuityGap, bool isAudioAac),
                (override));
    MOCK_METHOD(bool, setBufferingLimit, (int sessionId, uint32_t limitBufferingMs), (override));
    MOCK_METHOD(bool, getBufferingLimit, (int sessionId, uint32_t &limitBufferingMs), (override));
    MOCK_METHOD(bool, setUseBuffering, (int sessionId, bool useBuffering), (override));
    MOCK_METHOD(bool, getUseBuffering, (int sessionId, bool &useBuffering), (override));
    MOCK_METHOD(std::vector<std::string>, getSupportedMimeTypes, (MediaSourceType type), (override));
    MOCK_METHOD(bool, isMimeTypeSupported, (const std::string &mimeType), (override));
    MOCK_METHOD(std::vector<std::string>, getSupportedProperties,
                (MediaSourceType mediaType, const std::vector<std::string> &propertyNames), (override));
    MOCK_METHOD(void, ping, (const std::shared_ptr<IHeartbeatProcedure> &heartbeatProcedure), (override));
};
} // namespace firebolt::rialto::server::service

#endif // FIREBOLT_RIALTO_SERVER_SERVICE_MEDIA_PIPELINE_SERVICE_MOCK_H_
