/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Sky UK
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

#include "TasksTestsBase.h"
#include "IMediaPipeline.h"
#include "Matchers.h"
#include "TasksTestsContext.h"
#include "tasks/generic/AttachSamples.h"
#include "tasks/generic/AttachSource.h"
#include "tasks/generic/CheckAudioUnderflow.h"
#include "tasks/generic/DeepElementAdded.h"
#include "tasks/generic/EnoughData.h"
#include "tasks/generic/Eos.h"
#include "tasks/generic/FinishSetupSource.h"
#include "tasks/generic/NeedData.h"
#include "tasks/generic/Pause.h"
#include "tasks/generic/Play.h"
#include "tasks/generic/ReadShmDataAndAttachSamples.h"
#include "tasks/generic/RemoveSource.h"
#include "tasks/generic/RenderFrame.h"
#include "tasks/generic/ReportPosition.h"
#include "tasks/generic/SetMute.h"
#include "tasks/generic/SetPlaybackRate.h"
#include "tasks/generic/SetPosition.h"
#include "tasks/generic/SetVideoGeometry.h"
#include "tasks/generic/SetVolume.h"
#include "tasks/generic/SetupElement.h"
#include "tasks/generic/SetupSource.h"
#include "tasks/generic/Shutdown.h"
#include "tasks/generic/Stop.h"
#include "tasks/generic/Underflow.h"
#include "tasks/generic/UpdatePlaybackGroup.h"
#include <gst/gst.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;

namespace
{
std::shared_ptr<TasksTestsContext> testContext;

constexpr firebolt::rialto::server::Rectangle kRectangle{1, 2, 3, 4};
constexpr double kVolume{0.7};
constexpr gulong kSignalId{123};
constexpr auto kAudioSourceId{static_cast<std::int32_t>(firebolt::rialto::MediaSourceType::AUDIO)};
constexpr auto kVideoSourceId{static_cast<std::int32_t>(firebolt::rialto::MediaSourceType::VIDEO)};
constexpr gint64 kItHappenedInThePast = 1238450934;
constexpr gint64 kItWillHappenInTheFuture = 3823530248;
constexpr int64_t kDuration{9000000000};
constexpr int32_t kSampleRate{13};
constexpr int32_t kNumberOfChannels{4};
constexpr int32_t kWidth{1024};
constexpr int32_t kHeight{768};
constexpr firebolt::rialto::Fraction kFrameRate{15, 1};
constexpr int32_t kDolbyVisionProfile{5};
constexpr gint64 kPosition{1234};
constexpr gint64 kPositionOverUnderflowMargin{350 * 1000000 + 1};
constexpr bool kMute{false};
constexpr double kRate{1.5};
constexpr guint kDataLength{7};
constexpr guint64 kOffset{123};
const std::shared_ptr<firebolt::rialto::CodecData> kEmptyCodecData{std::make_shared<firebolt::rialto::CodecData>()};
const std::vector<uint8_t> kCodecBufferVector{1, 2, 3, 4};
const std::shared_ptr<firebolt::rialto::CodecData> kCodecDataBuffer{std::make_shared<firebolt::rialto::CodecData>(
    firebolt::rialto::CodecData{kCodecBufferVector, firebolt::rialto::CodecDataType::BUFFER})};
const std::string kCodecStr{"TEST"};
const std::shared_ptr<firebolt::rialto::CodecData> kCodecDataStr{std::make_shared<firebolt::rialto::CodecData>(
    firebolt::rialto::CodecData{{kCodecStr.begin(), kCodecStr.end()}, firebolt::rialto::CodecDataType::STRING})};
const std::string kVidName{"vidsrc"};
const std::string kAudName{"audsrc"};

firebolt::rialto::IMediaPipeline::MediaSegmentVector buildAudioSamples()
{
    firebolt::rialto::IMediaPipeline::MediaSegmentVector dataVec;
    dataVec.emplace_back(
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSegmentAudio>(kAudioSourceId, kItHappenedInThePast,
                                                                              kDuration, kSampleRate, kNumberOfChannels));
    dataVec.emplace_back(
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSegmentAudio>(kAudioSourceId, kItWillHappenInTheFuture,
                                                                              kDuration, kSampleRate, kNumberOfChannels));
    dataVec.back()->setCodecData(kCodecDataBuffer);
    return dataVec;
}

firebolt::rialto::IMediaPipeline::MediaSegmentVector buildVideoSamples()
{
    firebolt::rialto::IMediaPipeline::MediaSegmentVector dataVec;
    dataVec.emplace_back(
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSegmentVideo>(kVideoSourceId, kItHappenedInThePast,
                                                                              kDuration, kWidth, kHeight, kFrameRate));
    dataVec.emplace_back(
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSegmentVideo>(kVideoSourceId, kItWillHappenInTheFuture,
                                                                              kDuration, kWidth, kHeight, kFrameRate));
    dataVec.back()->setCodecData(kCodecDataBuffer);
    return dataVec;
}
} // namespace

TasksTestsBase::TasksTestsBase()
{
    testContext = std::make_shared<TasksTestsContext>();

    gst_init(nullptr, nullptr);

    testContext->m_elementFactory = gst_element_factory_find("fakesrc");
    testContext->m_context.pipeline = &testContext->m_pipeline;
    testContext->m_context.audioAppSrc = &testContext->m_appSrcAudio;
    testContext->m_context.gstSrc = testContext->m_gstSrc;
    testContext->m_context.source = &testContext->m_element;
    testContext->m_context.decryptionService = testContext->m_decryptionServiceMock.get();
}

TasksTestsBase::~TasksTestsBase()
{
    gst_object_unref(testContext->m_elementFactory);

    testContext.reset();
}

void TasksTestsBase::setContextStreamInfo(firebolt::rialto::MediaSourceType sourceType)
{
    firebolt::rialto::server::StreamInfo &streamInfo = (sourceType == firebolt::rialto::MediaSourceType::AUDIO
                                                            ? testContext->m_streamInfoAudio
                                                            : testContext->m_streamInfoVideo);
    testContext->m_context.streamInfo.emplace(sourceType, streamInfo);
}

void TasksTestsBase::setContextPlaying()
{
    testContext->m_context.isPlaying = true;
}

void TasksTestsBase::setContextNeedData(bool doNeedData)
{
    testContext->m_context.audioNeedData = doNeedData;
    testContext->m_context.videoNeedData = doNeedData;
}

void TasksTestsBase::setContextAudioUnderflowOccured(bool isUnderflow)
{
    testContext->m_context.audioUnderflowOccured = isUnderflow;
}

void TasksTestsBase::setContextVideoUnderflowOccured(bool isUnderflow)
{
    testContext->m_context.videoUnderflowOccured = isUnderflow;
}

void TasksTestsBase::setContextAudioAppSrc()
{
    testContext->m_context.audioAppSrc = &testContext->m_appSrcAudio;
}

void TasksTestsBase::setContextEndOfStream(firebolt::rialto::MediaSourceType sourceType)
{
    GstElement *appSrc = (sourceType == firebolt::rialto::MediaSourceType::AUDIO ? &testContext->m_appSrcAudio
                                                                                 : &testContext->m_appSrcVideo);
    testContext->m_context.endOfStreamInfo.emplace(sourceType, appSrc);
}

void TasksTestsBase::setContextEndOfStreamNotified()
{
    testContext->m_context.eosNotified = true;
}

void TasksTestsBase::setContextPipelineNull()
{
    testContext->m_context.pipeline = nullptr;
}

void TasksTestsBase::setContextNeedDataPending(bool isNeedDataPending)
{
    testContext->m_context.audioNeedDataPending = isNeedDataPending;
    testContext->m_context.videoNeedDataPending = isNeedDataPending;
}

void TasksTestsBase::setContextNeedDataPendingAudioOnly(bool isNeedDataPending)
{
    testContext->m_context.audioNeedDataPending = isNeedDataPending;
}

void TasksTestsBase::setContextNeedDataPendingVideoOnly(bool isNeedDataPending)
{
    testContext->m_context.videoNeedDataPending = isNeedDataPending;
}

void TasksTestsBase::setContextAudioBuffer()
{
    testContext->m_context.audioBuffers.emplace_back(&testContext->m_audioBuffer);
}

void TasksTestsBase::setContextVideoBuffer()
{
    testContext->m_context.videoBuffers.emplace_back(&testContext->m_videoBuffer);
}

void TasksTestsBase::setContextPlaybackRate()
{
    testContext->m_context.playbackRate = kRate;
}

void TasksTestsBase::setContextSourceNull()
{
    testContext->m_context.source = nullptr;
}

void TasksTestsBase::setContextAudioSourceRemoved()
{
    testContext->m_context.audioSourceRemoved = true;
}

void TasksTestsBase::setContextStreamInfoEmpty()
{
    testContext->m_context.streamInfo.clear();
}

void TasksTestsBase::setContextNeedDataAudioOnly()
{
    testContext->m_context.audioNeedData = true;
}

void TasksTestsBase::expectSetupVideoElement()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetFactory(_)).WillOnce(Return(testContext->m_elementFactory));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstElementFactoryListIsType(testContext->m_elementFactory, GST_ELEMENT_FACTORY_TYPE_DECODER))
        .WillOnce(Return(TRUE));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstElementFactoryListIsType(testContext->m_elementFactory, GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO))
        .WillOnce(Return(TRUE));
    EXPECT_CALL(*testContext->m_glibWrapper, gObjectType(&testContext->m_element)).WillRepeatedly(Return(G_TYPE_PARAM));
    EXPECT_CALL(*testContext->m_glibWrapper, gSignalListIds(_, _))
        .WillOnce(Invoke(
            [&](GType itype, guint *n_ids)
            {
                *n_ids = 1;
                return testContext->m_signals;
            }));
    EXPECT_CALL(*testContext->m_glibWrapper, gSignalQuery(testContext->m_signals[0], _))
        .WillOnce(Invoke([&](guint signal_id, GSignalQuery *query) { query->signal_name = "buffer-underflow-callback"; }));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(testContext->m_signals));
    EXPECT_CALL(*testContext->m_glibWrapper, gSignalConnect(_, CharStrMatcher("buffer-underflow-callback"), _, _))
        .WillOnce(Invoke(
            [&](gpointer instance, const gchar *detailed_signal, GCallback c_handler, gpointer data)
            {
                testContext->m_videoUnderflowCallback = c_handler;
                return kSignalId;
            }));
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectUnref(_));
}

