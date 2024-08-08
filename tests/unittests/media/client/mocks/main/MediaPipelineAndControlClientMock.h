/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Sky UK
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

#ifndef FIREBOLT_RIALTO_CLIENT_MEDIA_PIPELINE_AND_CONTROL_CLIENT_MOCK_H_
#define FIREBOLT_RIALTO_CLIENT_MEDIA_PIPELINE_AND_CONTROL_CLIENT_MOCK_H_

#include <gmock/gmock.h>
#include <memory>
#include <string>

#include "MediaPipeline.h"

namespace firebolt::rialto::client
{
class MediaPipelineAndControlClientMock : public IMediaPipelineAndIControlClient
{
public:
    MOCK_METHOD(bool, load, (MediaType type, const std::string &mimeType, const std::string &url), (override));

    MOCK_METHOD(bool, attachSource, (const std::unique_ptr<MediaSource> &source), (override));

    MOCK_METHOD(bool, removeSource, (int32_t id), (override));

    MOCK_METHOD(bool, allSourcesAttached, (), (override));

    MOCK_METHOD(bool, play, (), (override));
    MOCK_METHOD(bool, pause, (), (override));
    MOCK_METHOD(bool, stop, (), (override));

    MOCK_METHOD(bool, setPlaybackRate, (double rate), (override));

    MOCK_METHOD(bool, setPosition, (int64_t position), (override));

    MOCK_METHOD(bool, getPosition, (int64_t & position), (override));

    MOCK_METHOD(bool, setVideoWindow, (uint32_t x, uint32_t y, uint32_t width, uint32_t height), (override));

    MOCK_METHOD(bool, haveData, (MediaSourceStatus status, uint32_t needDataRequestId), (override));

    MOCK_METHOD(AddSegmentStatus, addSegment,
                (uint32_t needDataRequestId, const std::unique_ptr<MediaSegment> &mediaSegment), (override));

    MOCK_METHOD(bool, renderFrame, (), (override));

    MOCK_METHOD(bool, setVolume, (double volume), (override));
    MOCK_METHOD(bool, getVolume, (double &volume), (override));

    MOCK_METHOD(bool, setMute, (bool volume), (override));
    MOCK_METHOD(bool, getMute, (bool &volume), (override));

    MOCK_METHOD(bool, flush, (int32_t sourceId, bool resetTime), (override));

    MOCK_METHOD(bool, setSourcePosition, (int32_t sourceId, int64_t position), (override));

    MOCK_METHOD(std::weak_ptr<IMediaPipelineClient>, getClient, (), (override));
    MOCK_METHOD(void, notifyApplicationState, (ApplicationState state), (override));
};
} // namespace firebolt::rialto::client

#endif // FIREBOLT_RIALTO_CLIENT_MEDIA_PIPELINE_AND_CONTROL_CLIENT_MOCK_H_
