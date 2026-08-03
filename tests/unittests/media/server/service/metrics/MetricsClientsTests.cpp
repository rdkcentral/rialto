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

#include "MediaPipelineClientMock.h"
#include "MediaPipelineMetricsClient.h"
#include "PrivateMetricsServiceMock.h"
#include "WebAudioPlayerClientMock.h"
#include "WebAudioPlayerMetricsClient.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server::service;
using testing::_;
using testing::Ref;
using testing::StrictMock;

TEST(MetricsClientsTests, mediaPipelineStateIsReportedAndForwarded)
{
    constexpr int kSessionId{7};
    auto client{std::make_shared<StrictMock<MediaPipelineClientMock>>()};
    StrictMock<PrivateMetricsServiceMock> metricsService;
    MediaPipelineMetricsClient sut{kSessionId, client, metricsService};

    EXPECT_CALL(metricsService,
                notifyPlaybackStateChanged(kSessionId, PlaybackState::UNKNOWN, PlaybackState::PLAYING));
    EXPECT_CALL(*client, notifyPlaybackState(PlaybackState::PLAYING));
    sut.notifyPlaybackState(PlaybackState::PLAYING);

    EXPECT_CALL(metricsService,
                notifyPlaybackStateChanged(kSessionId, PlaybackState::PLAYING, PlaybackState::PAUSED));
    EXPECT_CALL(*client, notifyPlaybackState(PlaybackState::PAUSED));
    sut.notifyPlaybackState(PlaybackState::PAUSED);
}

TEST(MetricsClientsTests, mediaPipelineCallbacksAreForwarded)
{
    auto client{std::make_shared<StrictMock<MediaPipelineClientMock>>()};
    StrictMock<PrivateMetricsServiceMock> metricsService;
    MediaPipelineMetricsClient sut{1, client, metricsService};
    auto shmInfo{std::make_shared<MediaPlayerShmInfo>(MediaPlayerShmInfo{1, 2, 3, 4})};
    const QosInfo qosInfo{5, 6};
    const PlaybackInfo playbackInfo{7, 0.5};

    EXPECT_CALL(*client, notifyDuration(10));
    sut.notifyDuration(10);
    EXPECT_CALL(*client, notifyPosition(11));
    sut.notifyPosition(11);
    EXPECT_CALL(*client, notifyNativeSize(1920, 1080, 1.5));
    sut.notifyNativeSize(1920, 1080, 1.5);
    EXPECT_CALL(*client, notifyNetworkState(NetworkState::IDLE));
    sut.notifyNetworkState(NetworkState::IDLE);
    EXPECT_CALL(*client, notifyVideoData(true));
    sut.notifyVideoData(true);
    EXPECT_CALL(*client, notifyAudioData(false));
    sut.notifyAudioData(false);
    EXPECT_CALL(*client, notifyNeedMediaData(2, 3, 4, shmInfo));
    sut.notifyNeedMediaData(2, 3, 4, shmInfo);
    EXPECT_CALL(*client, notifyCancelNeedMediaData(5));
    sut.notifyCancelNeedMediaData(5);
    EXPECT_CALL(*client, notifyQos(6, Ref(qosInfo)));
    sut.notifyQos(6, qosInfo);
    EXPECT_CALL(*client, notifyBufferUnderflow(7));
    sut.notifyBufferUnderflow(7);
    EXPECT_CALL(*client, notifyFirstFrameReceived(8));
    sut.notifyFirstFrameReceived(8);
    EXPECT_CALL(*client, notifyPlaybackError(9, PlaybackError::DECRYPTION));
    sut.notifyPlaybackError(9, PlaybackError::DECRYPTION);
    EXPECT_CALL(*client, notifySourceFlushed(10));
    sut.notifySourceFlushed(10);
    EXPECT_CALL(*client, notifyPlaybackInfo(Ref(playbackInfo)));
    sut.notifyPlaybackInfo(playbackInfo);
}

TEST(MetricsClientsTests, webAudioStateIsReportedAndForwarded)
{
    constexpr int kHandle{9};
    auto client{std::make_shared<StrictMock<WebAudioPlayerClientMock>>()};
    StrictMock<PrivateMetricsServiceMock> metricsService;
    WebAudioPlayerMetricsClient sut{kHandle, client, metricsService};

    EXPECT_CALL(metricsService,
                notifyWebAudioPlayerStateChanged(kHandle, WebAudioPlayerState::UNKNOWN, WebAudioPlayerState::PLAYING));
    EXPECT_CALL(*client, notifyState(WebAudioPlayerState::PLAYING));
    sut.notifyState(WebAudioPlayerState::PLAYING);

    EXPECT_CALL(metricsService,
                notifyWebAudioPlayerStateChanged(kHandle, WebAudioPlayerState::PLAYING, WebAudioPlayerState::PAUSED));
    EXPECT_CALL(*client, notifyState(WebAudioPlayerState::PAUSED));
    sut.notifyState(WebAudioPlayerState::PAUSED);
}