void TasksTestsBase::expectSetupAudioElement()
{
    EXPECT_CALL(*testContext->m_glibWrapper, gStrHasPrefix(_, CharStrMatcher("westerossink"))).WillOnce(Return(false));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrHasPrefix(_, CharStrMatcher("amlhalasink"))).WillOnce(Return(false));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetFactory(_)).WillRepeatedly(Return(testContext->m_elementFactory));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstElementFactoryListIsType(testContext->m_elementFactory, GST_ELEMENT_FACTORY_TYPE_DECODER))
        .WillRepeatedly(Return(TRUE));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstElementFactoryListIsType(testContext->m_elementFactory, GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO))
        .WillOnce(Return(FALSE));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstElementFactoryListIsType(testContext->m_elementFactory, GST_ELEMENT_FACTORY_TYPE_MEDIA_AUDIO))
        .WillOnce(Return(TRUE));
    EXPECT_CALL(*testContext->m_glibWrapper, gObjectType(&testContext->m_element)).WillRepeatedly(Return(G_TYPE_PARAM));
    EXPECT_CALL(*testContext->m_glibWrapper, gSignalListIds(_, _))
        .WillOnce(Invoke(
            [&](GType itype, guint *n_ids)
            {
                *n_ids = 1;
                return testContext->m_signals;
            }));
    EXPECT_CALL(*testContext->m_glibWrapper, gSignalQuery(testContext->m_signals[0], _))
        .WillOnce(Invoke([&](guint signal_id, GSignalQuery *query) { query->signal_name = "buffer-underflow-callback"; }));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(testContext->m_signals));
    EXPECT_CALL(*testContext->m_glibWrapper, gSignalConnect(_, _, _, _))
        .WillOnce(Invoke(
            [&](gpointer instance, const gchar *detailed_signal, GCallback c_handler, gpointer data)
            {
                testContext->m_audioUnderflowCallback = c_handler;
                return kSignalId;
            }));
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectUnref(_));
}

void TasksTestsBase::shouldSetupVideoElementOnly()
{
    EXPECT_CALL(*testContext->m_glibWrapper, gStrHasPrefix(_, CharStrMatcher("westerossink"))).WillOnce(Return(false));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrHasPrefix(_, CharStrMatcher("amlhalasink"))).WillOnce(Return(false));
    expectSetupVideoElement();
}

void TasksTestsBase::shouldSetupVideoElementWesterossink()
{
    testContext->m_context.pendingGeometry = kRectangle;
    EXPECT_CALL(*testContext->m_glibWrapper, gStrHasPrefix(_, CharStrMatcher("westerossink"))).WillOnce(Return(true));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrHasPrefix(_, CharStrMatcher("amlhalasink"))).WillOnce(Return(false));
    EXPECT_CALL(testContext->m_gstPlayer, setWesterossinkRectangle());
    expectSetupVideoElement();
}

void TasksTestsBase::shouldSetupVideoElementAmlhalasink()
{
    EXPECT_CALL(*testContext->m_glibWrapper, gStrHasPrefix(_, CharStrMatcher("westerossink"))).WillOnce(Return(false));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrHasPrefix(_, CharStrMatcher("amlhalasink"))).WillOnce(Return(true));
    EXPECT_CALL(*testContext->m_glibWrapper,
                gObjectSetStub(G_OBJECT(&(testContext->m_element)), CharStrMatcher("wait-video")));
    EXPECT_CALL(*testContext->m_glibWrapper,
                gObjectSetStub(G_OBJECT(&(testContext->m_element)), CharStrMatcher("a-wait-timeout")));
    EXPECT_CALL(*testContext->m_glibWrapper,
                gObjectSetStub(G_OBJECT(&(testContext->m_element)), CharStrMatcher("disable-xrun")));
    expectSetupVideoElement();
}

void TasksTestsBase::shouldSetupVideoElementPendingGeometryNonWesterissink()
{
    testContext->m_context.pendingGeometry = kRectangle;
    EXPECT_CALL(*testContext->m_glibWrapper, gStrHasPrefix(_, CharStrMatcher("westerossink"))).WillOnce(Return(false));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrHasPrefix(_, CharStrMatcher("amlhalasink"))).WillOnce(Return(false));
    expectSetupVideoElement();
}

void TasksTestsBase::shouldSetupAudioElementOnly()
{
    expectSetupAudioElement();
}

void TasksTestsBase::shouldSetVideoUnderflowCallback()
{
    ASSERT_TRUE(testContext->m_videoUnderflowCallback);
    EXPECT_CALL(testContext->m_gstPlayer, scheduleVideoUnderflow());
}

void TasksTestsBase::triggerVideoUnderflowCallback()
{
    ((void (*)(GstElement *, guint, gpointer, gpointer))testContext->m_videoUnderflowCallback)(&testContext->m_element,
                                                                                               0, nullptr,
                                                                                               &testContext->m_gstPlayer);
}

void TasksTestsBase::shouldSetAudioUnderflowCallback()
{
    ASSERT_TRUE(testContext->m_audioUnderflowCallback);
    EXPECT_CALL(testContext->m_gstPlayer, scheduleAudioUnderflow());
}

void TasksTestsBase::triggerAudioUnderflowCallback()
{
    ((void (*)(GstElement *, guint, gpointer, gpointer))testContext->m_audioUnderflowCallback)(&testContext->m_element,
                                                                                               0, nullptr,
                                                                                               &testContext->m_gstPlayer);
}

void TasksTestsBase::triggerSetupElement()
{
    firebolt::rialto::server::tasks::generic::SetupElement task{testContext->m_context, testContext->m_gstWrapper,
                                                                testContext->m_glibWrapper, testContext->m_gstPlayer,
                                                                &testContext->m_element};
    task.execute();
}

void TasksTestsBase::setPipelineToNull()
{
    testContext->m_context.pipeline = nullptr;
}

void TasksTestsBase::triggerSetVideoGeometryFailure()
{
    firebolt::rialto::server::tasks::generic::SetVideoGeometry task{testContext->m_context, testContext->m_gstPlayer,
                                                                    kRectangle};
    task.execute();
    EXPECT_EQ(testContext->m_context.pendingGeometry, kRectangle);
}

void TasksTestsBase::shouldSetVideoGeometry()
{
    EXPECT_CALL(testContext->m_gstPlayer, setWesterossinkRectangle());
}

void TasksTestsBase::triggerSetVideoGeometrySuccess()
{
    firebolt::rialto::server::tasks::generic::SetVideoGeometry task{testContext->m_context, testContext->m_gstPlayer,
                                                                    kRectangle};
    task.execute();
    EXPECT_EQ(testContext->m_context.pendingGeometry, kRectangle);
}

void TasksTestsBase::setAllSourcesAttached()
{
    testContext->m_context.wereAllSourcesAttached = true;
}

void TasksTestsBase::shouldScheduleAllSourcesAttached()
{
    EXPECT_CALL(testContext->m_gstPlayer, scheduleAllSourcesAttached());
}

void TasksTestsBase::triggerSetupSource()
{
    firebolt::rialto::server::tasks::generic::SetupSource task{testContext->m_context, testContext->m_gstPlayer,
                                                               &testContext->m_element};
    task.execute();
    EXPECT_EQ(testContext->m_context.source, &testContext->m_element);
}

void TasksTestsBase::shouldSetGstVolume()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstStreamVolumeSetVolume(_, GST_STREAM_VOLUME_FORMAT_LINEAR, kVolume));
}

void TasksTestsBase::triggerSetVolume()
{
    firebolt::rialto::server::tasks::generic::SetVolume task{testContext->m_context, testContext->m_gstWrapper, kVolume};
    task.execute();
}

void TasksTestsBase::shouldAttachAllAudioSamples()
{
    std::shared_ptr<firebolt::rialto::CodecData> kNullCodecData{};
    EXPECT_CALL(testContext->m_gstPlayer, createBuffer(_)).Times(2).WillRepeatedly(Return(&testContext->m_audioBuffer));
    EXPECT_CALL(testContext->m_gstPlayer, updateAudioCaps(kSampleRate, kNumberOfChannels, kNullCodecData));
    EXPECT_CALL(testContext->m_gstPlayer, updateAudioCaps(kSampleRate, kNumberOfChannels, kCodecDataBuffer));
    EXPECT_CALL(testContext->m_gstPlayer, attachAudioData()).Times(2);
    EXPECT_CALL(testContext->m_gstPlayer, notifyNeedMediaData(true, false));
}

void TasksTestsBase::triggerAttachSamplesAudio()
{
    auto samples = buildAudioSamples();
    firebolt::rialto::server::tasks::generic::AttachSamples task{testContext->m_context, testContext->m_gstPlayer,
                                                                 samples};
    task.execute();
    EXPECT_EQ(testContext->m_context.audioBuffers.size(), 2);
}

void TasksTestsBase::shouldAttachAllVideoSamples()
{
    std::shared_ptr<firebolt::rialto::CodecData> kNullCodecData{};
    EXPECT_CALL(testContext->m_gstPlayer, createBuffer(_)).Times(2).WillRepeatedly(Return(&testContext->m_videoBuffer));
    EXPECT_CALL(testContext->m_gstPlayer, updateVideoCaps(kWidth, kHeight, kFrameRate, kNullCodecData));
    EXPECT_CALL(testContext->m_gstPlayer, updateVideoCaps(kWidth, kHeight, kFrameRate, kCodecDataBuffer));
    EXPECT_CALL(testContext->m_gstPlayer, attachVideoData()).Times(2);
    EXPECT_CALL(testContext->m_gstPlayer, notifyNeedMediaData(false, true));
}

void TasksTestsBase::triggerAttachSamplesVideo()
{
    auto samples = buildVideoSamples();
    firebolt::rialto::server::tasks::generic::AttachSamples task{testContext->m_context, testContext->m_gstPlayer,
                                                                 samples};
    task.execute();
    EXPECT_EQ(testContext->m_context.videoBuffers.size(), 2);
}

void TasksTestsBase::shouldAttachAudioSource()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsNewEmptySimple(StrEq("audio/mpeg")))
        .WillOnce(Return(&testContext->m_gstCaps1));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstCapsSetSimpleIntStub(&testContext->m_gstCaps1, StrEq("mpegversion"), G_TYPE_INT, 4));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsToString(&testContext->m_gstCaps1))
        .WillOnce(Return(&testContext->m_capsStr));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(&testContext->m_capsStr));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementFactoryMake(_, CharStrMatcher(kAudName.c_str())))
        .WillOnce(Return(&testContext->m_appSrcAudio));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstAppSrcSetCaps(GST_APP_SRC(&testContext->m_appSrcAudio), &testContext->m_gstCaps1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsUnref(&testContext->m_gstCaps1));
}

void TasksTestsBase::triggerAttachAudioSource()
{
    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSource> source =
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceAudio>("audio/aac", false);
    firebolt::rialto::server::tasks::generic::AttachSource task{testContext->m_context,
                                                                testContext->m_gstWrapper,
                                                                testContext->m_glibWrapper,
                                                                testContext->m_rdkGstreamerUtilsWrapper,
                                                                testContext->m_gstPlayer,
                                                                source};
    task.execute();
}

void TasksTestsBase::checkAudioSourceAttached()
{
    EXPECT_EQ(1, testContext->m_context.streamInfo.size());
    EXPECT_NE(testContext->m_context.streamInfo.end(),
              testContext->m_context.streamInfo.find(firebolt::rialto::MediaSourceType::AUDIO));
    EXPECT_EQ(&testContext->m_appSrcAudio,
              testContext->m_context.streamInfo.at(firebolt::rialto::MediaSourceType::AUDIO).appSrc);
    EXPECT_FALSE(testContext->m_context.streamInfo.at(firebolt::rialto::MediaSourceType::AUDIO).hasDrm);
}

