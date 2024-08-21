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

#ifndef FIREBOLT_RIALTO_SERVER_MEDIA_PIPELINE_SERVER_INTERNAL_MOCK_H_
#define FIREBOLT_RIALTO_SERVER_MEDIA_PIPELINE_SERVER_INTERNAL_MOCK_H_

#include "IMediaPipelineServerInternal.h"
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <vector>

namespace firebolt::rialto::server
{
class MediaPipelineServerInternalMock : public IMediaPipelineServerInternal
{
public:
    MOCK_METHOD(std::weak_ptr<IMediaPipelineClient>, getClient, (), (override));
    MOCK_METHOD(bool, load, (MediaType type, const std::string &mimeType, const std::string &url), (override));
    MOCK_METHOD(bool, attachSource, (const std::unique_ptr<MediaSource> &source), (override));
    MOCK_METHOD(bool, removeSource, (int32_t id), (override));
    MOCK_METHOD(bool, allSourcesAttached, (), (override));
    MOCK_METHOD(bool, play, (), (override));
    MOCK_METHOD(bool, pause, (), (override));
    MOCK_METHOD(bool, stop, (), (override));
    MOCK_METHOD(bool, setPlaybackRate, (double rate), (override));
    MOCK_METHOD(bool, setPosition, (int64_t position), (override));
    MOCK_METHOD(bool, getPosition, (std::int64_t & position), (override));
    MOCK_METHOD(bool, setImmediateOutput, (int32_t sourceId, bool immediateOutput), (override));
    MOCK_METHOD(bool, getImmediateOutput, (int32_t sourceId, bool &immediateOutput), (override));

    MOCK_METHOD(bool, setVideoWindow, (uint32_t x, uint32_t y, uint32_t width, uint32_t height), (override));
    MOCK_METHOD(bool, haveData, (MediaSourceStatus status, uint32_t numFrames, uint32_t needDataRequestId), (override));
    MOCK_METHOD(bool, haveData, (MediaSourceStatus status, uint32_t needDataRequestId), (override));
    MOCK_METHOD(bool, renderFrame, (), (override));
    MOCK_METHOD(AddSegmentStatus, addSegment,
                (uint32_t needDataRequestId, const std::unique_ptr<MediaSegment> &mediaSegment), (override));
    MOCK_METHOD(bool, setVolume, (double volume), (override));
    MOCK_METHOD(bool, getVolume, (double &volume), (override));
    MOCK_METHOD(bool, setMute, (bool mute), (override));
    MOCK_METHOD(bool, getMute, (bool &mute), (override));
    MOCK_METHOD(void, ping, (std::unique_ptr<IHeartbeatHandler> && heartbeatHandler), (override));
    MOCK_METHOD(bool, flush, (int32_t sourceId, bool resetTime), (override));
    MOCK_METHOD(bool, setSourcePosition, (int32_t sourceId, int64_t position), (override));
    MOCK_METHOD(bool, processAudioGap, (int64_t position, uint32_t duration, int64_t discontinuityGap, bool isAudioAac),
                (override));
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_MEDIA_PIPELINE_SERVER_INTERNAL_MOCK_H_
