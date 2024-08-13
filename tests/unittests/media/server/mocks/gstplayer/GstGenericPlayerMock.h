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

#ifndef FIREBOLT_RIALTO_SERVER_GST_GENERIC_PLAYER_MOCK_H_
#define FIREBOLT_RIALTO_SERVER_GST_GENERIC_PLAYER_MOCK_H_

#include "IGstGenericPlayer.h"
#include <gmock/gmock.h>
#include <memory>
#include <string>

namespace firebolt::rialto::server
{
class GstGenericPlayerMock : public IGstGenericPlayer
{
public:
    GstGenericPlayerMock() = default;
    virtual ~GstGenericPlayerMock() = default;

    MOCK_METHOD(void, attachSource, (const std::unique_ptr<IMediaPipeline::MediaSource> &mediaSource), (override));
    MOCK_METHOD(void, removeSource, (const MediaSourceType &mediaSourceType), (override));
    MOCK_METHOD(void, allSourcesAttached, (), (override));
    MOCK_METHOD(void, play, (), (override));
    MOCK_METHOD(void, pause, (), (override));
    MOCK_METHOD(void, stop, (), (override));
    MOCK_METHOD(void, attachSamples, (const IMediaPipeline::MediaSegmentVector &mediaSegments), (override));
    MOCK_METHOD(void, attachSamples, (const std::shared_ptr<IDataReader> &dataReader), (override));
    MOCK_METHOD(void, setPosition, (std::int64_t position), (override));
    MOCK_METHOD(bool, getPosition, (std::int64_t & position), (override));
    MOCK_METHOD(void, setVideoGeometry, (int x, int y, int width, int height), (override));
    MOCK_METHOD(void, setEos, (const firebolt::rialto::MediaSourceType &type), (override));
    MOCK_METHOD(void, setPlaybackRate, (double rate), (override));
    MOCK_METHOD(void, renderFrame, (), (override));
    MOCK_METHOD(void, setVolume, (double volume), (override));
    MOCK_METHOD(bool, getVolume, (double &volume), (override));
    MOCK_METHOD(void, setMute, (bool mute), (override));
    MOCK_METHOD(bool, getMute, (bool &mute), (override));
    MOCK_METHOD(void, ping, (std::unique_ptr<IHeartbeatHandler> && heartbeatHandler), (override));
    MOCK_METHOD(void, flush, (const MediaSourceType &mediaSourceType, bool resetTime), (override));
    MOCK_METHOD(void, setSourcePosition, (const MediaSourceType &mediaSourceType, int64_t position), (override));
    MOCK_METHOD(void, processAudioGap, (int64_t position, uint32_t duration, int64_t discontinuityGap, bool isAudioAac),
                (override));
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_GST_GENERIC_PLAYER_MOCK_H_
