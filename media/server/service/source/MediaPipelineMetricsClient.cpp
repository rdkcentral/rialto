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

#include "MediaPipelineMetricsClient.h"

namespace firebolt::rialto::server::service
{
MediaPipelineMetricsClient::MediaPipelineMetricsClient(int sessionId, const std::shared_ptr<IMediaPipelineClient> &client,
                                                       IPrivateMetricsService &metricsService)
    : m_sessionId{sessionId}, m_client{client}, m_metricsService{metricsService}
{
}

void MediaPipelineMetricsClient::notifyDuration(int64_t duration)
{
    m_client->notifyDuration(duration);
}
void MediaPipelineMetricsClient::notifyPosition(int64_t position)
{
    m_client->notifyPosition(position);
}
void MediaPipelineMetricsClient::notifyNativeSize(uint32_t width, uint32_t height, double aspect)
{
    m_client->notifyNativeSize(width, height, aspect);
}
void MediaPipelineMetricsClient::notifyNetworkState(NetworkState state)
{
    m_client->notifyNetworkState(state);
}
void MediaPipelineMetricsClient::notifyPlaybackState(PlaybackState state)
{
    m_metricsService.notifyPlaybackStateChanged(m_sessionId, m_currentPlaybackState, state);
    m_currentPlaybackState = state;
    m_client->notifyPlaybackState(state);
}
void MediaPipelineMetricsClient::notifyVideoData(bool hasData)
{
    m_client->notifyVideoData(hasData);
}
void MediaPipelineMetricsClient::notifyAudioData(bool hasData)
{
    m_client->notifyAudioData(hasData);
}
void MediaPipelineMetricsClient::notifyNeedMediaData(int32_t sourceId, size_t frameCount, uint32_t needDataRequestId,
                                                     const std::shared_ptr<MediaPlayerShmInfo> &shmInfo)
{
    m_client->notifyNeedMediaData(sourceId, frameCount, needDataRequestId, shmInfo);
}
void MediaPipelineMetricsClient::notifyCancelNeedMediaData(int32_t sourceId)
{
    m_client->notifyCancelNeedMediaData(sourceId);
}
void MediaPipelineMetricsClient::notifyQos(int32_t sourceId, const QosInfo &qosInfo)
{
    m_client->notifyQos(sourceId, qosInfo);
}
void MediaPipelineMetricsClient::notifyBufferUnderflow(int32_t sourceId)
{
    m_client->notifyBufferUnderflow(sourceId);
}
void MediaPipelineMetricsClient::notifyFirstFrameReceived(int32_t sourceId)
{
    m_client->notifyFirstFrameReceived(sourceId);
}
void MediaPipelineMetricsClient::notifyPlaybackError(int32_t sourceId, PlaybackError error)
{
    m_client->notifyPlaybackError(sourceId, error);
}
void MediaPipelineMetricsClient::notifySourceFlushed(int32_t sourceId)
{
    m_client->notifySourceFlushed(sourceId);
}
void MediaPipelineMetricsClient::notifyPlaybackInfo(const PlaybackInfo &playbackInfo)
{
    m_client->notifyPlaybackInfo(playbackInfo);
}
} // namespace firebolt::rialto::server::service
