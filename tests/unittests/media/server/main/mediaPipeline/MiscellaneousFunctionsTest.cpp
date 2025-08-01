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

#include "HeartbeatHandlerMock.h"
#include "MediaPipelineTestBase.h"

using ::testing::ByMove;
using ::testing::SetArgReferee;

class RialtoServerMediaPipelineMiscellaneousFunctionsTest : public MediaPipelineTestBase
{
protected:
    const int64_t m_kPosition{4028596027};
    const double m_kPlaybackRate{1.5};
    uint64_t m_kRenderedFrames{3141};
    uint64_t m_kDroppedFrames{95};
    const int m_kDummySourceId{123};

    RialtoServerMediaPipelineMiscellaneousFunctionsTest() { createMediaPipeline(); }

    ~RialtoServerMediaPipelineMiscellaneousFunctionsTest() { destroyMediaPipeline(); }
};

/**
 * Test that Play returns success if the gstreamer player API succeeds and sets the pipeline state.
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, PlaySuccess)
{
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();

    EXPECT_CALL(*m_gstPlayerMock, play());
    EXPECT_TRUE(m_mediaPipeline->play());
}

/**
 * Test that Play returns failure if the gstreamer player is not initialized
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, PlayFailureDueToUninitializedPlayer)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_FALSE(m_mediaPipeline->play());
}

/**
 * Test that Stop returns success if the gstreamer player API succeeds and sets the pipeline state.
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, StopSuccess)
{
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();

    EXPECT_CALL(*m_gstPlayerMock, stop());
    EXPECT_TRUE(m_mediaPipeline->stop());
}

/**
 * Test that Stop returns failure if the gstreamer player is not initialized
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, StopFailureDueToUninitializedPlayer)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_FALSE(m_mediaPipeline->stop());
}

/**
 * Test that Pause returns success if the gstreamer player API succeeds and sets the pipeline state.
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, PauseSuccess)
{
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();

    EXPECT_CALL(*m_gstPlayerMock, pause());
    EXPECT_TRUE(m_mediaPipeline->pause());
}

/**
 * Test that Pause returns failure if the gstreamer player is not initialized
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, PauseFailureDueToUninitializedPlayer)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_FALSE(m_mediaPipeline->pause());
}

/**
 * Test that SetVideoWindow returns success if the gstreamer player API succeeds and sets the video geometry
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetVideoWindowSuccess)
{
    uint32_t x{3};
    uint32_t y{4};
    uint32_t width{640};
    uint32_t height{480};
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();

    EXPECT_CALL(*m_gstPlayerMock, setVideoGeometry(x, y, width, height));
    EXPECT_TRUE(m_mediaPipeline->setVideoWindow(x, y, width, height));
}

/**
 * Test that SetVideoWindow returns failure if the gstreamer player is not initialized
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetVideoWindowFailureDueToUninitializedPlayer)
{
    uint32_t x{3};
    uint32_t y{4};
    uint32_t width{640};
    uint32_t height{480};
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_FALSE(m_mediaPipeline->setVideoWindow(x, y, width, height));
}

/**
 * Test that SetPosition returns failure if the gstreamer player is not initialized
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetPositionFailureDueToUninitializedPlayer)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_FALSE(m_mediaPipeline->setPosition(m_kPosition));
}

/**
 * Test that SetPosition succeeds.
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetPositionSuccess)
{
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();

    EXPECT_CALL(*m_gstPlayerMock, setPosition(m_kPosition));
    EXPECT_TRUE(m_mediaPipeline->setPosition(m_kPosition));
}

/**
 * Test that SetPosition resets the Eos flag on success
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetPositionResetEos)
{
    loadGstPlayer();
    int videoSourceId = attachSource(firebolt::rialto::MediaSourceType::VIDEO, "video/h264");
    int audioSourceId = attachSource(firebolt::rialto::MediaSourceType::AUDIO, "audio/x-opus");
    setEos(firebolt::rialto::MediaSourceType::VIDEO);
    setEos(firebolt::rialto::MediaSourceType::AUDIO);

    mainThreadWillEnqueueTaskAndWait();

    EXPECT_CALL(*m_gstPlayerMock, setPosition(m_kPosition));
    EXPECT_TRUE(m_mediaPipeline->setPosition(m_kPosition));

    // Expect need data notified to client
    expectNotifyNeedData(firebolt::rialto::MediaSourceType::VIDEO, videoSourceId, 3);
    m_gstPlayerCallback->notifyNeedMediaData(firebolt::rialto::MediaSourceType::VIDEO);

    expectNotifyNeedData(firebolt::rialto::MediaSourceType::AUDIO, audioSourceId, 3);
    m_gstPlayerCallback->notifyNeedMediaData(firebolt::rialto::MediaSourceType::AUDIO);
}

/**
 * Test that SetPlaybackRate returns failure if the gstreamer player is not initialized
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetPlaybackRateFailureDueToUninitializedPlayer)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_FALSE(m_mediaPipeline->setPlaybackRate(m_kPlaybackRate));
}

/**
 * Test that SetPlaybackRate returns failure if rate is 0.0
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetPlaybackRateFailureDueToWrongRateValue)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_FALSE(m_mediaPipeline->setPlaybackRate(0.0));
}

/**
 * Test that SetPlaybackRate returns failure if the gstreamer API succeeds and sets playback rate
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetPlaybackRateSuccess)
{
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();

    EXPECT_CALL(*m_gstPlayerMock, setPlaybackRate(m_kPlaybackRate));
    EXPECT_TRUE(m_mediaPipeline->setPlaybackRate(m_kPlaybackRate));
}

/**
 * Test that GetPosition returns failure if the gstreamer player is not initialized
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, GetPositionFailureDueToUninitializedPlayer)
{
    mainThreadWillEnqueuePriorityTaskAndWait();
    int64_t targetPosition{};
    EXPECT_FALSE(m_mediaPipeline->getPosition(targetPosition));
}

/**
 * Test that GetPosition returns failure if the gstreamer API fails
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, GetPositionFailure)
{
    loadGstPlayer();
    mainThreadWillEnqueuePriorityTaskAndWait();
    int64_t targetPosition{};
    EXPECT_CALL(*m_gstPlayerMock, getPosition(_)).WillOnce(Return(false));
    EXPECT_FALSE(m_mediaPipeline->getPosition(targetPosition));
}

/**
 * Test that GetPosition returns success if the gstreamer API succeeds and gets playback rate
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, GetPositionSuccess)
{
    loadGstPlayer();
    mainThreadWillEnqueuePriorityTaskAndWait();
    int64_t targetPosition{};
    EXPECT_CALL(*m_gstPlayerMock, getPosition(_))
        .WillOnce(Invoke(
            [&](int64_t &pos)
            {
                pos = m_kPosition;
                return true;
            }));
    EXPECT_TRUE(m_mediaPipeline->getPosition(targetPosition));
    EXPECT_EQ(targetPosition, m_kPosition);
}

/**
 * Test that SetImmediateOutput returns failure if the gstreamer player is not initialized
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetImmediateOutputFailureDueToUninitializedPlayer)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_FALSE(m_mediaPipeline->setImmediateOutput(m_kDummySourceId, true));
}

/**
 * Test that SetImmediateOutput returns failure if the gstreamer API fails
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetImmediateOutputFailure)
{
    loadGstPlayer();
    int videoSourceId = attachSource(firebolt::rialto::MediaSourceType::VIDEO, "video/h264");
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, setImmediateOutput(_, _)).WillOnce(Return(false));
    EXPECT_FALSE(m_mediaPipeline->setImmediateOutput(videoSourceId, true));
}

/**
 * Test that SetImmediateOutput fails if source is not present.
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetImmediateOutputNoSourcePresent)
{
    loadGstPlayer();
    // No attachment of source
    mainThreadWillEnqueueTaskAndWait();

    EXPECT_FALSE(m_mediaPipeline->setImmediateOutput(m_kDummySourceId, true));
}

/**
 * Test that SetImmediateOutput returns success if the gstreamer API succeeds
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetImmediateOutputSuccess)
{
    loadGstPlayer();
    int videoSourceId = attachSource(firebolt::rialto::MediaSourceType::VIDEO, "video/h264");

    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, setImmediateOutput(_, true)).WillOnce(Return(true));
    EXPECT_TRUE(m_mediaPipeline->setImmediateOutput(videoSourceId, true));

    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, setImmediateOutput(_, false)).WillOnce(Return(true));
    EXPECT_TRUE(m_mediaPipeline->setImmediateOutput(videoSourceId, false));
}

/**
 * Test that GetImmediateOutput returns failure if the gstreamer player is not initialized
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, GetImmediateOutputFailureDueToUninitializedPlayer)
{
    mainThreadWillEnqueueTaskAndWait();
    bool immediateOutputState;
    EXPECT_FALSE(m_mediaPipeline->getImmediateOutput(m_kDummySourceId, immediateOutputState));
}

/**
 * Test that GetImmediateOutput returns failure if the gstreamer API fails
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, GetImmediateOutputFailure)
{
    loadGstPlayer();
    int videoSourceId = attachSource(firebolt::rialto::MediaSourceType::VIDEO, "video/h264");
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, getImmediateOutput(_, _)).WillOnce(Return(false));
    bool immediateOutputState;
    EXPECT_FALSE(m_mediaPipeline->getImmediateOutput(videoSourceId, immediateOutputState));
}

/**
 * Test that GetImmediateOutput fails if source is not present.
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, GetImmediateOutputNoSourcePresent)
{
    loadGstPlayer();
    // No attachment of source
    mainThreadWillEnqueueTaskAndWait();

    bool immediateOutputState;
    EXPECT_FALSE(m_mediaPipeline->getImmediateOutput(m_kDummySourceId, immediateOutputState));
}

/**
 * Test that GetImmediateOutput returns success if the gstreamer API succeeds
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, GetImmediateOutputSuccess)
{
    loadGstPlayer();
    int videoSourceId = attachSource(firebolt::rialto::MediaSourceType::VIDEO, "video/h264");

    bool immediateOutputState;
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, getImmediateOutput(_, _)).WillOnce(DoAll(SetArgReferee<1>(true), Return(true)));
    EXPECT_TRUE(m_mediaPipeline->getImmediateOutput(videoSourceId, immediateOutputState));
    EXPECT_EQ(immediateOutputState, true);

    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, getImmediateOutput(_, _)).WillOnce(DoAll(SetArgReferee<1>(false), Return(true)));
    EXPECT_TRUE(m_mediaPipeline->getImmediateOutput(videoSourceId, immediateOutputState));
    EXPECT_EQ(immediateOutputState, false);
}

/**
 * Test that GetStats returns failure if the gstreamer player is not initialized
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, GetStatsFailureDueToUninitializedPlayer)
{
    mainThreadWillEnqueueTaskAndWait();
    uint64_t renderedFrames;
    uint64_t droppedFrames;
    EXPECT_FALSE(m_mediaPipeline->getStats(m_kDummySourceId, renderedFrames, droppedFrames));
}

/**
 * Test that GetStats returns failure if the gstreamer API fails
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, GetStatsFailure)
{
    loadGstPlayer();
    int videoSourceId = attachSource(firebolt::rialto::MediaSourceType::VIDEO, "video/h264");
    mainThreadWillEnqueueTaskAndWait();
    uint64_t renderedFrames;
    uint64_t droppedFrames;
    EXPECT_CALL(*m_gstPlayerMock, getStats(_, _, _)).WillOnce(Return(false));
    EXPECT_FALSE(m_mediaPipeline->getStats(videoSourceId, renderedFrames, droppedFrames));
}

/**
 * Test that GetStats fails if source is not present.
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, GetStatsNoSourcePresent)
{
    loadGstPlayer();
    // No attachment of source
    mainThreadWillEnqueueTaskAndWait();

    uint64_t renderedFrames;
    uint64_t droppedFrames;
    EXPECT_FALSE(m_mediaPipeline->getStats(m_kDummySourceId, renderedFrames, droppedFrames));
}

/**
 * Test that GetStats returns success if the gstreamer API succeeds and gets playback rate
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, GetStatsSuccess)
{
    loadGstPlayer();
    int videoSourceId = attachSource(firebolt::rialto::MediaSourceType::VIDEO, "video/h264");
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, getStats(_, _, _))
        .WillOnce(Invoke(
            [&](const MediaSourceType &mediaSourceType, uint64_t &renderedFrames, uint64_t &droppedFrames)
            {
                renderedFrames = m_kRenderedFrames;
                droppedFrames = m_kDroppedFrames;
                return true;
            }));
    uint64_t renderedFrames;
    uint64_t droppedFrames;
    EXPECT_TRUE(m_mediaPipeline->getStats(videoSourceId, renderedFrames, droppedFrames));
    EXPECT_EQ(renderedFrames, m_kRenderedFrames);
    EXPECT_EQ(droppedFrames, m_kDroppedFrames);
}

TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, RenderFrameSuccess)
{
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, renderFrame()).Times(1);
    EXPECT_TRUE(m_mediaPipeline->renderFrame());
}

TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, RenderFrameFail)
{
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_FALSE(m_mediaPipeline->renderFrame());
}

/**
 * Test that SetVolume returns failure if the gstreamer player is not initialized
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetVolumeFailureDueToUninitializedPlayer)
{
    const double kVolume{0.7};
    const uint32_t kVolumeDuration = 0;
    const firebolt::rialto::EaseType kEaseType = firebolt::rialto::EaseType::EASE_LINEAR;
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_FALSE(m_mediaPipeline->setVolume(kVolume, kVolumeDuration, kEaseType));
}

/**
 * Test that SetVolume returns failure if the gstreamer player is not initialized
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetVolumeSuccess)
{
    const double kVolume{0.7};
    const uint32_t kVolumeDuration = 0;
    const firebolt::rialto::EaseType kEaseType = firebolt::rialto::EaseType::EASE_LINEAR;
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();

    EXPECT_CALL(*m_gstPlayerMock, setVolume(kVolume, kVolumeDuration, kEaseType));
    EXPECT_TRUE(m_mediaPipeline->setVolume(kVolume, kVolumeDuration, kEaseType));
}

/**
 * Test that GetVolume returns failure if the gstreamer player is not initialized
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, GetVolumeFailureDueToUninitializedPlayer)
{
    mainThreadWillEnqueueTaskAndWait();
    double resultVolume{};
    EXPECT_FALSE(m_mediaPipeline->getVolume(resultVolume));
}

/**
 * Test that GetVolume returns failure if the gstreamer API fails
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, GetVolumeFailure)
{
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
    double resultVolume{};
    EXPECT_CALL(*m_gstPlayerMock, getVolume(_)).WillOnce(Return(false));
    EXPECT_FALSE(m_mediaPipeline->getVolume(resultVolume));
}

/**
 * Test that GetVolume returns success if the gstreamer API succeeds and gets volume
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, GetVolumeSuccess)
{
    constexpr double kCurrentVolume{0.7};
    double resultVolume{};

    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, getVolume(_))
        .WillOnce(Invoke(
            [&](double &vol)
            {
                vol = kCurrentVolume;
                return true;
            }));
    EXPECT_TRUE(m_mediaPipeline->getVolume(resultVolume));
    EXPECT_EQ(resultVolume, kCurrentVolume);
}

/**
 * Test that SetMute returns failure if the gstreamer player is not initialized
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetMuteFailureDueToUninitializedPlayer)
{
    const bool kMute{false};
    const int32_t kUnknownSourceId{-1};
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_FALSE(m_mediaPipeline->setMute(kUnknownSourceId, kMute));
}

/**
 * Test that SetMute returns failure if the source is not attached
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetMuteFailureDueToUnattachedSource)
{
    const bool kMute{false};
    const int32_t kUnknownSourceId{-1};
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_FALSE(m_mediaPipeline->setMute(kUnknownSourceId, kMute));
}

/**
 * Test that SetMute returns success
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetMuteSuccess)
{
    const bool kMute{false};
    loadGstPlayer();
    int audioSourceId = attachSource(firebolt::rialto::MediaSourceType::AUDIO, "audio/x-opus");

    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, setMute(firebolt::rialto::MediaSourceType::AUDIO, kMute));
    EXPECT_TRUE(m_mediaPipeline->setMute(audioSourceId, kMute));
}

/**
 * Test that GetMute returns failure if the gstreamer player is not initialized
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, GetMuteFailureDueToUninitializedPlayer)
{
    const int32_t kUnknownSourceId{-1};
    mainThreadWillEnqueueTaskAndWait();
    bool resultMute{};
    EXPECT_FALSE(m_mediaPipeline->getMute(kUnknownSourceId, resultMute));
}

/**
 * Test that GetMute returns failure if the source is not attached
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, GetMuteFailureDueToUnattachedSource)
{
    const int32_t kUnknownSourceId{-1};
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
    bool resultMute{};
    EXPECT_FALSE(m_mediaPipeline->getMute(kUnknownSourceId, resultMute));
}

/**
 * Test that GetMute returns failure if the gstreamer API fails
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, GetMuteFailure)
{
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
    int audioSourceId = attachSource(firebolt::rialto::MediaSourceType::AUDIO, "audio/x-opus");
    bool resultMute{};
    EXPECT_CALL(*m_gstPlayerMock, getMute(MediaSourceType::AUDIO, _)).WillOnce(Return(false));
    EXPECT_FALSE(m_mediaPipeline->getMute(audioSourceId, resultMute));
}

/**
 * Test that GetMute returns success if the gstreamer API succeeds and gets mute
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, GetMuteSuccess)
{
    constexpr bool kCurrentMute{false};
    bool resultMute{};

    loadGstPlayer();
    int audioSourceId = attachSource(firebolt::rialto::MediaSourceType::AUDIO, "audio/x-opus");

    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, getMute(MediaSourceType::AUDIO, _))
        .WillOnce(Invoke(
            [&](MediaSourceType, bool &mut)
            {
                mut = kCurrentMute;
                return true;
            }));

    EXPECT_TRUE(m_mediaPipeline->getMute(audioSourceId, resultMute));
    EXPECT_EQ(resultMute, kCurrentMute);
}

/**
 * Test that SetLowLatency returns failure if the gstreamer player is not initialized
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetLowLatencyFailureDueToUninitializedPlayer)
{
    mainThreadWillEnqueueTaskAndWait();
    constexpr bool kLowLatency{true};
    EXPECT_FALSE(m_mediaPipeline->setLowLatency(kLowLatency));
}

/**
 * Test that SetLowLatency returns failure if the gstreamer API fails
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetLowLatencyFailure)
{
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
    constexpr bool kLowLatency{true};
    EXPECT_CALL(*m_gstPlayerMock, setLowLatency(_)).WillOnce(Return(false));
    EXPECT_FALSE(m_mediaPipeline->setLowLatency(kLowLatency));
}

/**
 * Test that SetLowLatency returns success if the gstreamer API succeeds and gets mute
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetLowLatencySuccess)
{
    constexpr bool kLowLatency{true};
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, setLowLatency(kLowLatency)).WillOnce(Return(true));

    EXPECT_TRUE(m_mediaPipeline->setLowLatency(kLowLatency));
}

/**
 * Test that SetSync returns failure if the gstreamer player is not initialized
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetSyncFailureDueToUninitializedPlayer)
{
    mainThreadWillEnqueueTaskAndWait();
    constexpr bool kSync{true};
    EXPECT_FALSE(m_mediaPipeline->setSync(kSync));
}

/**
 * Test that SetSync returns failure if the gstreamer API fails
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetSyncFailure)
{
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
    constexpr bool kSync{true};
    EXPECT_CALL(*m_gstPlayerMock, setSync(_)).WillOnce(Return(false));
    EXPECT_FALSE(m_mediaPipeline->setSync(kSync));
}

/**
 * Test that SetSync returns success if the gstreamer API succeeds and gets mute
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetSyncSuccess)
{
    constexpr bool kSync{true};
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, setSync(kSync)).WillOnce(Return(true));

    EXPECT_TRUE(m_mediaPipeline->setSync(kSync));
}

/**
 * Test that GetSync returns failure if the gstreamer player is not initialized
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, GetSyncFailureDueToUninitializedPlayer)
{
    mainThreadWillEnqueueTaskAndWait();
    bool resultSync{};
    EXPECT_FALSE(m_mediaPipeline->getSync(resultSync));
}

/**
 * Test that GetSync returns failure if the gstreamer API fails
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, GetSyncFailure)
{
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
    bool resultSync{};
    EXPECT_CALL(*m_gstPlayerMock, getSync(_)).WillOnce(Return(false));
    EXPECT_FALSE(m_mediaPipeline->getSync(resultSync));
}

/**
 * Test that GetSync returns success if the gstreamer API succeeds and gets mute
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, GetSyncSuccess)
{
    constexpr bool kSync{true};
    bool resultSync{};
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, getSync(_)).WillOnce(DoAll(SetArgReferee<0>(kSync), Return(true)));

    EXPECT_TRUE(m_mediaPipeline->getSync(resultSync));
    EXPECT_EQ(resultSync, kSync);
}

/**
 * Test that SetSyncOff returns failure if the gstreamer player is not initialized
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetSyncOffFailureDueToUninitializedPlayer)
{
    mainThreadWillEnqueueTaskAndWait();
    constexpr bool kSyncOff{true};
    EXPECT_FALSE(m_mediaPipeline->setSyncOff(kSyncOff));
}

/**
 * Test that SetSyncOff returns failure if the gstreamer API fails
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetSyncOffFailure)
{
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
    constexpr bool kSyncOff{true};
    EXPECT_CALL(*m_gstPlayerMock, setSyncOff(_)).WillOnce(Return(false));
    EXPECT_FALSE(m_mediaPipeline->setSyncOff(kSyncOff));
}

/**
 * Test that SetSyncOff returns success if the gstreamer API succeeds and gets mute
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetSyncOffSuccess)
{
    constexpr bool kSyncOff{true};
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, setSyncOff(kSyncOff)).WillOnce(Return(true));

    EXPECT_TRUE(m_mediaPipeline->setSyncOff(kSyncOff));
}

/**
 * Test that SetStreamSyncMode returns failure if the gstreamer player is not initialized
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetStreamSyncModeFailureDueToUninitializedPlayer)
{
    mainThreadWillEnqueueTaskAndWait();
    constexpr int32_t kStreamSyncMode{1};
    EXPECT_FALSE(m_mediaPipeline->setStreamSyncMode(m_kDummySourceId, kStreamSyncMode));
}

/**
 * Test that SetStreamSyncMode returns failure if source is not attached
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetStreamSyncModeFailureNoSourceAttached)
{
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
    constexpr int32_t kStreamSyncMode{1};
    EXPECT_FALSE(m_mediaPipeline->setStreamSyncMode(m_kDummySourceId, kStreamSyncMode));
}

/**
 * Test that SetStreamSyncMode returns failure if the gstreamer API fails
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetStreamSyncModeFailure)
{
    loadGstPlayer();
    int videoSourceId = attachSource(firebolt::rialto::MediaSourceType::VIDEO, "video/h264");
    mainThreadWillEnqueueTaskAndWait();
    constexpr int32_t kStreamSyncMode{1};
    EXPECT_CALL(*m_gstPlayerMock, setStreamSyncMode(firebolt::rialto::MediaSourceType::VIDEO, _)).WillOnce(Return(false));
    EXPECT_FALSE(m_mediaPipeline->setStreamSyncMode(videoSourceId, kStreamSyncMode));
}

/**
 * Test that SetStreamSyncMode returns success if the gstreamer API succeeds
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetStreamSyncModeSuccess)
{
    constexpr int32_t kStreamSyncMode{1};
    loadGstPlayer();
    int videoSourceId = attachSource(firebolt::rialto::MediaSourceType::VIDEO, "video/h264");
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, setStreamSyncMode(firebolt::rialto::MediaSourceType::VIDEO, kStreamSyncMode))
        .WillOnce(Return(true));

    EXPECT_TRUE(m_mediaPipeline->setStreamSyncMode(videoSourceId, kStreamSyncMode));
}

/**
 * Test that GetStreamSyncMode returns failure if the gstreamer player is not initialized
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, GetStreamSyncModeFailureDueToUninitializedPlayer)
{
    mainThreadWillEnqueueTaskAndWait();
    int32_t resultStreamSyncMode{};
    EXPECT_FALSE(m_mediaPipeline->getStreamSyncMode(resultStreamSyncMode));
}

/**
 * Test that GetStreamSyncMode returns failure if the gstreamer API fails
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, GetStreamSyncModeFailure)
{
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
    int32_t resultStreamSyncMode{};
    EXPECT_CALL(*m_gstPlayerMock, getStreamSyncMode(_)).WillOnce(Return(false));
    EXPECT_FALSE(m_mediaPipeline->getStreamSyncMode(resultStreamSyncMode));
}

/**
 * Test that GetStreamSyncMode returns success if the gstreamer API succeeds
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, GetStreamSyncModeSuccess)
{
    constexpr int32_t kStreamSyncMode{1};
    int32_t resultStreamSyncMode{};
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, getStreamSyncMode(_)).WillOnce(DoAll(SetArgReferee<0>(kStreamSyncMode), Return(true)));

    EXPECT_TRUE(m_mediaPipeline->getStreamSyncMode(resultStreamSyncMode));
    EXPECT_EQ(resultStreamSyncMode, kStreamSyncMode);
}

/**
 * Test that SetBufferingLimit returns failure if the gstreamer player is not initialized
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetBufferingLimitFailureDueToUninitializedPlayer)
{
    mainThreadWillEnqueueTaskAndWait();
    constexpr int32_t kBufferingLimit{1};
    EXPECT_FALSE(m_mediaPipeline->setBufferingLimit(kBufferingLimit));
}

/**
 * Test that SetBufferingLimit returns success if the gstreamer API succeeds
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetBufferingLimitSuccess)
{
    constexpr int32_t kBufferingLimit{1};
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, setBufferingLimit(kBufferingLimit));
    EXPECT_TRUE(m_mediaPipeline->setBufferingLimit(kBufferingLimit));
}

/**
 * Test that GetBufferingLimit returns failure if the gstreamer player is not initialized
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, GetBufferingLimitFailureDueToUninitializedPlayer)
{
    mainThreadWillEnqueueTaskAndWait();
    uint32_t resultBufferingLimit{};
    EXPECT_FALSE(m_mediaPipeline->getBufferingLimit(resultBufferingLimit));
}

/**
 * Test that GetBufferingLimit returns failure if the gstreamer API fails
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, GetBufferingLimitFailure)
{
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
    uint32_t resultBufferingLimit{};
    EXPECT_CALL(*m_gstPlayerMock, getBufferingLimit(_)).WillOnce(Return(false));
    EXPECT_FALSE(m_mediaPipeline->getBufferingLimit(resultBufferingLimit));
}

/**
 * Test that GetBufferingLimit returns success if the gstreamer API succeeds
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, GetBufferingLimitSuccess)
{
    constexpr uint32_t kBufferingLimit{1};
    uint32_t resultBufferingLimit{};
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, getBufferingLimit(_)).WillOnce(DoAll(SetArgReferee<0>(kBufferingLimit), Return(true)));

    EXPECT_TRUE(m_mediaPipeline->getBufferingLimit(resultBufferingLimit));
    EXPECT_EQ(resultBufferingLimit, kBufferingLimit);
}

/**
 * Test that SetUseBuffering returns failure if the gstreamer player is not initialized
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetUseBufferingFailureDueToUninitializedPlayer)
{
    mainThreadWillEnqueueTaskAndWait();
    constexpr int32_t kUseBuffering{1};
    EXPECT_FALSE(m_mediaPipeline->setUseBuffering(kUseBuffering));
}

/**
 * Test that SetUseBuffering returns success if the gstreamer API succeeds
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetUseBufferingSuccess)
{
    constexpr int32_t kUseBuffering{1};
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, setUseBuffering(kUseBuffering));
    EXPECT_TRUE(m_mediaPipeline->setUseBuffering(kUseBuffering));
}

/**
 * Test that GetUseBuffering returns failure if the gstreamer player is not initialized
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, GetUseBufferingFailureDueToUninitializedPlayer)
{
    mainThreadWillEnqueueTaskAndWait();
    bool resultUseBuffering{};
    EXPECT_FALSE(m_mediaPipeline->getUseBuffering(resultUseBuffering));
}

/**
 * Test that GetUseBuffering returns failure if the gstreamer API fails
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, GetUseBufferingFailure)
{
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
    bool resultUseBuffering{};
    EXPECT_CALL(*m_gstPlayerMock, getUseBuffering(_)).WillOnce(Return(false));
    EXPECT_FALSE(m_mediaPipeline->getUseBuffering(resultUseBuffering));
}

/**
 * Test that GetUseBuffering returns success if the gstreamer API succeeds
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, GetUseBufferingSuccess)
{
    constexpr bool kUseBuffering{true};
    bool resultUseBuffering{};
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, getUseBuffering(_)).WillOnce(DoAll(SetArgReferee<0>(kUseBuffering), Return(true)));

    EXPECT_TRUE(m_mediaPipeline->getUseBuffering(resultUseBuffering));
    EXPECT_EQ(resultUseBuffering, kUseBuffering);
}

/**
 * Test that IsVideoMaster returns false
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, IsVideoMasterFailureDueToUninitializedPlayer)
{
    bool resultIsVideoMaster{};
    EXPECT_FALSE(m_mediaPipeline->isVideoMaster(resultIsVideoMaster));
}

/**
 * Test that active requests are invalidated successfully
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, InvalidateActiveRequestsSuccess)
{
    constexpr auto kMediaSourceType{firebolt::rialto::MediaSourceType::AUDIO};
    loadGstPlayer();
    mainThreadWillEnqueueTask();

    EXPECT_CALL(*m_activeRequestsMock, erase(kMediaSourceType));
    m_mediaPipeline->invalidateActiveRequests(kMediaSourceType);
}

/**
 * Test that Ping checks GstPlayer worker thread
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, PingWithGstPlayerWorkerThreadCheck)
{
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();

    EXPECT_CALL(*m_gstPlayerMock, ping(_));
    m_mediaPipeline->ping(std::make_unique<StrictMock<firebolt::rialto::server::HeartbeatHandlerMock>>());
}

/**
 * Test that Ping checks main thread only
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SimplePing)
{
    mainThreadWillEnqueueTaskAndWait();

    m_mediaPipeline->ping(std::make_unique<StrictMock<firebolt::rialto::server::HeartbeatHandlerMock>>());
}