void TasksTestsBase::shouldAttachAudioSourceWithChannelsAndRate()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsNewEmptySimple(StrEq("audio/x-eac3")))
        .WillOnce(Return(&testContext->m_gstCaps1));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstCapsSetSimpleIntStub(&testContext->m_gstCaps1, StrEq("channels"), G_TYPE_INT, kNumberOfChannels));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstCapsSetSimpleIntStub(&testContext->m_gstCaps1, StrEq("rate"), G_TYPE_INT, kSampleRate));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsToString(&testContext->m_gstCaps1))
        .WillOnce(Return(&testContext->m_capsStr));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(&testContext->m_capsStr));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementFactoryMake(_, CharStrMatcher(kAudName.c_str())))
        .WillOnce(Return(&testContext->m_appSrcAudio));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstAppSrcSetCaps(GST_APP_SRC(&testContext->m_appSrcAudio), &testContext->m_gstCaps1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsUnref(&testContext->m_gstCaps1));
}

void TasksTestsBase::triggerAttachAudioSourceWithChannelsAndRateAndDrm()
{
    firebolt::rialto::AudioConfig audioConfig{kNumberOfChannels, kSampleRate, {}};
    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSource> source =
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceAudio>("audio/x-eac3", true, audioConfig);
    firebolt::rialto::server::tasks::generic::AttachSource task{testContext->m_context,
                                                                testContext->m_gstWrapper,
                                                                testContext->m_glibWrapper,
                                                                testContext->m_rdkGstreamerUtilsWrapper,
                                                                testContext->m_gstPlayer,
                                                                source};
    task.execute();
}

void TasksTestsBase::checkAudioSourceAttachedWithDrm()
{
    EXPECT_EQ(1, testContext->m_context.streamInfo.size());
    EXPECT_NE(testContext->m_context.streamInfo.end(),
              testContext->m_context.streamInfo.find(firebolt::rialto::MediaSourceType::AUDIO));
    EXPECT_EQ(&testContext->m_appSrcAudio,
              testContext->m_context.streamInfo.at(firebolt::rialto::MediaSourceType::AUDIO).appSrc);
    EXPECT_TRUE(testContext->m_context.streamInfo.at(firebolt::rialto::MediaSourceType::AUDIO).hasDrm);
}

void TasksTestsBase::shouldAttachAudioSourceWithAudioSpecificConf()
{
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstCodecUtilsOpusCreateCapsFromHeader(arrayMatcher(kCodecBufferVector), kCodecBufferVector.size()))
        .WillOnce(Return(&testContext->m_gstCaps1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsToString(&testContext->m_gstCaps1))
        .WillOnce(Return(&testContext->m_capsStr));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(&testContext->m_capsStr));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementFactoryMake(_, CharStrMatcher(kAudName.c_str())))
        .WillOnce(Return(&testContext->m_appSrcAudio));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstAppSrcSetCaps(GST_APP_SRC(&testContext->m_appSrcAudio), &testContext->m_gstCaps1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsUnref(&testContext->m_gstCaps1));
}

void TasksTestsBase::triggerAttachOpusAudioSourceWithAudioSpecificConf()
{
    firebolt::rialto::AudioConfig audioConfig{0, 0, kCodecBufferVector};
    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSource> source =
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceAudio>("audio/x-opus", false, audioConfig);
    firebolt::rialto::server::tasks::generic::AttachSource task{testContext->m_context,
                                                                testContext->m_gstWrapper,
                                                                testContext->m_glibWrapper,
                                                                testContext->m_rdkGstreamerUtilsWrapper,
                                                                testContext->m_gstPlayer,
                                                                source};
    task.execute();
}

void TasksTestsBase::shouldAttachVideoSource()
{
    gpointer memory = nullptr;
    expectSetGenericVideoCaps();
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsNewEmptySimple(StrEq("video/x-h264")))
        .WillOnce(Return(&testContext->m_gstCaps1));
    EXPECT_CALL(*testContext->m_glibWrapper, gMemdup(arrayMatcher(kCodecDataBuffer->data), kCodecDataBuffer->data.size()))
        .WillOnce(Return(memory));
    EXPECT_CALL(*testContext->m_gstWrapper, gstBufferNewWrapped(memory, kCodecDataBuffer->data.size()))
        .WillOnce(Return(&(testContext->m_videoBuffer)));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsSetSimpleBufferStub(&testContext->m_gstCaps1, StrEq("codec_data"), _,
                                                                       &(testContext->m_videoBuffer)));
    EXPECT_CALL(*testContext->m_gstWrapper, gstBufferUnref(&(testContext->m_videoBuffer)));
}

void TasksTestsBase::triggerAttachVideoSource()
{
    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSource> source =
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceVideo>("video/h264", false, kWidth, kHeight,
                                                                             firebolt::rialto::SegmentAlignment::AU,
                                                                             firebolt::rialto::StreamFormat::AVC,
                                                                             kCodecDataBuffer);
    firebolt::rialto::server::tasks::generic::AttachSource task{testContext->m_context,
                                                                testContext->m_gstWrapper,
                                                                testContext->m_glibWrapper,
                                                                testContext->m_rdkGstreamerUtilsWrapper,
                                                                testContext->m_gstPlayer,
                                                                source};
    task.execute();
}

void TasksTestsBase::checkVideoSourceAttached()
{
    EXPECT_EQ(1, testContext->m_context.streamInfo.size());
    EXPECT_NE(testContext->m_context.streamInfo.end(),
              testContext->m_context.streamInfo.find(firebolt::rialto::MediaSourceType::VIDEO));
    EXPECT_EQ(&testContext->m_appSrcVideo,
              testContext->m_context.streamInfo.at(firebolt::rialto::MediaSourceType::VIDEO).appSrc);
    EXPECT_FALSE(testContext->m_context.streamInfo.at(firebolt::rialto::MediaSourceType::VIDEO).hasDrm);
}

void TasksTestsBase::shouldAttachVideoSourceWithStringCodecData()
{
    expectSetGenericVideoCaps();
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsNewEmptySimple(StrEq("video/x-h264")))
        .WillOnce(Return(&testContext->m_gstCaps1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsSetSimpleStringStub(&testContext->m_gstCaps1, StrEq("codec_data"),
                                                                       G_TYPE_STRING, StrEq(kCodecStr.c_str())));
}

void TasksTestsBase::triggerAttachVideoSourceWithStringCodecData()
{
    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSource> source =
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceVideo>("video/h264", true, kWidth, kHeight,
                                                                             firebolt::rialto::SegmentAlignment::AU,
                                                                             firebolt::rialto::StreamFormat::AVC,
                                                                             kCodecDataStr);
    firebolt::rialto::server::tasks::generic::AttachSource task{testContext->m_context,
                                                                testContext->m_gstWrapper,
                                                                testContext->m_glibWrapper,
                                                                testContext->m_rdkGstreamerUtilsWrapper,
                                                                testContext->m_gstPlayer,
                                                                source};
    task.execute();
}

void TasksTestsBase::checkVideoSourceAttachedWithDrm()
{
    EXPECT_EQ(1, testContext->m_context.streamInfo.size());
    EXPECT_NE(testContext->m_context.streamInfo.end(),
              testContext->m_context.streamInfo.find(firebolt::rialto::MediaSourceType::VIDEO));
    EXPECT_EQ(&testContext->m_appSrcVideo,
              testContext->m_context.streamInfo.at(firebolt::rialto::MediaSourceType::VIDEO).appSrc);
    EXPECT_TRUE(testContext->m_context.streamInfo.at(firebolt::rialto::MediaSourceType::VIDEO).hasDrm);
}

void TasksTestsBase::shouldAttachVideoSourceWithEmptyCodecData()
{
    gpointer memory = nullptr;
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsNewEmptySimple(StrEq("video/x-h264")))
        .WillOnce(Return(&testContext->m_gstCaps1));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstCapsSetSimpleStringStub(&testContext->m_gstCaps1, StrEq("alignment"), _, StrEq("au")));
    EXPECT_CALL(*testContext->m_glibWrapper, gMemdup(arrayMatcher(kEmptyCodecData->data), kEmptyCodecData->data.size()))
        .WillOnce(Return(memory));
    EXPECT_CALL(*testContext->m_gstWrapper, gstBufferNewWrapped(memory, kEmptyCodecData->data.size()))
        .WillOnce(Return(&(testContext->m_videoBuffer)));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsSetSimpleBufferStub(&testContext->m_gstCaps1, StrEq("codec_data"), _,
                                                                       &(testContext->m_videoBuffer)));
    EXPECT_CALL(*testContext->m_gstWrapper, gstBufferUnref(&(testContext->m_videoBuffer)));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstCapsSetSimpleStringStub(&testContext->m_gstCaps1, StrEq("stream-format"), _, StrEq("avc")));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsToString(&testContext->m_gstCaps1))
        .WillOnce(Return(&testContext->m_capsStr));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(&testContext->m_capsStr));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementFactoryMake(_, CharStrMatcher(kVidName.c_str())))
        .WillOnce(Return(&testContext->m_appSrcVideo));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstAppSrcSetCaps(GST_APP_SRC(&testContext->m_appSrcVideo), &testContext->m_gstCaps1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsUnref(&testContext->m_gstCaps1));
}

void TasksTestsBase::triggerAttachVideoSourceWithEmptyCodecData()
{
    int width = 0, height = 0;
    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSource> source =
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceVideo>("video/h264", true, width, height,
                                                                             firebolt::rialto::SegmentAlignment::AU,
                                                                             firebolt::rialto::StreamFormat::AVC,
                                                                             kEmptyCodecData);
    firebolt::rialto::server::tasks::generic::AttachSource task{testContext->m_context,
                                                                testContext->m_gstWrapper,
                                                                testContext->m_glibWrapper,
                                                                testContext->m_rdkGstreamerUtilsWrapper,
                                                                testContext->m_gstPlayer,
                                                                source};
    task.execute();
}

void TasksTestsBase::shouldAttachVideoSourceWithDolbyVisionSource()
{
    gpointer memory = nullptr;
    expectSetGenericVideoCaps();
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsNewEmptySimple(StrEq("video/x-h265")))
        .WillOnce(Return(&testContext->m_gstCaps1));
    EXPECT_CALL(*testContext->m_glibWrapper, gMemdup(arrayMatcher(kCodecDataBuffer->data), kCodecDataBuffer->data.size()))
        .WillOnce(Return(memory));
    EXPECT_CALL(*testContext->m_gstWrapper, gstBufferNewWrapped(memory, kCodecDataBuffer->data.size()))
        .WillOnce(Return(&(testContext->m_videoBuffer)));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsSetSimpleBufferStub(&testContext->m_gstCaps1, StrEq("codec_data"), _,
                                                                       &(testContext->m_videoBuffer)));
    EXPECT_CALL(*testContext->m_gstWrapper, gstBufferUnref(&(testContext->m_videoBuffer)));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstCapsSetSimpleBooleanStub(&testContext->m_gstCaps1, StrEq("dovi-stream"), _, true));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstCapsSetSimpleUintStub(&testContext->m_gstCaps1, StrEq("dv_profile"), _, kDolbyVisionProfile));
}

