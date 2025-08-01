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
    MOCK_METHOD(bool, setImmediateOutput, (const MediaSourceType &mediaSourceType, bool immediateOutput), (override));
    MOCK_METHOD(bool, getImmediateOutput, (const MediaSourceType &mediaSourceType, bool &immediateOutput), (override));
    MOCK_METHOD(bool, getStats,
                (const MediaSourceType &mediaSourceType, uint64_t &renderedFrames, uint64_t &droppedFrames), (override));
    MOCK_METHOD(void, setVideoGeometry, (int x, int y, int width, int height), (override));
    MOCK_METHOD(void, setEos, (const firebolt::rialto::MediaSourceType &type), (override));
    MOCK_METHOD(void, setPlaybackRate, (double rate), (override));
    MOCK_METHOD(void, renderFrame, (), (override));
    MOCK_METHOD(void, setVolume, (double targetVolume, uint32_t volumeDuration, firebolt::rialto::EaseType easeType),
                (override));
    MOCK_METHOD(bool, getVolume, (double &volume), (override));
    MOCK_METHOD(void, setMute, (const MediaSourceType &mediaSourceType, bool mute), (override));
    MOCK_METHOD(bool, getMute, (const MediaSourceType &mediaSourceType, bool &mute), (override));
    MOCK_METHOD(void, setTextTrackIdentifier, (const std::string &textTrackIdentifier), (override));
    MOCK_METHOD(bool, getTextTrackIdentifier, (std::string & textTrackIdentifier), (override));
    MOCK_METHOD(bool, setLowLatency, (bool lowLatency), (override));
    MOCK_METHOD(bool, setSync, (bool sync), (override));
    MOCK_METHOD(bool, getSync, (bool &sync), (override));
    MOCK_METHOD(bool, setSyncOff, (bool syncOff), (override));
    MOCK_METHOD(bool, setStreamSyncMode, (const MediaSourceType &mediaSourceType, int32_t streamSyncMode), (override));
    MOCK_METHOD(bool, getStreamSyncMode, (int32_t & streamSyncMode), (override));
    MOCK_METHOD(void, ping, (std::unique_ptr<IHeartbeatHandler> && heartbeatHandler), (override));
    MOCK_METHOD(void, flush, (const MediaSourceType &mediaSourceType, bool resetTime, bool &async), (override));
    MOCK_METHOD(void, setSourcePosition,
                (const MediaSourceType &mediaSourceType, int64_t position, bool resetTime, double appliedRate,
                 uint64_t stopPosition),
                (override));
    MOCK_METHOD(void, processAudioGap, (int64_t position, uint32_t duration, int64_t discontinuityGap, bool isAudioAac),
                (override));
    MOCK_METHOD(void, setBufferingLimit, (uint32_t limitBufferingMs), (override));
    MOCK_METHOD(bool, getBufferingLimit, (uint32_t & limitBufferingMs), (override));
    MOCK_METHOD(void, setUseBuffering, (bool useBuffering), (override));
    MOCK_METHOD(bool, getUseBuffering, (bool &useBuffering), (override));
    MOCK_METHOD(void, switchSource, (const std::unique_ptr<IMediaPipeline::MediaSource> &mediaSource), (override));
    MOCK_METHOD(bool, isVideoMaster, (bool &isVideoMaster), (override));
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_GST_GENERIC_PLAYER_MOCK_H_
