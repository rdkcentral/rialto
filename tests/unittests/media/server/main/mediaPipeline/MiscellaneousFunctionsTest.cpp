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

class RialtoServerMediaPipelineMiscellaneousFunctionsTest : public MediaPipelineTestBase
{
protected:
    const int64_t m_kPosition{4028596027};
    const double m_kPlaybackRate{1.5};

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
    mainThreadWillEnqueueTaskAndWait();
    int64_t targetPosition{};
    EXPECT_FALSE(m_mediaPipeline->getPosition(targetPosition));
}

/**
 * Test that GetPosition returns failure if the gstreamer API fails
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, GetPositionFailure)
{
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
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
    mainThreadWillEnqueueTaskAndWait();
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

    EXPECT_CALL(*m_gstPlayerMock, setVolume(kVolume));
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
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_FALSE(m_mediaPipeline->setMute(kMute));
}

/**
 * Test that SetMute returns failure if the gstreamer player is not initialized
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, SetMuteSuccess)
{
    const bool kMute{false};
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();

    EXPECT_CALL(*m_gstPlayerMock, setMute(kMute));
    EXPECT_TRUE(m_mediaPipeline->setMute(kMute));
}

/**
 * Test that GetMute returns failure if the gstreamer player is not initialized
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, GetMuteFailureDueToUninitializedPlayer)
{
    mainThreadWillEnqueueTaskAndWait();
    bool resultMute{};
    EXPECT_FALSE(m_mediaPipeline->getMute(resultMute));
}

/**
 * Test that GetMute returns failure if the gstreamer API fails
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, GetMuteFailure)
{
    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
    bool resultMute{};
    EXPECT_CALL(*m_gstPlayerMock, getMute(_)).WillOnce(Return(false));
    EXPECT_FALSE(m_mediaPipeline->getMute(resultMute));
}

/**
 * Test that GetMute returns success if the gstreamer API succeeds and gets mute
 */
TEST_F(RialtoServerMediaPipelineMiscellaneousFunctionsTest, GetMuteSuccess)
{
    constexpr bool kCurrentMute{false};
    bool resultMute{};

    loadGstPlayer();
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, getMute(_))
        .WillOnce(Invoke(
            [&](bool &mut)
            {
                mut = kCurrentMute;
                return true;
            }));

    EXPECT_TRUE(m_mediaPipeline->getMute(resultMute));
    EXPECT_EQ(resultMute, kCurrentMute);
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