void TasksTestsBase::triggerAttachVideoSourceWithDolbyVisionSource()
{
    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSource> source = std::make_unique<
        firebolt::rialto::IMediaPipeline::MediaSourceVideoDolbyVision>("video/h265", kDolbyVisionProfile, true, kWidth,
                                                                       kHeight, firebolt::rialto::SegmentAlignment::AU,
                                                                       firebolt::rialto::StreamFormat::AVC,
                                                                       kCodecDataBuffer);
    firebolt::rialto::server::tasks::generic::AttachSource task{testContext->m_context,
                                                                testContext->m_gstWrapper,
                                                                testContext->m_glibWrapper,
                                                                testContext->m_rdkGstreamerUtilsWrapper,
                                                                testContext->m_gstPlayer,
                                                                source};
    task.execute();
}

void TasksTestsBase::expectSetGenericVideoCaps()
{
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstCapsSetSimpleIntStub(&testContext->m_gstCaps1, StrEq("width"), G_TYPE_INT, kWidth));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstCapsSetSimpleIntStub(&testContext->m_gstCaps1, StrEq("height"), G_TYPE_INT, kHeight));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstCapsSetSimpleStringStub(&testContext->m_gstCaps1, StrEq("alignment"), _, StrEq("au")));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstCapsSetSimpleStringStub(&testContext->m_gstCaps1, StrEq("stream-format"), _, StrEq("avc")));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsToString(&testContext->m_gstCaps1))
        .WillOnce(Return(&testContext->m_capsStr));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(&testContext->m_capsStr));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementFactoryMake(_, CharStrMatcher(kVidName.c_str())))
        .WillOnce(Return(&testContext->m_appSrcVideo));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstAppSrcSetCaps(GST_APP_SRC(&testContext->m_appSrcVideo), &testContext->m_gstCaps1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsUnref(&testContext->m_gstCaps1));
}

void TasksTestsBase::shouldSwitchAudioSource()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsNewEmptySimple(StrEq("audio/mpeg")))
        .WillOnce(Return(&testContext->m_gstCaps1));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstCapsSetSimpleIntStub(&testContext->m_gstCaps1, StrEq("mpegversion"), G_TYPE_INT, 4));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsToString(&testContext->m_gstCaps1))
        .WillOnce(Return(&testContext->m_capsStr));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(&testContext->m_capsStr));
    EXPECT_CALL(*testContext->m_gstWrapper, gstAppSrcGetCaps(GST_APP_SRC(&testContext->m_appSrcAudio)))
        .WillOnce(Return(&testContext->m_gstCaps2));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsToString(&testContext->m_gstCaps2)).WillOnce(Return(testContext->m_xEac3Str));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(testContext->m_xEac3Str));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsUnref(&testContext->m_gstCaps2));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementQueryPosition(_, GST_FORMAT_TIME, _))
        .WillOnce(Invoke(
            [this](GstElement *element, GstFormat format, gint64 *cur)
            {
                *cur = kPosition;
                return TRUE;
            }));
    EXPECT_CALL(*(testContext->m_rdkGstreamerUtilsWrapper),
                performAudioTrackCodecChannelSwitch(&testContext->m_context.playbackGroup, _, _, _, _, _, _, _, _, _, _,
                                                    _, _))
        .WillOnce(Return(true));
    EXPECT_CALL(testContext->m_gstPlayer, notifyNeedMediaData(true, false));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsUnref(&testContext->m_gstCaps1));
}

void TasksTestsBase::triggerSwitchAudioSource()
{
    testContext->m_context.streamInfo.emplace(firebolt::rialto::MediaSourceType::AUDIO, &testContext->m_appSrcAudio);
    testContext->m_context.audioSourceRemoved = true;
    triggerAttachAudioSource();
}

void TasksTestsBase::checkNewAudioSourceAttached()
{
    EXPECT_TRUE(testContext->m_context.audioNeedData);
    EXPECT_FALSE(testContext->m_context.audioSourceRemoved);
    EXPECT_EQ(testContext->m_context.lastAudioSampleTimestamps, kPosition);
}

void TasksTestsBase::shouldNotSwitchAudioSourceWhenMimeTypeIsEmpty()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsNewEmpty()).WillOnce(Return(&testContext->m_gstCaps2));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsToString(&(testContext->m_gstCaps2)))
        .WillOnce(Return(&testContext->m_capsStr));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(&testContext->m_capsStr));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsUnref(&testContext->m_gstCaps2));
}

void TasksTestsBase::triggerSwitchAudioSourceWithEmptyMimeType()
{
    testContext->m_context.streamInfo.emplace(firebolt::rialto::MediaSourceType::AUDIO, &testContext->m_appSrcAudio);
    testContext->m_context.audioSourceRemoved = true;
    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSource> source =
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceAudio>("");
    firebolt::rialto::server::tasks::generic::AttachSource task{testContext->m_context,
                                                                testContext->m_gstWrapper,
                                                                testContext->m_glibWrapper,
                                                                testContext->m_rdkGstreamerUtilsWrapper,
                                                                testContext->m_gstPlayer,
                                                                source};
    task.execute();
}

void TasksTestsBase::shouldQueryPositionAndSetToZero()
{
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstElementQueryPosition(&testContext->m_pipeline, GST_FORMAT_TIME, NotNullMatcher()))
        .WillOnce(Invoke(
            [this](GstElement *element, GstFormat format, gint64 *cur)
            {
                *cur = 0;
                return TRUE;
            }));
}

void TasksTestsBase::triggerCheckAudioUnderflowNoNotification()
{
    testContext->m_context.lastAudioSampleTimestamps = kPositionOverUnderflowMargin;
    firebolt::rialto::server::tasks::generic::CheckAudioUnderflow task{testContext->m_context, testContext->m_gstPlayer,
                                                                       &testContext->m_gstPlayerClient,
                                                                       testContext->m_gstWrapper};
    task.execute();

    EXPECT_FALSE(testContext->m_context.audioUnderflowOccured);
}

void TasksTestsBase::shouldNotifyAudioUnderflow()
{
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstElementQueryPosition(&testContext->m_pipeline, GST_FORMAT_TIME, NotNullMatcher()))
        .WillOnce(Invoke(
            [this](GstElement *element, GstFormat format, gint64 *cur)
            {
                *cur = kPositionOverUnderflowMargin;
                return TRUE;
            }));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetState(&testContext->m_pipeline))
        .WillOnce(Invoke([this](GstElement *element) { return GST_STATE_PLAYING; }));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetPendingState(&testContext->m_pipeline))
        .WillOnce(Invoke([this](GstElement *element) { return GST_STATE_VOID_PENDING; }));
    EXPECT_CALL(testContext->m_gstPlayerClient, notifyBufferUnderflow(firebolt::rialto::MediaSourceType::AUDIO));
}

void TasksTestsBase::triggerCheckAudioUnderflow()
{
    testContext->m_context.lastAudioSampleTimestamps = 0;
    firebolt::rialto::server::tasks::generic::CheckAudioUnderflow task{testContext->m_context, testContext->m_gstPlayer,
                                                                       &testContext->m_gstPlayerClient,
                                                                       testContext->m_gstWrapper};
    task.execute();

    EXPECT_TRUE(testContext->m_context.audioUnderflowOccured);
}

void TasksTestsBase::shouldNotRegisterCallbackWhenPtrsAreNotEqual()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectParent(&testContext->m_element)).WillOnce(Return(&testContext->m_obj1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectCast(&testContext->m_bin)).WillOnce(Return(&testContext->m_obj2));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(nullptr));
}

void TasksTestsBase::constructDeepElementAdded()
{
    firebolt::rialto::server::tasks::generic::DeepElementAdded task{testContext->m_context,
                                                                    testContext->m_gstPlayer,
                                                                    testContext->m_gstWrapper,
                                                                    testContext->m_glibWrapper,
                                                                    GST_BIN(&testContext->m_pipeline),
                                                                    &testContext->m_bin,
                                                                    &testContext->m_element};
}

void TasksTestsBase::shouldNotRegisterCallbackWhenElementIsNull()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectParent(&testContext->m_element)).WillOnce(Return(&testContext->m_obj1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectCast(&testContext->m_bin)).WillOnce(Return(&testContext->m_obj1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetName(&testContext->m_element)).WillOnce(Return(nullptr));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(nullptr));
}

void TasksTestsBase::shouldNotRegisterCallbackWhenElementNameIsNotTypefind()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectParent(&testContext->m_element)).WillOnce(Return(&testContext->m_obj1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectCast(&testContext->m_bin)).WillRepeatedly(Return(&testContext->m_obj1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetName(&testContext->m_element))
        .WillOnce(Return(testContext->m_elementName));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_elementName, CharStrMatcher("typefind")))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(testContext->m_elementName));
}

void TasksTestsBase::shouldRegisterCallbackForTypefindElement()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectParent(&testContext->m_element)).WillOnce(Return(&testContext->m_obj1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectCast(&testContext->m_bin)).WillRepeatedly(Return(&testContext->m_obj1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetName(&testContext->m_element))
        .WillOnce(Return(testContext->m_typefindElementName));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_typefindElementName, CharStrMatcher("typefind")))
        .WillOnce(Return(testContext->m_typefindElementName));
    EXPECT_CALL(*testContext->m_glibWrapper, gSignalConnect(G_OBJECT(&testContext->m_element),
                                                            CharStrMatcher("have-type"), _, &testContext->m_gstPlayer))
        .WillOnce(Return(kSignalId));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(testContext->m_typefindElementName));
}

void TasksTestsBase::shouldUpdatePlaybackGroupWhenCallbackIsCalled()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectParent(&testContext->m_element)).WillOnce(Return(&testContext->m_obj1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectCast(&testContext->m_bin)).WillOnce(Return(&testContext->m_obj1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetName(&testContext->m_element))
        .WillOnce(Return(testContext->m_typefindElementName));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_typefindElementName, CharStrMatcher("typefind")))
        .WillOnce(Return(testContext->m_typefindElementName));
    EXPECT_CALL(*testContext->m_glibWrapper, gSignalConnect(G_OBJECT(&testContext->m_element),
                                                            CharStrMatcher("have-type"), _, &testContext->m_gstPlayer))
        .WillOnce(Invoke(
            [&](gpointer instance, const gchar *detailed_signal, GCallback c_handler, gpointer data)
            {
                ((void (*)(GstElement *, guint, const GstCaps *, gpointer))c_handler)(&testContext->m_element, 0,
                                                                                      &testContext->m_gstCaps1,
                                                                                      &testContext->m_gstPlayer);
                return kSignalId;
            }));
    EXPECT_CALL(testContext->m_gstPlayer, updatePlaybackGroup(&testContext->m_element, &testContext->m_gstCaps1));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(testContext->m_typefindElementName));
}

