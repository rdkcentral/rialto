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
    MOCK_METHOD(bool, play, (int), (override));
    MOCK_METHOD(bool, pause, (int), (override));
    MOCK_METHOD(bool, stop, (int), (override));
    MOCK_METHOD(bool, setPlaybackRate, (int, double), (override));
    MOCK_METHOD(bool, setPosition, (int, int64_t), (override));
    MOCK_METHOD(bool, getPosition, (int sessionId, int64_t &position), (override));
    MOCK_METHOD(bool, setVideoWindow, (int, std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t), (override));
    MOCK_METHOD(bool, haveData, (int, MediaSourceStatus, std::uint32_t, std::uint32_t), (override));
    MOCK_METHOD(bool, renderFrame, (int), (override));
    MOCK_METHOD(bool, setVolume, (int sessionId, double volume), (override));
    MOCK_METHOD(bool, getVolume, (int sessionId, double &volume), (override));
    MOCK_METHOD(std::vector<std::string>, getSupportedMimeTypes, (MediaSourceType type), (override));
    MOCK_METHOD(bool, isMimeTypeSupported, (const std::string &mimeType), (override));
};
} // namespace firebolt::rialto::server::service

#endif // FIREBOLT_RIALTO_SERVER_SERVICE_MEDIA_PIPELINE_SERVICE_MOCK_H_
