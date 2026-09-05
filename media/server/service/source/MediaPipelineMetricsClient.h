/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
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

#ifndef FIREBOLT_RIALTO_SERVER_SERVICE_MEDIA_PIPELINE_METRICS_CLIENT_H_
#define FIREBOLT_RIALTO_SERVER_SERVICE_MEDIA_PIPELINE_METRICS_CLIENT_H_

#include "IMediaPipelineClient.h"
#include "IPrivateMetricsService.h"
#include <memory>

namespace firebolt::rialto::server::service
{
class MediaPipelineMetricsClient : public IMediaPipelineClient
{
public:
    MediaPipelineMetricsClient(int sessionId, const std::shared_ptr<IMediaPipelineClient> &client,
                               IPrivateMetricsService &metricsService);
    ~MediaPipelineMetricsClient() override = default;

    void notifyDuration(int64_t duration) override;
    void notifyPosition(int64_t position) override;
    void notifyNativeSize(uint32_t width, uint32_t height, double aspect) override;
    void notifyNetworkState(NetworkState state) override;
    void notifyPlaybackState(PlaybackState state) override;
    void notifyVideoData(bool hasData) override;
    void notifyAudioData(bool hasData) override;
    void notifyNeedMediaData(int32_t sourceId, size_t frameCount, uint32_t needDataRequestId,
                             const std::shared_ptr<MediaPlayerShmInfo> &shmInfo) override;
    void notifyCancelNeedMediaData(int32_t sourceId) override;
    void notifyQos(int32_t sourceId, const QosInfo &qosInfo) override;
    void notifyBufferUnderflow(int32_t sourceId) override;
    void notifyFirstFrameReceived(int32_t sourceId) override;
    void notifyPlaybackError(int32_t sourceId, PlaybackError error) override;
    void notifySourceFlushed(int32_t sourceId) override;
    void notifyPlaybackInfo(const PlaybackInfo &playbackInfo) override;

private:
    int m_sessionId;
    std::shared_ptr<IMediaPipelineClient> m_client;
    IPrivateMetricsService &m_metricsService;
    PlaybackState m_currentPlaybackState{PlaybackState::UNKNOWN};
};
} // namespace firebolt::rialto::server::service

#endif // FIREBOLT_RIALTO_SERVER_SERVICE_MEDIA_PIPELINE_METRICS_CLIENT_H_