void TasksTestsBase::shouldSetTypefindElement()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectCast(nullptr)).WillOnce(Return(nullptr));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_typefindElementName, CharStrMatcher("audiosink")))
        .WillOnce(Return(nullptr));
}

void TasksTestsBase::triggerDeepElementAdded()
{
    firebolt::rialto::server::tasks::generic::DeepElementAdded task{testContext->m_context,
                                                                    testContext->m_gstPlayer,
                                                                    testContext->m_gstWrapper,
                                                                    testContext->m_glibWrapper,
                                                                    GST_BIN(&testContext->m_pipeline),
                                                                    &testContext->m_bin,
                                                                    &testContext->m_element};
    task.execute();
}

void TasksTestsBase::checkTypefindPlaybackGroupAdded()
{
    EXPECT_EQ(testContext->m_context.playbackGroup.m_gstPipeline, GST_ELEMENT(&testContext->m_pipeline));
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioTypefind, &testContext->m_element);
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioParse, nullptr);
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioDecoder, nullptr);
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioPlaysinkBin, nullptr);
}

void TasksTestsBase::checkPipelinePlaybackGroupAdded()
{
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioTypefind, nullptr);
    EXPECT_EQ(testContext->m_context.playbackGroup.m_gstPipeline, GST_ELEMENT(&testContext->m_pipeline));
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioParse, nullptr);
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioDecoder, nullptr);
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioPlaysinkBin, nullptr);
}

void TasksTestsBase::shouldSetParseElement()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectParent(&testContext->m_element)).WillOnce(Return(&testContext->m_obj1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectCast(&testContext->m_bin)).WillRepeatedly(Return(&testContext->m_obj1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetName(&testContext->m_element))
        .WillOnce(Return(testContext->m_parseElementName));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_parseElementName, CharStrMatcher("typefind")))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(testContext->m_parseElementName));

    testContext->m_context.playbackGroup.m_curAudioDecodeBin = &testContext->m_audioDecodeBin;
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectCast(&testContext->m_audioDecodeBin))
        .WillOnce(Return(&testContext->m_obj1));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_parseElementName, CharStrMatcher("parse")))
        .WillOnce(Return(testContext->m_parseElementName));
}

void TasksTestsBase::checkParsePlaybackGroupAdded()
{
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioTypefind, nullptr);
    EXPECT_EQ(testContext->m_context.playbackGroup.m_gstPipeline, GST_ELEMENT(&testContext->m_pipeline));
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioParse, &testContext->m_element);
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioDecoder, nullptr);
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioPlaysinkBin, nullptr);
}

void TasksTestsBase::shouldSetDecoderElement()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectParent(&testContext->m_element)).WillOnce(Return(&testContext->m_obj1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectCast(&testContext->m_bin)).WillRepeatedly(Return(&testContext->m_obj1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetName(&testContext->m_element))
        .WillOnce(Return(testContext->m_decoderElementName));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_decoderElementName, CharStrMatcher("typefind")))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(testContext->m_decoderElementName));

    testContext->m_context.playbackGroup.m_curAudioDecodeBin = &testContext->m_audioDecodeBin;
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectCast(&testContext->m_audioDecodeBin))
        .WillOnce(Return(&testContext->m_obj1));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_decoderElementName, CharStrMatcher("parse")))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_decoderElementName, CharStrMatcher("dec")))
        .WillOnce(Return(testContext->m_decoderElementName));
}

void TasksTestsBase::checkDecoderPlaybackGroupAdded()
{
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioTypefind, nullptr);
    EXPECT_EQ(testContext->m_context.playbackGroup.m_gstPipeline, GST_ELEMENT(&testContext->m_pipeline));
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioParse, nullptr);
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioDecoder, &testContext->m_element);
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioPlaysinkBin, nullptr);
}

void TasksTestsBase::shouldSetGenericElement()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectCast(nullptr)).WillOnce(Return(nullptr));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_elementName, CharStrMatcher("audiosink")))
        .WillOnce(Return(nullptr));
}

void TasksTestsBase::shouldSetAudioSinkElement()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectParent(&testContext->m_element)).WillOnce(Return(&testContext->m_obj1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectCast(&testContext->m_bin)).WillRepeatedly(Return(&testContext->m_obj1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetName(&testContext->m_element))
        .WillOnce(Return(testContext->m_audioSinkElementName));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_audioSinkElementName, CharStrMatcher("typefind")))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(testContext->m_audioSinkElementName));

    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectCast(nullptr)).WillOnce(Return(nullptr));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_audioSinkElementName, CharStrMatcher("audiosink")))
        .WillOnce(Return(testContext->m_audioSinkElementName));
}

void TasksTestsBase::shouldHaveNullParentSink()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetParent(&testContext->m_element))
        .WillOnce(Return(GST_OBJECT(&testContext->m_audioParentSink)));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetName(&testContext->m_audioParentSink)).WillOnce(Return(nullptr));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(nullptr));
}

void TasksTestsBase::shouldHaveNonBinParentSink()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetParent(&testContext->m_element))
        .WillOnce(Return(GST_OBJECT(&testContext->m_audioParentSink)));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetName(&testContext->m_audioParentSink))
        .WillOnce(Return(testContext->m_elementName));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_elementName, CharStrMatcher("bin")))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(testContext->m_elementName));
}

void TasksTestsBase::shouldHaveBinParentSink()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetParent(&testContext->m_element))
        .WillOnce(Return(GST_OBJECT(&testContext->m_audioParentSink)));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetName(&testContext->m_audioParentSink))
        .WillOnce(Return(testContext->m_binElementName));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_binElementName, CharStrMatcher("bin")))
        .WillOnce(Return(testContext->m_binElementName));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(testContext->m_binElementName));
}

void TasksTestsBase::checkAudioSinkPlaybackGroupAdded()
{
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioTypefind, nullptr);
    EXPECT_EQ(testContext->m_context.playbackGroup.m_gstPipeline, GST_ELEMENT(&testContext->m_pipeline));
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioParse, nullptr);
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioDecoder, nullptr);
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioPlaysinkBin, &testContext->m_audioParentSink);
}

void TasksTestsBase::triggerUpdatePlaybackGroupNoCaps()
{
    firebolt::rialto::server::tasks::generic::UpdatePlaybackGroup task{testContext->m_context, testContext->m_gstWrapper,
                                                                       testContext->m_glibWrapper,
                                                                       &testContext->m_element, nullptr};
    task.execute();
}

void TasksTestsBase::checkNoPlaybackGroupAdded()
{
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioDecodeBin, nullptr);
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioTypefind, nullptr);
}

void TasksTestsBase::shouldReturnNullCaps()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsToString(&testContext->m_gstCaps1)).WillOnce(Return(nullptr));
}

void TasksTestsBase::triggerUpdatePlaybackGroup()
{
    firebolt::rialto::server::tasks::generic::UpdatePlaybackGroup task{testContext->m_context, testContext->m_gstWrapper,
                                                                       testContext->m_glibWrapper,
                                                                       &testContext->m_element, &testContext->m_gstCaps1};
    task.execute();
}

void TasksTestsBase::shouldDoNothingForVideoCaps()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsToString(&testContext->m_gstCaps1))
        .WillOnce(Return(testContext->m_videoStr));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_videoStr, CharStrMatcher("audio/")))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(testContext->m_videoStr));
}

void TasksTestsBase::shouldDoNothingWhenTypefindParentIsNull()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsToString(&testContext->m_gstCaps1))
        .WillOnce(Return(testContext->m_audioStr));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_audioStr, CharStrMatcher("audio/")))
        .WillOnce(Return(testContext->m_audioStr));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetParent(&testContext->m_element)).WillOnce(Return(nullptr));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(testContext->m_audioStr));
}

void TasksTestsBase::shouldDoNothingWhenElementOtherThanDecodebin()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsToString(&testContext->m_gstCaps1))
        .WillOnce(Return(testContext->m_audioStr));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_audioStr, CharStrMatcher("audio/")))
        .WillOnce(Return(testContext->m_audioStr));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetParent(&testContext->m_element))
        .WillOnce(Return(GST_OBJECT(&testContext->m_audioParentSink)));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetName(&testContext->m_audioParentSink))
        .WillOnce(Return(testContext->m_elementName));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_elementName, CharStrMatcher("decodebin")))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(testContext->m_elementName));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(testContext->m_audioStr));
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectUnref(&testContext->m_audioParentSink));
}

void TasksTestsBase::shouldSuccessfullyFindTypefindAndParent()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsToString(&testContext->m_gstCaps1))
        .WillOnce(Return(testContext->m_audioStr));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_audioStr, CharStrMatcher("audio/")))
        .WillOnce(Return(testContext->m_audioStr));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetParent(&testContext->m_element))
        .WillOnce(Return(GST_OBJECT(&testContext->m_audioParentSink)));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetName(&testContext->m_audioParentSink))
        .WillOnce(Return(testContext->m_decoderElementName));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_decoderElementName, CharStrMatcher("decodebin")))
        .WillOnce(Return(testContext->m_decoderElementName));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetName(&testContext->m_element))
        .WillOnce(Return(testContext->m_typefindElementName));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(testContext->m_decoderElementName));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(testContext->m_typefindElementName));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(testContext->m_audioStr));
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectUnref(&testContext->m_audioParentSink));
}

void TasksTestsBase::checkPlaybackGroupAdded()
{
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioDecodeBin, &testContext->m_audioParentSink);
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioTypefind, &testContext->m_element);
}

void TasksTestsBase::shouldStopGstPlayer()
{
    testContext->m_context.videoNeedData = true;
    testContext->m_context.audioNeedData = true;
    EXPECT_CALL(testContext->m_gstPlayer, stopPositionReportingAndCheckAudioUnderflowTimer());
    EXPECT_CALL(testContext->m_gstPlayer, changePipelineState(GST_STATE_NULL));
}

void TasksTestsBase::triggerStop()
{
    firebolt::rialto::server::tasks::generic::Stop task{testContext->m_context, testContext->m_gstPlayer};
    task.execute();
}

void TasksTestsBase::checkNoMoreNeedData()
{
    EXPECT_FALSE(testContext->m_context.videoNeedData);
    EXPECT_FALSE(testContext->m_context.audioNeedData);
}

void TasksTestsBase::triggerEnoughDataAudio()
{
    firebolt::rialto::server::tasks::generic::EnoughData task{testContext->m_context,
                                                              GST_APP_SRC(&testContext->m_appSrcAudio)};
    task.execute();
}

void TasksTestsBase::triggerEnoughDataVideo()
{
    firebolt::rialto::server::tasks::generic::EnoughData task{testContext->m_context,
                                                              GST_APP_SRC(&testContext->m_appSrcVideo)};
    task.execute();
}

