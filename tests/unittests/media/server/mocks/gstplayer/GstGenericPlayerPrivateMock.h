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

#ifndef FIREBOLT_RIALTO_SERVER_GST_GENERIC_PLAYER_PRIVATE_MOCK_H_
#define FIREBOLT_RIALTO_SERVER_GST_GENERIC_PLAYER_PRIVATE_MOCK_H_

#include "IGstGenericPlayerPrivate.h"
#include <gmock/gmock.h>
#include <memory>
#include <string>
#include <vector>

namespace firebolt::rialto::server
{
class GstGenericPlayerPrivateMock : public IGstGenericPlayerPrivate
{
public:
    MOCK_METHOD(void, scheduleNeedMediaData, (GstAppSrc * src), (override));
    MOCK_METHOD(void, scheduleEnoughData, (GstAppSrc * src), (override));
    MOCK_METHOD(void, scheduleAudioUnderflow, (), (override));
    MOCK_METHOD(void, scheduleVideoUnderflow, (), (override));
    MOCK_METHOD(void, scheduleAllSourcesAttached, (), (override));
    MOCK_METHOD(bool, setVideoSinkRectangle, (), (override));
    MOCK_METHOD(void, notifyNeedMediaData, (const MediaSourceType mediaSource), (override));
    MOCK_METHOD(GstBuffer *, createBuffer, (const IMediaPipeline::MediaSegment &mediaSegment), (const, override));
    MOCK_METHOD(void, attachData, (const firebolt::rialto::MediaSourceType mediaType), (override));
    MOCK_METHOD(void, updateAudioCaps, (int32_t rate, int32_t channels, const std::shared_ptr<CodecData> &codecData),
                (override));
    MOCK_METHOD(void, updateVideoCaps,
                (int32_t width, int32_t height, Fraction frameRate, const std::shared_ptr<CodecData> &codecData),
                (override));
    MOCK_METHOD(bool, changePipelineState, (GstState newState), (override));
    MOCK_METHOD(void, startPositionReportingAndCheckAudioUnderflowTimer, (), (override));
    MOCK_METHOD(void, stopPositionReportingAndCheckAudioUnderflowTimer, (), (override));
    MOCK_METHOD(void, stopWorkerThread, (), (override));
    MOCK_METHOD(void, cancelUnderflow, (firebolt::rialto::MediaSourceType mediaSource), (override));
    MOCK_METHOD(void, setPendingPlaybackRate, (), (override));
    MOCK_METHOD(void, updatePlaybackGroup, (GstElement * typefind, const GstCaps *caps), (override));
    MOCK_METHOD(void, addAutoVideoSinkChild, (GObject * object), (override));
    MOCK_METHOD(void, removeAutoVideoSinkChild, (GObject * object), (override));
    MOCK_METHOD(GstElement *, getSinkChildIfAutoVideoSink, (GstElement * sink), (override));
    MOCK_METHOD(void, setPlaybinFlags, (bool enableAudio), (override));
    MOCK_METHOD(void, addAudioClippingToBuffer, (GstBuffer * buffer, uint64_t clippingStart, uint64_t clippingEnd),
                (const, override));
    MOCK_METHOD(void, pushSampleIfRequired, (GstElement * source, const std::string &typeStr), (override));
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_GST_GENERIC_PLAYER_PRIVATE_MOCK_H_
