/*
 * Copyright (C) 2023 Sky UK
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "Matchers.h"
#include "MediaPipelineCapabilitiesMock.h"
#include "RialtoGStreamerMSEBaseSinkPrivate.h"
#include "RialtoGStreamerMSEVideoSink.h"
#include "RialtoGstTest.h"

using firebolt::rialto::IMediaPipelineCapabilitiesFactory;
using firebolt::rialto::MediaPipelineCapabilitiesFactoryMock;
using firebolt::rialto::MediaPipelineCapabilitiesMock;
using testing::_;
using testing::DoAll;
using testing::Return;
using testing::SetArgReferee;
using testing::StrictMock;

namespace
{
constexpr bool kHasDrm{true};
constexpr int32_t kWidth{1920};
constexpr int32_t kHeight{1080};
constexpr bool kFrameStepOnPreroll{true};
constexpr int32_t kUnknownSourceId{-1};
const std::string kDefaultWindowSet{"0,0,1920,1080"};
const std::string kCustomWindowSet{"20,40,640,480"};
} // namespace

class GstreamerMseVideoSinkTests : public RialtoGstTest
{
public:
    GstreamerMseVideoSinkTests() = default;
    ~GstreamerMseVideoSinkTests() override = default;

    GstCaps *createDefaultCaps() const
    {
        return gst_caps_new_simple("video/x-h264", "width", G_TYPE_INT, kWidth, "height", G_TYPE_INT, kHeight, nullptr);
    }

    firebolt::rialto::IMediaPipeline::MediaSourceVideo createDefaultMediaSource() const
    {
        return firebolt::rialto::IMediaPipeline::MediaSourceVideo{"video/h264", kHasDrm, kWidth, kHeight};
    }
};

TEST_F(GstreamerMseVideoSinkTests, ShouldFailToReachPausedStateWhenMediaPipelineCantBeCreated)
{
    constexpr firebolt::rialto::VideoRequirements kDefaultRequirements{3840, 2160};
    RialtoMSEBaseSink *videoSink = createVideoSink();
    GstElement *pipeline = createPipelineWithSink(videoSink);

    EXPECT_CALL(*m_mediaPipelineFactoryMock, createMediaPipeline(_, kDefaultRequirements)).WillOnce(Return(nullptr));
    EXPECT_EQ(GST_STATE_CHANGE_FAILURE, gst_element_set_state(pipeline, GST_STATE_PAUSED));
    EXPECT_EQ(GST_STATE_CHANGE_SUCCESS, gst_element_set_state(pipeline, GST_STATE_NULL));

    gst_object_unref(pipeline);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldNotHandleUnknownEvent)
{
    RialtoMSEBaseSink *videoSink = createVideoSink();
    GstElement *pipeline = createPipelineWithSink(videoSink);

    EXPECT_EQ(GST_STATE_CHANGE_SUCCESS, gst_element_set_state(pipeline, GST_STATE_READY));

    gst_pad_set_active(videoSink->priv->m_sinkPad, TRUE);
    gst_pad_send_event(videoSink->priv->m_sinkPad, gst_event_new_gap(1, 1));

    EXPECT_EQ(GST_STATE_CHANGE_SUCCESS, gst_element_set_state(pipeline, GST_STATE_NULL));

    gst_object_unref(pipeline);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldNotAttachSourceWhenPipelineIsBelowPausedState)
{
    RialtoMSEBaseSink *videoSink = createVideoSink();
    GstElement *pipeline = createPipelineWithSink(videoSink);

    EXPECT_EQ(GST_STATE_CHANGE_SUCCESS, gst_element_set_state(pipeline, GST_STATE_READY));

    gst_pad_set_active(videoSink->priv->m_sinkPad, TRUE);
    GstCaps *caps{createDefaultCaps()};
    setCaps(videoSink, caps);

    EXPECT_EQ(GST_STATE_CHANGE_SUCCESS, gst_element_set_state(pipeline, GST_STATE_NULL));

    gst_caps_unref(caps);
    gst_object_unref(pipeline);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldAttachSourceWithH264)
{
    RialtoMSEBaseSink *videoSink = createVideoSink();
    GstElement *pipeline = createPipelineWithSink(videoSink);

    setPausedState(pipeline, videoSink);
    const int32_t kSourceId{videoSourceWillBeAttached(createDefaultMediaSource())};
    allSourcesWillBeAttached();

    GstCaps *caps{createDefaultCaps()};
    setCaps(videoSink, caps);

    setNullState(pipeline, kSourceId);

    gst_caps_unref(caps);
    gst_object_unref(pipeline);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldSetQueuedImmediateOutput)
{
    RialtoMSEBaseSink *videoSink = createVideoSink();

    // Queue an immediate-output request
    EXPECT_CALL(m_mediaPipelineMock, setImmediateOutput(_, true)).WillOnce(Return(true));
    g_object_set(videoSink, "immediate-output", TRUE, nullptr);

    GstElement *pipeline = createPipelineWithSink(videoSink);

    setPausedState(pipeline, videoSink);
    const int32_t kSourceId{videoSourceWillBeAttached(createDefaultMediaSource())};
    allSourcesWillBeAttached();

    GstCaps *caps{createDefaultCaps()};
    setCaps(videoSink, caps);

    setNullState(pipeline, kSourceId);

    gst_caps_unref(caps);
    gst_object_unref(pipeline);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldSetImmediateOutputProperty)
{
    RialtoMSEBaseSink *videoSink = createVideoSink();
    GstElement *pipeline = createPipelineWithSink(videoSink);

    setPausedState(pipeline, videoSink);
    const int32_t kSourceId{videoSourceWillBeAttached(createVideoMediaSource())};
    allSourcesWillBeAttached();

    GstCaps *caps{createVideoCaps()};
    setCaps(videoSink, caps);

    EXPECT_CALL(m_mediaPipelineMock, setImmediateOutput(_, _)).WillOnce(Return(true));
    g_object_set(videoSink, "immediate-output", TRUE, nullptr);

    setNullState(pipeline, kSourceId);
    gst_caps_unref(caps);
    gst_object_unref(pipeline);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldFailToSetImmediateOutputPropertyDueToPipelinedFailure)
{
    RialtoMSEBaseSink *videoSink = createVideoSink();
    GstElement *pipeline = createPipelineWithSink(videoSink);

    setPausedState(pipeline, videoSink);
    const int32_t kSourceId{videoSourceWillBeAttached(createVideoMediaSource())};
    allSourcesWillBeAttached();

    GstCaps *caps{createVideoCaps()};
    setCaps(videoSink, caps);

    EXPECT_CALL(m_mediaPipelineMock, setImmediateOutput(_, _)).WillOnce(Return(false));
    g_object_set(videoSink, "immediate-output", TRUE, nullptr);

    setNullState(pipeline, kSourceId);
    gst_caps_unref(caps);
    gst_object_unref(pipeline);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldFailToSetImmediateOutputProperty)
{
    RialtoMSEBaseSink *videoSink = createVideoSink();

    // No pipeline therefore the m_mediaPipelineMock method setImmediateOutput() will not be called
    g_object_set(videoSink, "immediate-output", TRUE, nullptr);

    gst_element_set_state(GST_ELEMENT_CAST(videoSink), GST_STATE_NULL);
    gst_object_unref(videoSink);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldGetImmediateOutputProperty)
{
    RialtoMSEBaseSink *videoSink = createVideoSink();
    GstElement *pipeline = createPipelineWithSink(videoSink);

    setPausedState(pipeline, videoSink);
    const int32_t kSourceId{videoSourceWillBeAttached(createVideoMediaSource())};
    allSourcesWillBeAttached();

    GstCaps *caps{createVideoCaps()};
    setCaps(videoSink, caps);

    EXPECT_CALL(m_mediaPipelineMock, getImmediateOutput(_, _)).WillOnce(DoAll(SetArgReferee<1>(true), Return(true)));
    gboolean immediateOutput;
    g_object_get(videoSink, "immediate-output", &immediateOutput, nullptr);
    EXPECT_TRUE(immediateOutput);

    setNullState(pipeline, kSourceId);
    gst_caps_unref(caps);
    gst_object_unref(pipeline);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldFailToGetImmediateOutputPropertyDueToPipelinedFailure)
{
    RialtoMSEBaseSink *videoSink = createVideoSink();
    GstElement *pipeline = createPipelineWithSink(videoSink);

    setPausedState(pipeline, videoSink);
    const int32_t kSourceId{videoSourceWillBeAttached(createVideoMediaSource())};
    allSourcesWillBeAttached();

    GstCaps *caps{createVideoCaps()};
    setCaps(videoSink, caps);

    EXPECT_CALL(m_mediaPipelineMock, getImmediateOutput(_, _)).WillOnce(Return(false));
    gboolean immediateOutput{true};
    g_object_get(videoSink, "immediate-output", &immediateOutput, nullptr);
    EXPECT_FALSE(immediateOutput); // The return value for failure

    setNullState(pipeline, kSourceId);
    gst_caps_unref(caps);
    gst_object_unref(pipeline);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldFailToGetImmediateOutputProperty)
{
    RialtoMSEBaseSink *videoSink = createVideoSink();

    // No pipeline therefore the m_mediaPipelineMock method getImmediateOutput() will not be called
    gboolean immediateOutput;
    g_object_get(videoSink, "immediate-output", &immediateOutput, nullptr);

    gst_element_set_state(GST_ELEMENT_CAST(videoSink), GST_STATE_NULL);
    gst_object_unref(videoSink);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldGetVideoPtsProperty)
{
    constexpr gint64 kPosition{2500000000};
    constexpr double kVolume{1.0};
    constexpr gint64 kExpectedVideoPts{225000};

    RialtoMSEBaseSink *videoSink = createVideoSink();
    GstElement *pipeline = createPipelineWithSink(videoSink);

    setPausedState(pipeline, videoSink);
    const int32_t kSourceId{videoSourceWillBeAttached(createVideoMediaSource())};
    allSourcesWillBeAttached();

    GstCaps *caps{createVideoCaps()};
    setCaps(videoSink, caps);

    sendPlaybackInfoNotification(videoSink, firebolt::rialto::PlaybackInfo{kPosition, kVolume});
    gint64 videoPts{0};
    g_object_get(videoSink, "video-pts", &videoPts, nullptr);
    EXPECT_EQ(videoPts, kExpectedVideoPts);

    setNullState(pipeline, kSourceId);
    gst_caps_unref(caps);
    gst_object_unref(pipeline);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldFailToGetVideoPtsProperty)
{
    RialtoMSEBaseSink *videoSink = createVideoSink();

    gint64 videoPts{0};
    g_object_get(videoSink, "video-pts", &videoPts, nullptr);

    gst_element_set_state(GST_ELEMENT_CAST(videoSink), GST_STATE_NULL);
    gst_object_unref(videoSink);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldFailToSetStreamSyncModePropertyOnRialtoFailure)
{
    TestContext textContext = createPipelineWithVideoSinkAndSetToPaused();

    constexpr gboolean kSyncModeStreaming{TRUE};
    EXPECT_CALL(m_mediaPipelineMock, setStreamSyncMode(textContext.m_sourceId, kSyncModeStreaming)).WillOnce(Return(false));
    g_object_set(textContext.m_sink, "syncmode-streaming", kSyncModeStreaming, nullptr);

    // Error is logged

    setNullState(textContext.m_pipeline, textContext.m_sourceId);
    gst_object_unref(textContext.m_pipeline);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldSetStreamSyncMode)
{
    TestContext textContext = createPipelineWithVideoSinkAndSetToPaused();

    constexpr gboolean kSyncModeStreaming{TRUE};
    EXPECT_CALL(m_mediaPipelineMock, setStreamSyncMode(textContext.m_sourceId, kSyncModeStreaming)).WillOnce(Return(true));
    g_object_set(textContext.m_sink, "syncmode-streaming", kSyncModeStreaming, nullptr);

    setNullState(textContext.m_pipeline, textContext.m_sourceId);
    gst_object_unref(textContext.m_pipeline);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldSetCachedStreamSyncMode)
{
    RialtoMSEBaseSink *videoSink = createVideoSink();
    GstElement *pipeline = createPipelineWithSink(videoSink);

    constexpr gboolean kSyncModeStreaming{TRUE};
    g_object_set(videoSink, "syncmode-streaming", kSyncModeStreaming, nullptr);

    setPausedState(pipeline, videoSink);
    const int32_t kSourceId{videoSourceWillBeAttached(createVideoMediaSource())};
    allSourcesWillBeAttached();

    EXPECT_CALL(m_mediaPipelineMock, setStreamSyncMode(kSourceId, kSyncModeStreaming)).WillOnce(Return(true));

    GstCaps *caps{createVideoCaps()};
    setCaps(videoSink, caps);
    gst_caps_unref(caps);

    setNullState(pipeline, kSourceId);

    gst_object_unref(pipeline);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldNotSetCachedStreamSyncModeOnRialtoFailure)
{
    RialtoMSEBaseSink *videoSink = createVideoSink();
    GstElement *pipeline = createPipelineWithSink(videoSink);

    constexpr gboolean kSyncModeStreaming{TRUE};
    g_object_set(videoSink, "syncmode-streaming", kSyncModeStreaming, nullptr);

    setPausedState(pipeline, videoSink);
    const int32_t kSourceId{videoSourceWillBeAttached(createVideoMediaSource())};
    allSourcesWillBeAttached();

    EXPECT_CALL(m_mediaPipelineMock, setStreamSyncMode(kSourceId, kSyncModeStreaming)).WillOnce(Return(false));

    GstCaps *caps{createVideoCaps()};
    setCaps(videoSink, caps);
    gst_caps_unref(caps);

    // Error is logged

    setNullState(pipeline, kSourceId);

    gst_object_unref(pipeline);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldSetShowVideoWindow)
{
    TestContext textContext = createPipelineWithVideoSinkAndSetToPaused();

    constexpr gboolean kShowVideoWindow{FALSE};
    EXPECT_CALL(m_mediaPipelineMock, setMute(textContext.m_sourceId, !(static_cast<bool>(kShowVideoWindow))))
        .WillOnce(Return(true));
    g_object_set(textContext.m_sink, "show-video-window", kShowVideoWindow, nullptr);

    setNullState(textContext.m_pipeline, textContext.m_sourceId);
    gst_object_unref(textContext.m_pipeline);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldSetCachedShowVideoWindow)
{
    RialtoMSEBaseSink *videoSink = createVideoSink();
    GstElement *pipeline = createPipelineWithSink(videoSink);

    constexpr gboolean kShowVideoWindow{TRUE};
    g_object_set(videoSink, "show-video-window", kShowVideoWindow, nullptr);

    setPausedState(pipeline, videoSink);
    const int32_t kSourceId{videoSourceWillBeAttached(createVideoMediaSource())};
    allSourcesWillBeAttached();

    EXPECT_CALL(m_mediaPipelineMock, setMute(kSourceId, !(static_cast<bool>(kShowVideoWindow)))).WillOnce(Return(true));

    GstCaps *caps{createVideoCaps()};
    setCaps(videoSink, caps);
    gst_caps_unref(caps);

    setNullState(pipeline, kSourceId);

    gst_object_unref(pipeline);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldNotAttachSourceTwice)
{
    RialtoMSEBaseSink *videoSink = createVideoSink();
    GstElement *pipeline = createPipelineWithSink(videoSink);

    setPausedState(pipeline, videoSink);
    const int32_t kSourceId{videoSourceWillBeAttached(createDefaultMediaSource())};
    allSourcesWillBeAttached();

    GstCaps *caps{createDefaultCaps()};
    setCaps(videoSink, caps);
    setCaps(videoSink, caps);

    setNullState(pipeline, kSourceId);

    gst_caps_unref(caps);
    gst_object_unref(pipeline);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldAttachSourceWithVp9)
{
    RialtoMSEBaseSink *videoSink = createVideoSink();
    GstElement *pipeline = createPipelineWithSink(videoSink);

    setPausedState(pipeline, videoSink);
    const int32_t kSourceId{videoSourceWillBeAttached(
        firebolt::rialto::IMediaPipeline::MediaSourceVideo{"video/x-vp9", kHasDrm, kWidth, kHeight})};
    allSourcesWillBeAttached();

    GstCaps *caps{gst_caps_new_simple("video/x-vp9", "width", G_TYPE_INT, kWidth, "height", G_TYPE_INT, kHeight, nullptr)};
    setCaps(videoSink, caps);

    setNullState(pipeline, kSourceId);

    gst_caps_unref(caps);
    gst_object_unref(pipeline);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldAttachSourceWithH265)
{
    RialtoMSEBaseSink *videoSink = createVideoSink();
    GstElement *pipeline = createPipelineWithSink(videoSink);

    setPausedState(pipeline, videoSink);
    const int32_t kSourceId{videoSourceWillBeAttached(
        firebolt::rialto::IMediaPipeline::MediaSourceVideo{"video/h265", kHasDrm, kWidth, kHeight})};
    allSourcesWillBeAttached();

    GstCaps *caps{
        gst_caps_new_simple("video/x-h265", "width", G_TYPE_INT, kWidth, "height", G_TYPE_INT, kHeight, nullptr)};
    setCaps(videoSink, caps);

    setNullState(pipeline, kSourceId);

    gst_caps_unref(caps);
    gst_object_unref(pipeline);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldAttachSourceWithDolbyVision)
{
    constexpr unsigned kDvProfile{123};
    RialtoMSEBaseSink *videoSink = createVideoSink();
    GstElement *pipeline = createPipelineWithSink(videoSink);

    setPausedState(pipeline, videoSink);
    const int32_t kSourceId{dolbyVisionSourceWillBeAttached(
        firebolt::rialto::IMediaPipeline::MediaSourceVideoDolbyVision{"video/h265", kDvProfile, kHasDrm, kWidth,
                                                                      kHeight})};
    allSourcesWillBeAttached();

    GstCaps *caps{gst_caps_new_simple("video/x-h265", "width", G_TYPE_INT, kWidth, "height", G_TYPE_INT, kHeight,
                                      "dovi-stream", G_TYPE_BOOLEAN, TRUE, "dv_profile", G_TYPE_UINT, kDvProfile,
                                      nullptr)};
    setCaps(videoSink, caps);

    setNullState(pipeline, kSourceId);

    gst_caps_unref(caps);
    gst_object_unref(pipeline);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldReachPausedState)
{
    RialtoMSEBaseSink *videoSink = createVideoSink();
    GstElement *pipeline = createPipelineWithSink(videoSink);

    setPausedState(pipeline, videoSink);
    const int32_t kSourceId{videoSourceWillBeAttached(createDefaultMediaSource())};
    allSourcesWillBeAttached();

    GstCaps *caps{createDefaultCaps()};
    setCaps(videoSink, caps);

    sendPlaybackStateNotification(videoSink, firebolt::rialto::PlaybackState::PAUSED);

    EXPECT_TRUE(waitForMessage(pipeline, GST_MESSAGE_ASYNC_DONE));

    setNullState(pipeline, kSourceId);

    gst_caps_unref(caps);
    gst_object_unref(pipeline);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldFailToGetRectanglePropertyWhenPipelineIsBelowPausedState)
{
    RialtoMSEBaseSink *videoSink = createVideoSink();

    gchar *rectangle{nullptr};
    g_object_get(videoSink, "rectangle", rectangle, nullptr);
    EXPECT_FALSE(rectangle);

    gst_element_set_state(GST_ELEMENT_CAST(videoSink), GST_STATE_NULL);
    gst_object_unref(videoSink);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldGetRectangleProperty)
{
    TestContext textContext = createPipelineWithVideoSinkAndSetToPaused();

    gchar *rectangle{nullptr};
    g_object_get(textContext.m_sink, "rectangle", &rectangle, nullptr);
    ASSERT_TRUE(rectangle);
    EXPECT_EQ(std::string(rectangle), kDefaultWindowSet);

    g_free(rectangle);
    setNullState(textContext.m_pipeline, textContext.m_sourceId);
    gst_object_unref(textContext.m_pipeline);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldSetAndGetMaxVideoWidthProperty)
{
    RialtoMSEBaseSink *videoSink = createVideoSink();

    g_object_set(videoSink, "max-video-width", kWidth, nullptr);

    unsigned maxVideoWidth{0};
    g_object_get(videoSink, "max-video-width", &maxVideoWidth, nullptr);
    EXPECT_EQ(kWidth, maxVideoWidth);

    gst_element_set_state(GST_ELEMENT_CAST(videoSink), GST_STATE_NULL);
    gst_object_unref(videoSink);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldSetAndGetMaxVideoHeightProperty)
{
    RialtoMSEBaseSink *videoSink = createVideoSink();

    g_object_set(videoSink, "max-video-height", kHeight, nullptr);

    unsigned maxVideoHeight{0};
    g_object_get(videoSink, "max-video-height", &maxVideoHeight, nullptr);
    EXPECT_EQ(maxVideoHeight, kHeight);

    gst_element_set_state(GST_ELEMENT_CAST(videoSink), GST_STATE_NULL);
    gst_object_unref(videoSink);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldSetAndGetMaxVideoWidthPropertyDeprecated)
{
    RialtoMSEBaseSink *videoSink = createVideoSink();

    g_object_set(videoSink, "maxVideoWidth", kWidth, nullptr);

    unsigned maxVideoWidth{0};
    g_object_get(videoSink, "maxVideoWidth", &maxVideoWidth, nullptr);
    EXPECT_EQ(kWidth, maxVideoWidth);

    gst_element_set_state(GST_ELEMENT_CAST(videoSink), GST_STATE_NULL);
    gst_object_unref(videoSink);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldSetAndGetMaxVideoHeightPropertyDeprecated)
{
    RialtoMSEBaseSink *videoSink = createVideoSink();

    g_object_set(videoSink, "maxVideoHeight", kHeight, nullptr);

    unsigned maxVideoHeight{0};
    g_object_get(videoSink, "maxVideoHeight", &maxVideoHeight, nullptr);
    EXPECT_EQ(maxVideoHeight, kHeight);

    gst_element_set_state(GST_ELEMENT_CAST(videoSink), GST_STATE_NULL);
    gst_object_unref(videoSink);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldSetAndGetFrameStepOnPrerollProperty)
{
    RialtoMSEBaseSink *videoSink = createVideoSink();

    g_object_set(videoSink, "frame-step-on-preroll", kFrameStepOnPreroll, nullptr);

    bool frameStepOnPreroll{false};
    g_object_get(videoSink, "frame-step-on-preroll", &frameStepOnPreroll, nullptr);
    EXPECT_EQ(frameStepOnPreroll, kFrameStepOnPreroll);

    gst_element_set_state(GST_ELEMENT_CAST(videoSink), GST_STATE_NULL);
    gst_object_unref(videoSink);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldFailToSetRectanglePropertyWhenPipelineIsBelowPausedState)
{
    RialtoMSEBaseSink *videoSink = createVideoSink();

    g_object_set(videoSink, "rectangle", kCustomWindowSet.c_str(), nullptr);

    // Sink should return cached value
    gchar *rectangle{nullptr};
    g_object_get(videoSink, "rectangle", &rectangle, nullptr);
    ASSERT_TRUE(rectangle);
    EXPECT_EQ(std::string(rectangle), kCustomWindowSet);
    g_free(rectangle);

    gst_element_set_state(GST_ELEMENT_CAST(videoSink), GST_STATE_NULL);
    gst_object_unref(videoSink);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldFailToSetRectanglePropertyWhenStringIsNotValid)
{
    RialtoMSEBaseSink *videoSink = createVideoSink();

    g_object_set(videoSink, "rectangle", nullptr, nullptr);

    gst_element_set_state(GST_ELEMENT_CAST(videoSink), GST_STATE_NULL);
    gst_object_unref(videoSink);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldSetRectangleProperty)
{
    TestContext textContext = createPipelineWithVideoSinkAndSetToPaused();

    EXPECT_CALL(m_mediaPipelineMock, setVideoWindow(20, 40, 640, 480)).WillOnce(Return(true));
    g_object_set(textContext.m_sink, "rectangle", kCustomWindowSet.c_str(), nullptr);

    gchar *rectangle{nullptr};
    g_object_get(textContext.m_sink, "rectangle", &rectangle, nullptr);
    ASSERT_TRUE(rectangle);
    EXPECT_EQ(std::string(rectangle), kCustomWindowSet);

    g_free(rectangle);
    setNullState(textContext.m_pipeline, textContext.m_sourceId);
    gst_object_unref(textContext.m_pipeline);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldSetQueuedRectangleProperty)
{
    RialtoMSEBaseSink *videoSink = createVideoSink();
    GstElement *pipeline = createPipelineWithSink(videoSink);

    g_object_set(videoSink, "rectangle", kCustomWindowSet.c_str(), nullptr);

    EXPECT_CALL(m_mediaPipelineMock, setVideoWindow(20, 40, 640, 480)).WillOnce(Return(true));
    load(pipeline);
    EXPECT_EQ(GST_STATE_CHANGE_ASYNC, gst_element_set_state(pipeline, GST_STATE_PAUSED));

    gchar *rectangle{nullptr};
    g_object_get(videoSink, "rectangle", &rectangle, nullptr);
    ASSERT_TRUE(rectangle);
    EXPECT_EQ(std::string(rectangle), kCustomWindowSet);

    g_free(rectangle);
    setNullState(pipeline, kUnknownSourceId);
    gst_object_unref(pipeline);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldFailToSetFrameStepOnPrerollPropertyWhenPipelineIsBelowPausedState)
{
    RialtoMSEBaseSink *videoSink = createVideoSink();

    g_object_set(videoSink, "frame-step-on-preroll", kFrameStepOnPreroll, nullptr);

    gst_element_set_state(GST_ELEMENT_CAST(videoSink), GST_STATE_NULL);
    gst_object_unref(videoSink);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldSetFrameStepOnPrerollProperty)
{
    TestContext textContext = createPipelineWithVideoSinkAndSetToPaused();

    EXPECT_CALL(m_mediaPipelineMock, renderFrame()).WillOnce(Return(true));
    g_object_set(textContext.m_sink, "frame-step-on-preroll", kFrameStepOnPreroll, nullptr);

    setNullState(textContext.m_pipeline, textContext.m_sourceId);
    gst_object_unref(textContext.m_pipeline);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldNotRenderFrameTwice)
{
    TestContext textContext = createPipelineWithVideoSinkAndSetToPaused();

    EXPECT_CALL(m_mediaPipelineMock, renderFrame()).WillOnce(Return(true));
    g_object_set(textContext.m_sink, "frame-step-on-preroll", kFrameStepOnPreroll, nullptr);
    g_object_set(textContext.m_sink, "frame-step-on-preroll", kFrameStepOnPreroll, nullptr);

    setNullState(textContext.m_pipeline, textContext.m_sourceId);
    gst_object_unref(textContext.m_pipeline);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldGetIsMasterPropertyFromMediaPipelineCapabilities)
{
    constexpr bool kIsMaster{false};
    TestContext textContext = createPipelineWithVideoSinkAndSetToPaused();

    std::unique_ptr<StrictMock<MediaPipelineCapabilitiesMock>> capabilitiesMock{
        std::make_unique<StrictMock<MediaPipelineCapabilitiesMock>>()};
    EXPECT_CALL(*capabilitiesMock, isVideoMaster(_)).WillOnce(DoAll(SetArgReferee<0>(kIsMaster), Return(true)));
    std::shared_ptr<StrictMock<MediaPipelineCapabilitiesFactoryMock>> capabilitiesFactoryMock{
        std::dynamic_pointer_cast<StrictMock<MediaPipelineCapabilitiesFactoryMock>>(
            IMediaPipelineCapabilitiesFactory::createFactory())};
    ASSERT_TRUE(capabilitiesFactoryMock);
    EXPECT_CALL(*capabilitiesFactoryMock, createMediaPipelineCapabilities())
        .WillOnce(Return(ByMove(std::move(capabilitiesMock))));
    gboolean isMaster{TRUE};
    g_object_get(textContext.m_sink, "is-master", &isMaster, nullptr);
    EXPECT_EQ(static_cast<bool>(isMaster), kIsMaster);

    setNullState(textContext.m_pipeline, textContext.m_sourceId);
    gst_object_unref(textContext.m_pipeline);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldFailToGetOrSetUnknownProperty)
{
    RialtoMSEBaseSink *videoSink = createVideoSink();

    g_object_class_install_property(G_OBJECT_GET_CLASS(videoSink), 123,
                                    g_param_spec_boolean("surprise", "surprise", "surprise", FALSE,
                                                         GParamFlags(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

    gboolean value{FALSE};
    g_object_get(videoSink, "surprise", &value, nullptr);
    EXPECT_FALSE(value);

    constexpr gboolean kValue{FALSE};
    g_object_set(videoSink, "surprise", kValue, nullptr);

    gst_element_set_state(GST_ELEMENT_CAST(videoSink), GST_STATE_NULL);
    gst_object_unref(videoSink);
}

TEST_F(GstreamerMseVideoSinkTests, ShouldSendQosEvent)
{
    RialtoMSEBaseSink *videoSink = createVideoSink();
    GstElement *pipeline = createPipelineWithSink(videoSink);

    setPausedState(pipeline, videoSink);
    const int32_t kSourceId{videoSourceWillBeAttached(createDefaultMediaSource())};
    allSourcesWillBeAttached();

    GstCaps *caps{createDefaultCaps()};
    setCaps(videoSink, caps);

    sendPlaybackStateNotification(videoSink, firebolt::rialto::PlaybackState::PAUSED);

    auto mediaPlayerClient{m_mediaPipelineClient.lock()};
    ASSERT_TRUE(mediaPlayerClient);
    const firebolt::rialto::QosInfo kQosInfo{1, 2};
    mediaPlayerClient->notifyQos(kSourceId, kQosInfo);

    EXPECT_TRUE(waitForMessage(pipeline, GST_MESSAGE_QOS));

    setNullState(pipeline, kSourceId);

    gst_caps_unref(caps);
    gst_object_unref(pipeline);
}