void TasksTestsBase::checkNeedDataForBothSources()
{
    EXPECT_TRUE(testContext->m_context.audioNeedData);
    EXPECT_TRUE(testContext->m_context.videoNeedData);
}

void TasksTestsBase::checkNeedDataForAudioOnly()
{
    EXPECT_TRUE(testContext->m_context.audioNeedData);
    EXPECT_FALSE(testContext->m_context.videoNeedData);
}

void TasksTestsBase::checkNeedDataForVideoOnly()
{
    EXPECT_FALSE(testContext->m_context.audioNeedData);
    EXPECT_TRUE(testContext->m_context.videoNeedData);
}

void TasksTestsBase::triggerEosAudio()
{
    firebolt::rialto::server::tasks::generic::Eos task{testContext->m_context, testContext->m_gstPlayer,
                                                       testContext->m_gstWrapper,
                                                       firebolt::rialto::MediaSourceType::AUDIO};
    task.execute();
}

void TasksTestsBase::triggerEosVideo()
{
    firebolt::rialto::server::tasks::generic::Eos task{testContext->m_context, testContext->m_gstPlayer,
                                                       testContext->m_gstWrapper,
                                                       firebolt::rialto::MediaSourceType::VIDEO};
    task.execute();
}

void TasksTestsBase::shouldGstAppSrcEndOfStreamSuccess()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstAppSrcEndOfStream(_)).WillOnce(Return(GST_FLOW_OK));
}

void TasksTestsBase::shouldGstAppSrcEndOfStreamFailure()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstAppSrcEndOfStream(_)).WillOnce(Return(GST_FLOW_ERROR));
}

void TasksTestsBase::shouldCancelUnderflow()
{
    bool underflow = true;
    EXPECT_CALL(testContext->m_gstPlayer, cancelUnderflow(underflow));
}

void TasksTestsBase::setUnderflowFlag(bool isUnderflowFlag)
{
    testContext->m_underflowFlag = isUnderflowFlag;
}

void TasksTestsBase::setUnderflowEnabled(bool isUnderflowEnabled)
{
    testContext->m_underflowEnabled = isUnderflowEnabled;
}

void TasksTestsBase::triggerVideoUnderflow()
{
    firebolt::rialto::MediaSourceType sourceType{firebolt::rialto::MediaSourceType::VIDEO};
    firebolt::rialto::server::tasks::generic::Underflow task{testContext->m_context,
                                                             testContext->m_gstPlayer,
                                                             &testContext->m_gstPlayerClient,
                                                             testContext->m_underflowFlag,
                                                             testContext->m_underflowEnabled,
                                                             sourceType};
    task.execute();
}

void TasksTestsBase::checkUnderflowFlag(bool expected)
{
    EXPECT_EQ(testContext->m_underflowFlag, expected);
}

void TasksTestsBase::shouldNotifyVideoUnderflow()
{
    EXPECT_CALL(testContext->m_gstPlayerClient, notifyBufferUnderflow(firebolt::rialto::MediaSourceType::VIDEO));
}

void TasksTestsBase::shouldNotifyEndOfStream()
{
    EXPECT_CALL(testContext->m_gstPlayerClient, notifyPlaybackState(firebolt::rialto::PlaybackState::END_OF_STREAM));
}

void TasksTestsBase::checkEndOfStreamNotified()
{
    EXPECT_TRUE(testContext->m_context.eosNotified);
}

void TasksTestsBase::shouldStopWorkerThread()
{
    EXPECT_CALL(testContext->m_gstPlayer, stopWorkerThread());
}

void TasksTestsBase::triggerShutdown()
{
    firebolt::rialto::server::tasks::generic::Shutdown task{testContext->m_gstPlayer};
    task.execute();
}

void TasksTestsBase::triggerSetMute()
{
    firebolt::rialto::server::tasks::generic::SetMute task{testContext->m_context, testContext->m_gstWrapper, kMute};
    task.execute();
}

void TasksTestsBase::shouldGstSetMute()
{
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstStreamVolumeSetMute(GST_STREAM_VOLUME(testContext->m_context.pipeline), kMute));
}

void TasksTestsBase::triggerSetPositionNullClient()
{
    firebolt::rialto::server::tasks::generic::SetPosition task{testContext->m_context, testContext->m_gstPlayer,
                                                               nullptr, testContext->m_gstWrapper, kPosition};
    task.execute();
}

void TasksTestsBase::triggerSetPosition()
{
    firebolt::rialto::server::tasks::generic::SetPosition task{testContext->m_context, testContext->m_gstPlayer,
                                                               &testContext->m_gstPlayerClient,
                                                               testContext->m_gstWrapper, kPosition};
    task.execute();
}

void TasksTestsBase::checkNeedDataPendingForBothSources()
{
    EXPECT_TRUE(testContext->m_context.audioNeedDataPending);
    EXPECT_TRUE(testContext->m_context.videoNeedDataPending);
}

void TasksTestsBase::checkBuffersDoExist()
{
    EXPECT_FALSE(testContext->m_context.audioBuffers.empty());
    EXPECT_FALSE(testContext->m_context.videoBuffers.empty());
}

void TasksTestsBase::checkDoNotNeedDataForBothSources()
{
    EXPECT_FALSE(testContext->m_context.audioNeedData);
    EXPECT_FALSE(testContext->m_context.videoNeedData);
}

void TasksTestsBase::checkNoNeedDataPendingForBothSources()
{
    EXPECT_FALSE(testContext->m_context.audioNeedDataPending);
    EXPECT_FALSE(testContext->m_context.videoNeedDataPending);
}

void TasksTestsBase::checkBuffersEmpty()
{
    EXPECT_TRUE(testContext->m_context.audioBuffers.empty());
    EXPECT_TRUE(testContext->m_context.videoBuffers.empty());
}

void TasksTestsBase::shouldExtractBuffers()
{
    EXPECT_CALL(testContext->m_gstPlayerClient, notifyPlaybackState(firebolt::rialto::PlaybackState::SEEKING));
    EXPECT_CALL(testContext->m_gstPlayerClient, clearActiveRequestsCache());
    EXPECT_CALL(*testContext->m_gstWrapper, gstBufferUnref(&testContext->m_audioBuffer));
    EXPECT_CALL(*testContext->m_gstWrapper, gstBufferUnref(&testContext->m_videoBuffer));
}

void TasksTestsBase::shouldNotifyFailure()
{
    EXPECT_CALL(testContext->m_gstPlayerClient, notifyPlaybackState(firebolt::rialto::PlaybackState::FAILURE));
}

void TasksTestsBase::shouldSeekFailure()
{
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstElementSeek(&testContext->m_pipeline, 1.0, GST_FORMAT_TIME,
                               static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH), GST_SEEK_TYPE_SET, kPosition,
                               GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE))
        .WillOnce(Return(false));
    shouldNotifyFailure();
}

void TasksTestsBase::shouldSeekSuccess()
{
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstElementSeek(&testContext->m_pipeline, testContext->m_context.playbackRate, GST_FORMAT_TIME,
                               static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH), GST_SEEK_TYPE_SET, kPosition,
                               GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE))
        .WillOnce(Return(true));
    EXPECT_CALL(testContext->m_gstPlayerClient, notifyPlaybackState(firebolt::rialto::PlaybackState::FLUSHED));
    EXPECT_CALL(testContext->m_gstPlayerClient, notifyNeedMediaData(firebolt::rialto::MediaSourceType::AUDIO))
        .WillOnce(Return(true));
    EXPECT_CALL(testContext->m_gstPlayerClient, notifyNeedMediaData(firebolt::rialto::MediaSourceType::VIDEO))
        .WillOnce(Return(true));
}

void TasksTestsBase::checkNoEos()
{
    EXPECT_TRUE(testContext->m_context.endOfStreamInfo.empty());
    EXPECT_FALSE(testContext->m_context.eosNotified);
}

void TasksTestsBase::shouldChangeStatePlayingSuccess()
{
    EXPECT_CALL(testContext->m_gstPlayer, changePipelineState(GST_STATE_PLAYING)).WillOnce(Return(true));
}

void TasksTestsBase::shouldChangeStatePlayingFailure()
{
    EXPECT_CALL(testContext->m_gstPlayer, changePipelineState(GST_STATE_PLAYING)).WillOnce(Return(false));
}

void TasksTestsBase::triggerPlay()
{
    firebolt::rialto::server::tasks::generic::Play task{testContext->m_gstPlayer};
    task.execute();
}

void TasksTestsBase::shouldPause()
{
    EXPECT_CALL(testContext->m_gstPlayer, stopPositionReportingAndCheckAudioUnderflowTimer());
    EXPECT_CALL(testContext->m_gstPlayer, changePipelineState(GST_STATE_PAUSED));
}

void TasksTestsBase::triggerPause()
{
    firebolt::rialto::server::tasks::generic::Pause task{testContext->m_context, testContext->m_gstPlayer};
    task.execute();
}

void TasksTestsBase::checkContextPaused()
{
    EXPECT_FALSE(testContext->m_context.isPlaying);
}

void TasksTestsBase::shouldReportPosition()
{
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstElementQueryPosition(&testContext->m_pipeline, GST_FORMAT_TIME, NotNullMatcher()))
        .WillOnce(Invoke(
            [this](GstElement *element, GstFormat format, gint64 *cur)
            {
                *cur = kPosition;
                return TRUE;
            }));
    EXPECT_CALL(testContext->m_gstPlayerClient, notifyPosition(kPosition));
}

void TasksTestsBase::triggerReportPosition()
{
    firebolt::rialto::server::tasks::generic::ReportPosition task{testContext->m_context, &testContext->m_gstPlayerClient,
                                                                  testContext->m_gstWrapper};
    task.execute();
}

void TasksTestsBase::shouldFailToReportPosition()
{
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstElementQueryPosition(&testContext->m_pipeline, GST_FORMAT_TIME, NotNullMatcher()))
        .WillOnce(Invoke(
            [this](GstElement *element, GstFormat format, gint64 *cur)
            {
                *cur = -1;
                return TRUE;
            }));
}

