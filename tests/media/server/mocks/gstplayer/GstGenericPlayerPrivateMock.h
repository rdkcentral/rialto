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
    MOCK_METHOD(bool, setWesterossinkRectangle, (), (override));
    MOCK_METHOD(void, notifyNeedMediaData, (bool audioNotificationNeeded, bool videoNotificationNeeded), (override));
    MOCK_METHOD(GstBuffer *, createBuffer, (const IMediaPipeline::MediaSegment &mediaSegment), (const, override));
    MOCK_METHOD(void, attachAudioData, (), (override));
    MOCK_METHOD(void, attachVideoData, (), (override));
    MOCK_METHOD(void, updateAudioCaps,
                (int32_t rate, int32_t channels, const std::shared_ptr<std::vector<std::uint8_t>> &codecData),
                (override));
    MOCK_METHOD(void, updateVideoCaps,
                (int32_t width, int32_t height, const std::shared_ptr<std::vector<std::uint8_t>> &codecData), (override));
    MOCK_METHOD(bool, changePipelineState, (GstState newState), (override));
    MOCK_METHOD(void, startPositionReportingAndCheckAudioUnderflowTimer, (), (override));
    MOCK_METHOD(void, stopPositionReportingAndCheckAudioUnderflowTimer, (), (override));
    MOCK_METHOD(void, stopWorkerThread, (), (override));
    MOCK_METHOD(void, cancelUnderflow, (bool &underflowFlag), (override));
    MOCK_METHOD(void, setPendingPlaybackRate, (), (override));
    MOCK_METHOD(void, updatePlaybackGroup, (GstElement * typefind, const GstCaps *caps), (override));
};
} // namespace firebolt::rialto::server

#endif // FIREBOLT_RIALTO_SERVER_GST_GENERIC_PLAYER_PRIVATE_MOCK_H_
