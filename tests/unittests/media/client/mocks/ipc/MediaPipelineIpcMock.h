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

#ifndef FIREBOLT_RIALTO_CLIENT_MEDIA_PIPELINE_IPC_MOCK_H_
#define FIREBOLT_RIALTO_CLIENT_MEDIA_PIPELINE_IPC_MOCK_H_

#include "IMediaPipelineIpc.h"
#include <gmock/gmock.h>
#include <memory>
#include <string>

namespace firebolt::rialto::client
{
class MediaPipelineIpcMock : public IMediaPipelineIpc
{
public:
    MediaPipelineIpcMock() = default;
    virtual ~MediaPipelineIpcMock() = default;

    MOCK_METHOD(bool, attachSource, (const std::unique_ptr<IMediaPipeline::MediaSource> &source, int32_t &sourceId),
                (override));
    MOCK_METHOD(bool, removeSource, (int32_t sourceId), (override));
    MOCK_METHOD(bool, allSourcesAttached, (), (override));
    MOCK_METHOD(bool, load, (MediaType type, const std::string &mimeType, const std::string &url), (override));
    MOCK_METHOD(bool, setVideoWindow, (uint32_t x, uint32_t y, uint32_t width, uint32_t height), (override));
    MOCK_METHOD(bool, play, (), (override));
    MOCK_METHOD(bool, pause, (), (override));
    MOCK_METHOD(bool, stop, (), (override));
    MOCK_METHOD(bool, haveData, (MediaSourceStatus status, uint32_t numFrames, uint32_t requestId), (override));
    MOCK_METHOD(bool, setPosition, (int64_t position), (override));
    MOCK_METHOD(bool, getPosition, (int64_t & position), (override));
    MOCK_METHOD(bool, setImmediateOutput, (int32_t sourceId, bool immediateOutput), (override));
    MOCK_METHOD(bool, getImmediateOutput, (int32_t sourceId, bool &immediateOutput), (override));
    MOCK_METHOD(bool, getStats, (int32_t sourceId, uint64_t &renderedFrames, uint64_t &droppedFrames), (override));
    MOCK_METHOD(bool, setPlaybackRate, (double rate), (override));
    MOCK_METHOD(bool, renderFrame, (), (override));
    MOCK_METHOD(bool, setVolume, (double targetVolume, uint32_t volumeDuration, EaseType easeType), (override));
    MOCK_METHOD(bool, getVolume, (double &volume), (override));
    MOCK_METHOD(bool, setMute, (int32_t sourceId, bool mute), (override));
    MOCK_METHOD(bool, getMute, (int32_t sourceId, bool &mute), (override));
    MOCK_METHOD(bool, setTextTrackIdentifier, (const std::string &textTrackIdentifier), (override));
    MOCK_METHOD(bool, getTextTrackIdentifier, (std::string & textTrackIdentifier), (override));
    MOCK_METHOD(bool, setLowLatency, (bool lowLatency), (override));
    MOCK_METHOD(bool, setSync, (bool sync), (override));
    MOCK_METHOD(bool, getSync, (bool &sync), (override));
    MOCK_METHOD(bool, setSyncOff, (bool syncOff), (override));
    MOCK_METHOD(bool, setStreamSyncMode, (int32_t streamSyncMode), (override));
    MOCK_METHOD(bool, getStreamSyncMode, (int32_t & streamSyncMode), (override));
    MOCK_METHOD(bool, flush, (int32_t sourceId, bool resetTime), (override));
    MOCK_METHOD(bool, setSourcePosition, (int32_t sourceId, int64_t position, bool resetTime), (override));
    MOCK_METHOD(bool, processAudioGap, (int64_t position, uint32_t duration, int64_t discontinuityGap, bool isAudioAac),
                (override));
};
} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_MEDIA_PIPELINE_IPC_MOCK_H_