void TasksTestsBase::shouldFinishSetupSource()
{
    EXPECT_CALL(*testContext->m_gstSrc,
                setupAndAddAppArc(std::dynamic_pointer_cast<firebolt::rialto::server::IDecryptionService>(
                                      testContext->m_decryptionServiceMock)
                                      .get(),
                                  &testContext->m_element, testContext->m_streamInfoAudio, _, &testContext->m_gstPlayer,
                                  firebolt::rialto::MediaSourceType::AUDIO))
        .WillOnce(Invoke(
            [this](firebolt::rialto::server::IDecryptionService *decryptionService, GstElement *element,
                   firebolt::rialto::server::StreamInfo &streamInfo, const GstAppSrcCallbacks *callbacks,
                   gpointer userData, firebolt::rialto::MediaSourceType type)
            {
                testContext->m_audioCallbacks = *callbacks;
                testContext->m_audioUserData = userData;
            }));
    EXPECT_CALL(testContext->m_gstPlayer, notifyNeedMediaData(true, false));
    EXPECT_CALL(*testContext->m_gstSrc,
                setupAndAddAppArc(std::dynamic_pointer_cast<firebolt::rialto::server::IDecryptionService>(
                                      testContext->m_decryptionServiceMock)
                                      .get(),
                                  &testContext->m_element, testContext->m_streamInfoVideo, _, &testContext->m_gstPlayer,
                                  firebolt::rialto::MediaSourceType::VIDEO))
        .WillOnce(Invoke(
            [this](firebolt::rialto::server::IDecryptionService *decryptionService, GstElement *element,
                   firebolt::rialto::server::StreamInfo &streamInfo, const GstAppSrcCallbacks *callbacks,
                   gpointer userData, firebolt::rialto::MediaSourceType type)
            {
                testContext->m_videoCallbacks = *callbacks;
                testContext->m_videoUserData = userData;
            }));
    EXPECT_CALL(testContext->m_gstPlayer, notifyNeedMediaData(false, true));
    EXPECT_CALL(*testContext->m_gstSrc, allAppSrcsAdded(&testContext->m_element));
    EXPECT_CALL(testContext->m_gstPlayerClient, notifyPlaybackState(firebolt::rialto::PlaybackState::IDLE));
}

void TasksTestsBase::triggerFinishSetupSource()
{
    firebolt::rialto::server::tasks::generic::FinishSetupSource task{testContext->m_context, testContext->m_gstPlayer,
                                                                     &testContext->m_gstPlayerClient};
    task.execute();
}

void TasksTestsBase::shouldScheduleNeedMediaDataAudio()
{
    EXPECT_CALL(testContext->m_gstPlayer, scheduleNeedMediaData(GST_APP_SRC(&testContext->m_appSrcAudio)));
}

void TasksTestsBase::triggerAudioCallbackNeedData()
{
    ASSERT_TRUE(testContext->m_audioCallbacks.need_data);
    ASSERT_TRUE(testContext->m_audioUserData);
    ((void (*)(GstAppSrc *, guint,
               gpointer))testContext->m_audioCallbacks.need_data)(GST_APP_SRC(&testContext->m_appSrcAudio), kDataLength,
                                                                  testContext->m_audioUserData);
}

void TasksTestsBase::shouldScheduleNeedMediaDataVideo()
{
    EXPECT_CALL(testContext->m_gstPlayer, scheduleNeedMediaData(GST_APP_SRC(&testContext->m_appSrcVideo)));
}

void TasksTestsBase::triggerVideoCallbackNeedData()
{
    ASSERT_TRUE(testContext->m_videoCallbacks.need_data);
    ASSERT_TRUE(testContext->m_videoUserData);
    ((void (*)(GstAppSrc *, guint,
               gpointer))testContext->m_videoCallbacks.need_data)(GST_APP_SRC(&testContext->m_appSrcVideo), kDataLength,
                                                                  testContext->m_videoUserData);
}

void TasksTestsBase::shouldScheduleEnoughDataAudio()
{
    EXPECT_CALL(testContext->m_gstPlayer, scheduleEnoughData(GST_APP_SRC(&testContext->m_appSrcAudio)));
}

void TasksTestsBase::triggerAudioCallbackEnoughData()
{
    ASSERT_TRUE(testContext->m_audioCallbacks.enough_data);
    ASSERT_TRUE(testContext->m_audioUserData);
    ((void (*)(GstAppSrc *, gpointer))testContext->m_audioCallbacks.enough_data)(GST_APP_SRC(&testContext->m_appSrcAudio),
                                                                                 testContext->m_audioUserData);
}

void TasksTestsBase::shouldScheduleEnoughDataVideo()
{
    EXPECT_CALL(testContext->m_gstPlayer, scheduleEnoughData(GST_APP_SRC(&testContext->m_appSrcVideo)));
}

void TasksTestsBase::triggerVideoCallbackEnoughData()
{
    ASSERT_TRUE(testContext->m_videoCallbacks.enough_data);
    ASSERT_TRUE(testContext->m_videoUserData);
    ((void (*)(GstAppSrc *, gpointer))testContext->m_videoCallbacks.enough_data)(GST_APP_SRC(&testContext->m_appSrcVideo),
                                                                                 testContext->m_videoUserData);
}

void TasksTestsBase::triggerAudioCallbackSeekData()
{
    EXPECT_TRUE(testContext->m_audioCallbacks.seek_data);
    EXPECT_TRUE(testContext->m_audioUserData);
    ((gboolean(*)(GstAppSrc *, guint64,
                  gpointer))testContext->m_audioCallbacks.seek_data)(GST_APP_SRC(&testContext->m_appSrcAudio), kOffset,
                                                                     testContext->m_audioUserData);
}

void TasksTestsBase::triggerVideoCallbackSeekData()
{
    EXPECT_TRUE(testContext->m_videoCallbacks.seek_data);
    EXPECT_TRUE(testContext->m_videoUserData);
    ((gboolean(*)(GstAppSrc *, guint64,
                  gpointer))testContext->m_videoCallbacks.seek_data)(GST_APP_SRC(&testContext->m_appSrcVideo), kOffset,
                                                                     testContext->m_videoUserData);
}

void TasksTestsBase::checkSourcesAttached()
{
    EXPECT_TRUE(testContext->m_context.wereAllSourcesAttached);
}

void TasksTestsBase::triggerNeedDataAudio()
{
    firebolt::rialto::server::tasks::generic::NeedData task{testContext->m_context, &testContext->m_gstPlayerClient,
                                                            GST_APP_SRC(&testContext->m_appSrcAudio)};
    task.execute();
}

void TasksTestsBase::triggerNeedDataVideo()
{
    firebolt::rialto::server::tasks::generic::NeedData task{testContext->m_context, &testContext->m_gstPlayerClient,
                                                            GST_APP_SRC(&testContext->m_appSrcVideo)};
    task.execute();
}

void TasksTestsBase::triggerNeedDataUnknownSrc()
{
    GstAppSrc unknownSrc{};
    firebolt::rialto::server::tasks::generic::NeedData task{testContext->m_context, &testContext->m_gstPlayerClient,
                                                            &unknownSrc};
    task.execute();
}

void TasksTestsBase::shouldNotifyNeedAudioDataSuccess()
{
    EXPECT_CALL(testContext->m_gstPlayerClient, notifyNeedMediaData(firebolt::rialto::MediaSourceType::AUDIO))
        .WillOnce(Return(true));
}

void TasksTestsBase::shouldNotifyNeedVideoDataSuccess()
{
    EXPECT_CALL(testContext->m_gstPlayerClient, notifyNeedMediaData(firebolt::rialto::MediaSourceType::VIDEO))
        .WillOnce(Return(true));
}

void TasksTestsBase::checkNeedDataPendingForAudioOnly()
{
    EXPECT_TRUE(testContext->m_context.audioNeedDataPending);
    EXPECT_FALSE(testContext->m_context.videoNeedDataPending);
}

void TasksTestsBase::checkNeedDataPendingForVideoOnly()
{
    EXPECT_FALSE(testContext->m_context.audioNeedDataPending);
    EXPECT_TRUE(testContext->m_context.videoNeedDataPending);
}

void TasksTestsBase::shouldNotifyNeedAudioDataFailure()
{
    EXPECT_CALL(testContext->m_gstPlayerClient, notifyNeedMediaData(firebolt::rialto::MediaSourceType::AUDIO))
        .WillOnce(Return(false));
}

void TasksTestsBase::shouldNotifyNeedVideoDataFailure()
{
    EXPECT_CALL(testContext->m_gstPlayerClient, notifyNeedMediaData(firebolt::rialto::MediaSourceType::VIDEO))
        .WillOnce(Return(false));
}

void TasksTestsBase::triggerSetPlaybackRate()
{
    firebolt::rialto::server::tasks::generic::SetPlaybackRate task{testContext->m_context, testContext->m_gstWrapper,
                                                                   testContext->m_glibWrapper, kRate};
    task.execute();
}

void TasksTestsBase::checkNoPendingPlaybackRate()
{
    EXPECT_EQ(testContext->m_context.pendingPlaybackRate, firebolt::rialto::server::kNoPendingPlaybackRate);
}

void TasksTestsBase::checkPendingPlaybackRate()
{
    EXPECT_EQ(testContext->m_context.pendingPlaybackRate, kRate);
}

void TasksTestsBase::checkPlaybackRateSet()
{
    EXPECT_EQ(testContext->m_context.playbackRate, kRate);
}

void TasksTestsBase::checkPlaybackRateDefault()
{
    EXPECT_EQ(testContext->m_context.playbackRate, 1.0);
}

void TasksTestsBase::setPipelinePlaying()
{
    GST_STATE(&testContext->m_pipeline) = GST_STATE_PLAYING;
}

void TasksTestsBase::shouldSetPlaybackRateAudioSinkNullSuccess()
{
    EXPECT_CALL(*testContext->m_glibWrapper, gObjectGetStub(_, CharStrMatcher("audio-sink"), _));
    EXPECT_CALL(*testContext->m_gstWrapper, gstStructureNewDoubleStub(CharStrMatcher("custom-instant-rate-change"),
                                                                      CharStrMatcher("rate"), G_TYPE_DOUBLE, kRate))
        .WillOnce(Return(&testContext->m_structure));
    EXPECT_CALL(*testContext->m_gstWrapper, gstEventNewCustom(GST_EVENT_CUSTOM_DOWNSTREAM_OOB, &testContext->m_structure))
        .WillOnce(Return(&testContext->m_event));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementSendEvent(_, &testContext->m_event)).WillOnce(Return(TRUE));
}

void TasksTestsBase::shouldSetPlaybackRateAudioSinkNullFailure()
{
    EXPECT_CALL(*testContext->m_glibWrapper, gObjectGetStub(_, CharStrMatcher("audio-sink"), _));
    EXPECT_CALL(*testContext->m_gstWrapper, gstStructureNewDoubleStub(CharStrMatcher("custom-instant-rate-change"),
                                                                      CharStrMatcher("rate"), G_TYPE_DOUBLE, kRate))
        .WillOnce(Return(&testContext->m_structure));
    EXPECT_CALL(*testContext->m_gstWrapper, gstEventNewCustom(GST_EVENT_CUSTOM_DOWNSTREAM_OOB, &testContext->m_structure))
        .WillOnce(Return(&testContext->m_event));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementSendEvent(_, &testContext->m_event)).WillOnce(Return(FALSE));
}

