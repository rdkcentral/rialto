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

#ifndef FIREBOLT_RIALTO_MEDIA_PIPELINE_CLIENT_MOCK_H_
#define FIREBOLT_RIALTO_MEDIA_PIPELINE_CLIENT_MOCK_H_

#include "IMediaPipelineClient.h"
#include <gmock/gmock.h>
#include <memory>

namespace firebolt::rialto
{
class MediaPipelineClientMock : public IMediaPipelineClient
{
public:
    MediaPipelineClientMock() = default;
    virtual ~MediaPipelineClientMock() = default;

    MOCK_METHOD(void, notifyDuration, (int64_t duration), (override));
    MOCK_METHOD(void, notifyPosition, (int64_t position), (override));
    MOCK_METHOD(void, notifyNativeSize, (uint32_t width, uint32_t height, double aspect), (override));
    MOCK_METHOD(void, notifyNetworkState, (NetworkState state), (override));
    MOCK_METHOD(void, notifyPlaybackState, (PlaybackState state), (override));
    MOCK_METHOD(void, notifyVideoData, (bool hasData), (override));
    MOCK_METHOD(void, notifyAudioData, (bool hasData), (override));
    MOCK_METHOD(void, notifyNeedMediaData,
                (int32_t sourceId, size_t frameCount, uint32_t requestId,
                 const std::shared_ptr<MediaPlayerShmInfo> &shmInfo),
                (override));
    MOCK_METHOD(void, notifyCancelNeedMediaData, (int32_t sourceId), (override));
    MOCK_METHOD(void, notifyQos, (int32_t sourceId, const QosInfo &qosInfo), (override));
    MOCK_METHOD(void, notifyBufferUnderflow, (int32_t sourceId), (override));
};
} // namespace firebolt::rialto

#endif // FIREBOLT_RIALTO_MEDIA_PIPELINE_CLIENT_MOCK_H_