void TasksTestsBase::shouldSetPlaybackRateAudioSinkOtherThanAmlhala()
{
    EXPECT_CALL(*testContext->m_glibWrapper, gObjectGetStub(_, CharStrMatcher("audio-sink"), _))
        .WillOnce(Invoke(
            [&](gpointer object, const gchar *first_property_name, void *element)
            {
                GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                *elementPtr = &testContext->m_element;
            }));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrHasPrefix(_, CharStrMatcher("amlhalasink"))).WillOnce(Return(FALSE));
    EXPECT_CALL(*testContext->m_gstWrapper, gstStructureNewDoubleStub(CharStrMatcher("custom-instant-rate-change"),
                                                                      CharStrMatcher("rate"), G_TYPE_DOUBLE, kRate))
        .WillOnce(Return(&testContext->m_structure));
    EXPECT_CALL(*testContext->m_gstWrapper, gstEventNewCustom(GST_EVENT_CUSTOM_DOWNSTREAM_OOB, &testContext->m_structure))
        .WillOnce(Return(&testContext->m_event));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementSendEvent(_, &testContext->m_event)).WillOnce(Return(TRUE));
    EXPECT_CALL(*testContext->m_glibWrapper, gObjectUnref(&testContext->m_element));
}

void TasksTestsBase::shouldFailToSetPlaybackRateAudioSinkOtherThanAmlhala()
{
    EXPECT_CALL(*testContext->m_glibWrapper, gObjectGetStub(_, CharStrMatcher("audio-sink"), _))
        .WillOnce(Invoke(
            [&](gpointer object, const gchar *first_property_name, void *element)
            {
                GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                *elementPtr = &testContext->m_element;
            }));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrHasPrefix(_, CharStrMatcher("amlhalasink"))).WillOnce(Return(FALSE));
    EXPECT_CALL(*testContext->m_gstWrapper, gstStructureNewDoubleStub(CharStrMatcher("custom-instant-rate-change"),
                                                                      CharStrMatcher("rate"), G_TYPE_DOUBLE, kRate))
        .WillOnce(Return(&testContext->m_structure));
    EXPECT_CALL(*testContext->m_gstWrapper, gstEventNewCustom(GST_EVENT_CUSTOM_DOWNSTREAM_OOB, &testContext->m_structure))
        .WillOnce(Return(&testContext->m_event));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementSendEvent(_, &testContext->m_event)).WillOnce(Return(FALSE));
    EXPECT_CALL(*testContext->m_glibWrapper, gObjectUnref(&testContext->m_element));
}

void TasksTestsBase::shouldSetPlaybackRateAmlhalaAudioSink()
{
    EXPECT_CALL(*testContext->m_glibWrapper, gObjectGetStub(_, CharStrMatcher("audio-sink"), _))
        .WillOnce(Invoke(
            [&](gpointer object, const gchar *first_property_name, void *element)
            {
                GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                *elementPtr = &testContext->m_element;
            }));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrHasPrefix(_, CharStrMatcher("amlhalasink"))).WillOnce(Return(TRUE));
    EXPECT_CALL(*testContext->m_gstWrapper, gstSegmentNew()).WillOnce(Return(&testContext->m_segment));
    EXPECT_CALL(*testContext->m_gstWrapper, gstSegmentInit(&testContext->m_segment, GST_FORMAT_TIME));
    EXPECT_CALL(*testContext->m_gstWrapper, gstEventNewSegment(&testContext->m_segment))
        .WillOnce(Return(&testContext->m_event));
    EXPECT_CALL(*testContext->m_gstWrapper, gstPadSendEvent(_, &testContext->m_event)).WillOnce(Return(TRUE));
    EXPECT_CALL(*testContext->m_gstWrapper, gstSegmentFree(&testContext->m_segment));
    EXPECT_CALL(*testContext->m_glibWrapper, gObjectUnref(&testContext->m_element));
}

void TasksTestsBase::shouldFailToSetPlaybackRateAmlhalaAudioSink()
{
    EXPECT_CALL(*testContext->m_glibWrapper, gObjectGetStub(_, CharStrMatcher("audio-sink"), _))
        .WillOnce(Invoke(
            [&](gpointer object, const gchar *first_property_name, void *element)
            {
                GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                *elementPtr = &testContext->m_element;
            }));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrHasPrefix(_, CharStrMatcher("amlhalasink"))).WillOnce(Return(TRUE));
    EXPECT_CALL(*testContext->m_gstWrapper, gstSegmentNew()).WillOnce(Return(&testContext->m_segment));
    EXPECT_CALL(*testContext->m_gstWrapper, gstSegmentInit(&testContext->m_segment, GST_FORMAT_TIME));
    EXPECT_CALL(*testContext->m_gstWrapper, gstEventNewSegment(&testContext->m_segment))
        .WillOnce(Return(&testContext->m_event));
    EXPECT_CALL(*testContext->m_gstWrapper, gstPadSendEvent(_, &testContext->m_event)).WillOnce(Return(FALSE));
    EXPECT_CALL(*testContext->m_gstWrapper, gstSegmentFree(&testContext->m_segment));
    EXPECT_CALL(*testContext->m_glibWrapper, gObjectUnref(&testContext->m_element));
}

void TasksTestsBase::checkSegmentInfo()
{
    EXPECT_EQ(testContext->m_segment.rate, kRate);
    EXPECT_EQ(testContext->m_segment.start, GST_CLOCK_TIME_NONE);
    EXPECT_EQ(testContext->m_segment.position, GST_CLOCK_TIME_NONE);
}

void TasksTestsBase::shouldRenderFrame()
{
    EXPECT_CALL(*testContext->m_glibWrapper, gObjectGetStub(&testContext->m_pipeline, StrEq("video-sink"), _))
        .WillOnce(Invoke(
            [&](gpointer object, const gchar *first_property_name, void *element)
            {
                GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                *elementPtr = &testContext->m_element;
            }));

    EXPECT_CALL(*testContext->m_glibWrapper,
                gObjectClassFindProperty(G_OBJECT_GET_CLASS(&testContext->m_element), StrEq("frame-step-on-preroll")))
        .WillOnce(Return(&testContext->m_paramSpec));
    EXPECT_CALL(*testContext->m_glibWrapper, gObjectSetStub(&testContext->m_element, StrEq("frame-step-on-preroll")))
        .Times(2);
    EXPECT_CALL(*testContext->m_gstWrapper, gstEventNewStep(GST_FORMAT_BUFFERS, 1, 1.0, true, false))
        .WillOnce(Return(&testContext->m_event));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementSendEvent(&testContext->m_element, &testContext->m_event));
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectUnref(GST_OBJECT(&testContext->m_element)));
}

void TasksTestsBase::triggerRenderFrame()
{
    firebolt::rialto::server::tasks::generic::RenderFrame task{testContext->m_context, testContext->m_gstWrapper,
                                                               testContext->m_glibWrapper};
    task.execute();
}

void TasksTestsBase::shouldGetVideoSinkFailure()
{
    EXPECT_CALL(*testContext->m_glibWrapper, gObjectGetStub(&testContext->m_pipeline, StrEq("video-sink"), _))
        .WillOnce(Invoke(
            [&](gpointer object, const gchar *first_property_name, void *element)
            {
                GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                *elementPtr = nullptr;
            }));
}

void TasksTestsBase::shouldFindPropertyFailure()
{
    EXPECT_CALL(*testContext->m_glibWrapper, gObjectGetStub(&testContext->m_pipeline, StrEq("video-sink"), _))
        .WillOnce(Invoke(
            [&](gpointer object, const gchar *first_property_name, void *element)
            {
                GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                *elementPtr = &testContext->m_element;
            }));

    EXPECT_CALL(*testContext->m_glibWrapper,
                gObjectClassFindProperty(G_OBJECT_GET_CLASS(&testContext->m_element), StrEq("frame-step-on-preroll")))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectUnref(GST_OBJECT(&testContext->m_element)));
}

void TasksTestsBase::shouldInvalidateActiveAudioRequests()
{
    EXPECT_CALL(testContext->m_gstPlayerClient, invalidateActiveRequests(firebolt::rialto::MediaSourceType::AUDIO));
}

void TasksTestsBase::triggerRemoveSourceAudio()
{
    firebolt::rialto::server::tasks::generic::RemoveSource task{testContext->m_context, &testContext->m_gstPlayerClient,
                                                                testContext->m_gstWrapper,
                                                                firebolt::rialto::MediaSourceType::AUDIO};
    task.execute();
}

void TasksTestsBase::triggerRemoveSourceVideo()
{
    firebolt::rialto::server::tasks::generic::RemoveSource task{testContext->m_context, &testContext->m_gstPlayerClient,
                                                                testContext->m_gstWrapper,
                                                                firebolt::rialto::MediaSourceType::VIDEO};
    task.execute();
}

void TasksTestsBase::checkAudioSourceRemoved()
{
    EXPECT_TRUE(testContext->m_context.audioSourceRemoved);
}

void TasksTestsBase::checkAudioSourceNotRemoved()
{
    EXPECT_FALSE(testContext->m_context.audioSourceRemoved);
}

void TasksTestsBase::shouldFlushAudioSrcSuccess()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstEventNewFlushStart()).WillOnce(Return(&testContext->m_event));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementSendEvent(&testContext->m_appSrcAudio, &testContext->m_event))
        .WillOnce(Return(TRUE));
    EXPECT_CALL(*testContext->m_gstWrapper, gstEventNewFlushStop(FALSE)).WillOnce(Return(&testContext->m_event2));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementSendEvent(&testContext->m_appSrcAudio, &testContext->m_event2))
        .WillOnce(Return(TRUE));
}

void TasksTestsBase::shouldFlushAudioSrcFailure()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstEventNewFlushStart()).WillOnce(Return(&testContext->m_event));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementSendEvent(&testContext->m_appSrcAudio, &testContext->m_event))
        .WillOnce(Return(FALSE));
    EXPECT_CALL(*testContext->m_gstWrapper, gstEventNewFlushStop(FALSE)).WillOnce(Return(&testContext->m_event2));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementSendEvent(&testContext->m_appSrcAudio, &testContext->m_event2))
        .WillOnce(Return(FALSE));
}

void TasksTestsBase::shouldReadAudioData()
{
    EXPECT_CALL(*testContext->m_dataReader, readData()).WillOnce(Invoke([&]() { return std::move(buildAudioSamples()); }));
}

void TasksTestsBase::shouldReadVideoData()
{
    EXPECT_CALL(*testContext->m_dataReader, readData()).WillOnce(Invoke([&]() { return std::move(buildVideoSamples()); }));
}

void TasksTestsBase::triggerReadShmDataAndAttachSamplesAudio()
{
    firebolt::rialto::server::tasks::generic::ReadShmDataAndAttachSamples task{testContext->m_context,
                                                                               testContext->m_gstPlayer,
                                                                               testContext->m_dataReader};
    task.execute();
    EXPECT_EQ(testContext->m_context.audioBuffers.size(), 2);
}

void TasksTestsBase::triggerReadShmDataAndAttachSamplesVideo()
{
    firebolt::rialto::server::tasks::generic::ReadShmDataAndAttachSamples task{testContext->m_context,
                                                                               testContext->m_gstPlayer,
                                                                               testContext->m_dataReader};
    task.execute();
    EXPECT_EQ(testContext->m_context.videoBuffers.size(), 2);
}
