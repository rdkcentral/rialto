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

#include "GenericTasksTestsBase.h"
#include "GenericTasksTestsContext.h"
#include "HeartbeatHandlerMock.h"
#include "IMediaPipeline.h"
#include "Matchers.h"
#include "MatchersGenericPlayer.h"
#include "tasks/generic/AttachSamples.h"
#include "tasks/generic/AttachSource.h"
#include "tasks/generic/CheckAudioUnderflow.h"
#include "tasks/generic/DeepElementAdded.h"
#include "tasks/generic/EnoughData.h"
#include "tasks/generic/Eos.h"
#include "tasks/generic/FinishSetupSource.h"
#include "tasks/generic/Flush.h"
#include "tasks/generic/NeedData.h"
#include "tasks/generic/Pause.h"
#include "tasks/generic/Ping.h"
#include "tasks/generic/Play.h"
#include "tasks/generic/ProcessAudioGap.h"
#include "tasks/generic/ReadShmDataAndAttachSamples.h"
#include "tasks/generic/RemoveSource.h"
#include "tasks/generic/RenderFrame.h"
#include "tasks/generic/ReportPosition.h"
#include "tasks/generic/SetImmediateOutput.h"
#include "tasks/generic/SetLowLatency.h"
#include "tasks/generic/SetMute.h"
#include "tasks/generic/SetPlaybackRate.h"
#include "tasks/generic/SetPosition.h"
#include "tasks/generic/SetSourcePosition.h"
#include "tasks/generic/SetStreamSyncMode.h"
#include "tasks/generic/SetSync.h"
#include "tasks/generic/SetSyncOff.h"
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
std::shared_ptr<GenericTasksTestsContext> testContext;

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
constexpr uint64_t kClippingStart{1024};
constexpr uint64_t kClippingEnd{2048};
constexpr int32_t kWidth{1024};
constexpr int32_t kHeight{768};
const std::string kTextTrackIdentifier{"SERVICE1"};
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
const std::string kAutoVideoSinkTypeName{"GstAutoVideoSink"};
const std::string kElementTypeName{"GenericSink"};
constexpr bool kResetTime{false};
constexpr int32_t kId{0};
constexpr firebolt::rialto::Layout kLayout{firebolt::rialto::Layout::INTERLEAVED};
constexpr firebolt::rialto::Format kFormat{firebolt::rialto::Format::S16LE};
constexpr uint64_t kChannelMask{0x0000000000000003};
constexpr int64_t kDiscontinuityGap{1};
constexpr bool kIsAudioAac{false};

firebolt::rialto::IMediaPipeline::MediaSegmentVector buildAudioSamples()
{
    firebolt::rialto::IMediaPipeline::MediaSegmentVector dataVec;
    dataVec.emplace_back(
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSegmentAudio>(kAudioSourceId, kItHappenedInThePast,
                                                                              kDuration, kSampleRate, kNumberOfChannels,
                                                                              kClippingStart, kClippingEnd));
    dataVec.emplace_back(
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSegmentAudio>(kAudioSourceId, kItWillHappenInTheFuture,
                                                                              kDuration, kSampleRate, kNumberOfChannels,
                                                                              kClippingStart, kClippingEnd));
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

/**
 * Class to test the cast of AudioSource
 */
class MediaAudioSourceTest : public IMediaPipeline::MediaSource
{
public:
    MediaAudioSourceTest(SourceConfigType configType, int id) : IMediaPipeline::MediaSource(configType) {}
    ~MediaAudioSourceTest() {}

    MediaSourceType getType() const override { return MediaSourceType::AUDIO; }
    std::unique_ptr<MediaSource> copy() const override { return std::make_unique<MediaAudioSourceTest>(*this); }
};

/**
 * Class to test the cast of VideoSource
 */
class MediaVideoSourceTest : public IMediaPipeline::MediaSource
{
public:
    MediaVideoSourceTest(SourceConfigType configType, int id) : IMediaPipeline::MediaSource(configType) {}
    ~MediaVideoSourceTest() {}

    MediaSourceType getType() const override { return MediaSourceType::VIDEO; }
    std::unique_ptr<MediaSource> copy() const override { return std::make_unique<MediaVideoSourceTest>(*this); }
};

/**
 * Class to test the cast of DolbyVisionSource
 */
class MediaVideoDolbyVisionSourceTest : public IMediaPipeline::MediaSourceVideo
{
public:
    MediaVideoDolbyVisionSourceTest(SourceConfigType configType, const std::string &mimeType, int32_t dolbyVisionProfile)
        : IMediaPipeline::MediaSourceVideo(configType, mimeType, true, firebolt::rialto::kUndefinedSize,
                                           firebolt::rialto::kUndefinedSize, SegmentAlignment::UNDEFINED,
                                           StreamFormat::UNDEFINED, nullptr)
    {
    }
    ~MediaVideoDolbyVisionSourceTest() {}

    MediaSourceType getType() const override { return MediaSourceType::VIDEO; }
    std::unique_ptr<MediaSource> copy() const override
    {
        return std::make_unique<MediaVideoDolbyVisionSourceTest>(*this);
    }
};

GenericTasksTestsBase::GenericTasksTestsBase()
{
    testContext = std::make_shared<GenericTasksTestsContext>();

    testContext->m_elementFactory = gst_element_factory_find("fakesrc");
    testContext->m_element = gst_element_factory_create(testContext->m_elementFactory, nullptr);
    testContext->m_context.pipeline = &testContext->m_pipeline;
    testContext->m_context.audioAppSrc = &testContext->m_appSrcAudio;
    testContext->m_context.gstSrc = testContext->m_gstSrc;
    testContext->m_context.source = testContext->m_element;
    testContext->m_context.decryptionService = testContext->m_decryptionServiceMock.get();
}

GenericTasksTestsBase::~GenericTasksTestsBase()
{
    gst_object_unref(testContext->m_element);
    gst_object_unref(testContext->m_elementFactory);

    testContext.reset();
}

void GenericTasksTestsBase::setContextStreamInfo(firebolt::rialto::MediaSourceType sourceType)
{
    firebolt::rialto::server::StreamInfo &streamInfo = (sourceType == firebolt::rialto::MediaSourceType::AUDIO
                                                            ? testContext->m_streamInfoAudio
                                                            : testContext->m_streamInfoVideo);
    testContext->m_context.streamInfo.emplace(sourceType, streamInfo);
}

void GenericTasksTestsBase::setContextPlaying()
{
    testContext->m_context.isPlaying = true;
}

void GenericTasksTestsBase::setContextNeedData(bool doNeedData)
{
    testContext->m_context.audioNeedData = doNeedData;
    testContext->m_context.videoNeedData = doNeedData;
}

void GenericTasksTestsBase::setContextAudioUnderflowOccured(bool isUnderflow)
{
    testContext->m_context.audioUnderflowOccured = isUnderflow;
}

void GenericTasksTestsBase::setContextVideoUnderflowOccured(bool isUnderflow)
{
    testContext->m_context.videoUnderflowOccured = isUnderflow;
}

void GenericTasksTestsBase::setContextAudioAppSrc()
{
    testContext->m_context.audioAppSrc = &testContext->m_appSrcAudio;
}

void GenericTasksTestsBase::setContextEndOfStream(firebolt::rialto::MediaSourceType sourceType)
{
    GstElement *appSrc = (sourceType == firebolt::rialto::MediaSourceType::AUDIO ? &testContext->m_appSrcAudio
                                                                                 : &testContext->m_appSrcVideo);
    testContext->m_context.endOfStreamInfo.emplace(sourceType, appSrc);
}

void GenericTasksTestsBase::setContextEndOfStreamNotified()
{
    testContext->m_context.eosNotified = true;
}

void GenericTasksTestsBase::setContextPipelineNull()
{
    testContext->m_context.pipeline = nullptr;
}

void GenericTasksTestsBase::setContextNeedDataPending(bool isNeedDataPending)
{
    testContext->m_context.audioNeedDataPending = isNeedDataPending;
    testContext->m_context.videoNeedDataPending = isNeedDataPending;
}

void GenericTasksTestsBase::setContextNeedDataPendingAudioOnly(bool isNeedDataPending)
{
    testContext->m_context.audioNeedDataPending = isNeedDataPending;
}

void GenericTasksTestsBase::setContextNeedDataPendingVideoOnly(bool isNeedDataPending)
{
    testContext->m_context.videoNeedDataPending = isNeedDataPending;
}

void GenericTasksTestsBase::setContextAudioBuffer()
{
    testContext->m_context.audioBuffers.emplace_back(&testContext->m_audioBuffer);
}

void GenericTasksTestsBase::setContextVideoBuffer()
{
    testContext->m_context.videoBuffers.emplace_back(&testContext->m_videoBuffer);
}

void GenericTasksTestsBase::setContextPlaybackRate()
{
    testContext->m_context.playbackRate = kRate;
}

void GenericTasksTestsBase::setContextSourceNull()
{
    testContext->m_context.source = nullptr;
}

void GenericTasksTestsBase::setContextAudioSourceRemoved()
{
    testContext->m_context.audioSourceRemoved = true;
}

void GenericTasksTestsBase::setContextStreamInfoEmpty()
{
    testContext->m_context.streamInfo.clear();
}

void GenericTasksTestsBase::setContextNeedDataAudioOnly()
{
    testContext->m_context.audioNeedData = true;
}

void GenericTasksTestsBase::setContextSetupSourceFinished()
{
    testContext->m_context.setupSourceFinished = true;
}

void GenericTasksTestsBase::expectSetupVideoElement()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetFactory(_)).WillRepeatedly(Return(testContext->m_elementFactory));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstElementFactoryListIsType(testContext->m_elementFactory, GST_ELEMENT_FACTORY_TYPE_DECODER))
        .WillOnce(Return(TRUE));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstElementFactoryListIsType(testContext->m_elementFactory, GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO))
        .WillOnce(Return(TRUE));
    EXPECT_CALL(*testContext->m_glibWrapper, gObjectType(testContext->m_element)).WillRepeatedly(Return(G_TYPE_PARAM));
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
    EXPECT_CALL(*testContext->m_glibWrapper, gSignalConnect(_, StrEq("buffer-underflow-callback"), _, _))
        .WillOnce(Invoke(
            [&](gpointer instance, const gchar *detailed_signal, GCallback c_handler, gpointer data)
            {
                testContext->m_videoUnderflowCallback = c_handler;
                return kSignalId;
            }));
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectUnref(_));
}

void GenericTasksTestsBase::expectSetupAudioElement()
{
    EXPECT_CALL(*testContext->m_glibWrapper, gTypeName(G_OBJECT_TYPE(testContext->m_element)))
        .WillOnce(Return(kElementTypeName.c_str()));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstElementFactoryListIsType(testContext->m_elementFactory,
                                            GST_ELEMENT_FACTORY_TYPE_SINK | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO))
        .WillOnce(Return(FALSE));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrHasPrefix(_, StrEq("amlhalasink"))).WillOnce(Return(FALSE));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrHasPrefix(_, StrEq("brcmaudiosink"))).WillOnce(Return(FALSE));
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
    EXPECT_CALL(*testContext->m_glibWrapper, gObjectType(testContext->m_element)).WillRepeatedly(Return(G_TYPE_PARAM));
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

void GenericTasksTestsBase::shouldSetupVideoElementOnly()
{
    EXPECT_CALL(*testContext->m_glibWrapper, gTypeName(G_OBJECT_TYPE(testContext->m_element)))
        .WillOnce(Return(kElementTypeName.c_str()));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstElementFactoryListIsType(testContext->m_elementFactory,
                                            GST_ELEMENT_FACTORY_TYPE_SINK | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO))
        .WillOnce(Return(TRUE));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrHasPrefix(_, StrEq("amlhalasink"))).WillOnce(Return(FALSE));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrHasPrefix(_, StrEq("brcmaudiosink"))).WillOnce(Return(FALSE));
    expectSetupVideoElement();
}

void GenericTasksTestsBase::shouldSetupVideoElementWithPendingGeometry()
{
    testContext->m_context.pendingGeometry = kRectangle;
    EXPECT_CALL(*testContext->m_glibWrapper, gTypeName(G_OBJECT_TYPE(testContext->m_element)))
        .WillOnce(Return(kElementTypeName.c_str()));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstElementFactoryListIsType(testContext->m_elementFactory,
                                            GST_ELEMENT_FACTORY_TYPE_SINK | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO))
        .WillOnce(Return(TRUE));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrHasPrefix(_, StrEq("amlhalasink"))).WillOnce(Return(FALSE));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrHasPrefix(_, StrEq("brcmaudiosink"))).WillOnce(Return(FALSE));
    EXPECT_CALL(testContext->m_gstPlayer, setVideoSinkRectangle());
    expectSetupVideoElement();
}

void GenericTasksTestsBase::shouldSetupVideoElementAmlhalasink()
{
    EXPECT_CALL(*testContext->m_glibWrapper, gTypeName(G_OBJECT_TYPE(testContext->m_element)))
        .WillOnce(Return(kElementTypeName.c_str()));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstElementFactoryListIsType(testContext->m_elementFactory,
                                            GST_ELEMENT_FACTORY_TYPE_SINK | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO))
        .WillOnce(Return(TRUE));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrHasPrefix(_, StrEq("amlhalasink"))).WillOnce(Return(true));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrHasPrefix(_, StrEq("brcmaudiosink"))).WillOnce(Return(FALSE));
    EXPECT_CALL(*testContext->m_glibWrapper, gObjectSetStub(G_OBJECT(testContext->m_element), StrEq("wait-video")));
    EXPECT_CALL(*testContext->m_glibWrapper, gObjectSetStub(G_OBJECT(testContext->m_element), StrEq("a-wait-timeout")));
    EXPECT_CALL(*testContext->m_glibWrapper, gObjectSetStub(G_OBJECT(testContext->m_element), StrEq("disable-xrun")));
    expectSetupVideoElement();
}

void GenericTasksTestsBase::shouldSetupAudioElementBrcmAudioSink()
{
    EXPECT_CALL(*testContext->m_glibWrapper, gTypeName(G_OBJECT_TYPE(testContext->m_element)))
        .WillOnce(Return(kElementTypeName.c_str()));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstElementFactoryListIsType(testContext->m_elementFactory,
                                            GST_ELEMENT_FACTORY_TYPE_SINK | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO))
        .WillOnce(Return(FALSE));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrHasPrefix(_, StrEq("amlhalasink"))).WillOnce(Return(false));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrHasPrefix(_, StrEq("brcmaudiosink"))).WillOnce(Return(true));
    EXPECT_CALL(*testContext->m_glibWrapper, gObjectSetStub(G_OBJECT(testContext->m_element), StrEq("async")));
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
    EXPECT_CALL(*testContext->m_glibWrapper, gObjectType(testContext->m_element)).WillRepeatedly(Return(G_TYPE_PARAM));
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

void GenericTasksTestsBase::shouldSetupVideoElementAutoVideoSinkWithMultipleChildren()
{
    testContext->m_iterator.size = 2;
    shouldSetupVideoElementAutoVideoSink();
}

void GenericTasksTestsBase::shouldSetupVideoElementAutoVideoSink()
{
    EXPECT_CALL(*testContext->m_glibWrapper, gTypeName(G_OBJECT_TYPE(testContext->m_element)))
        .WillOnce(Return(kAutoVideoSinkTypeName.c_str()));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrHasPrefix(_, StrEq("amlhalasink"))).WillOnce(Return(false));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrHasPrefix(_, StrEq("brcmaudiosink"))).WillOnce(Return(FALSE));
    EXPECT_CALL(*testContext->m_glibWrapper,
                gSignalConnect(G_OBJECT(testContext->m_element), StrEq("child-added"), _, &testContext->m_gstPlayer))
        .WillOnce(Invoke(
            [&](gpointer instance, const gchar *detailed_signal, GCallback c_handler, gpointer data)
            {
                testContext->m_childAddedCallback = c_handler;
                return kSignalId;
            }));
    EXPECT_CALL(*testContext->m_glibWrapper,
                gSignalConnect(G_OBJECT(testContext->m_element), StrEq("child-removed"), _, &testContext->m_gstPlayer))
        .WillOnce(Invoke(
            [&](gpointer instance, const gchar *detailed_signal, GCallback c_handler, gpointer data)
            {
                testContext->m_childRemovedCallback = c_handler;
                return kSignalId;
            }));
    EXPECT_CALL(*testContext->m_gstWrapper, gstBinIterateSinks(GST_BIN(testContext->m_element)))
        .WillOnce(Return(&testContext->m_iterator));
    EXPECT_CALL(*testContext->m_glibWrapper, gValueUnset(_));
    EXPECT_CALL(*testContext->m_gstWrapper, gstIteratorFree(&testContext->m_iterator));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstElementFactoryListIsType(testContext->m_elementFactory,
                                            GST_ELEMENT_FACTORY_TYPE_SINK | GST_ELEMENT_FACTORY_TYPE_MEDIA_VIDEO))
        .WillOnce(Return(TRUE));
    expectSetupVideoElement();
}

void GenericTasksTestsBase::shouldSetupAudioElementOnly()
{
    expectSetupAudioElement();
}

void GenericTasksTestsBase::shouldSetVideoUnderflowCallback()
{
    ASSERT_TRUE(testContext->m_videoUnderflowCallback);
    EXPECT_CALL(testContext->m_gstPlayer, scheduleVideoUnderflow());
}

void GenericTasksTestsBase::triggerVideoUnderflowCallback()
{
    ((void (*)(GstElement *, guint, gpointer, gpointer))testContext->m_videoUnderflowCallback)(testContext->m_element,
                                                                                               0, nullptr,
                                                                                               &testContext->m_gstPlayer);
}

void GenericTasksTestsBase::shouldSetAudioUnderflowCallback()
{
    ASSERT_TRUE(testContext->m_audioUnderflowCallback);
    EXPECT_CALL(testContext->m_gstPlayer, scheduleAudioUnderflow());
}

void GenericTasksTestsBase::triggerAudioUnderflowCallback()
{
    ((void (*)(GstElement *, guint, gpointer, gpointer))testContext->m_audioUnderflowCallback)(testContext->m_element,
                                                                                               0, nullptr,
                                                                                               &testContext->m_gstPlayer);
}

void GenericTasksTestsBase::shouldAddAutoVideoSinkChildCallback()
{
    EXPECT_CALL(testContext->m_gstPlayer, addAutoVideoSinkChild(&testContext->m_gObj));
}

void GenericTasksTestsBase::triggerAutoVideoSinkChildAddedCallback()
{
    ASSERT_TRUE(testContext->m_childAddedCallback);
    ((void (*)(GstChildProxy * obj, GObject * object, gchar * name, gpointer self))
         testContext->m_childAddedCallback)(reinterpret_cast<GstChildProxy *>(testContext->m_element),
                                            &testContext->m_gObj, "GstAutoVideoSink", &testContext->m_gstPlayer);
}

void GenericTasksTestsBase::shouldRemoveAutoVideoSinkChildCallback()
{
    EXPECT_CALL(testContext->m_gstPlayer, removeAutoVideoSinkChild(&testContext->m_gObj));
}

void GenericTasksTestsBase::triggerAutoVideoSinkChildRemovedCallback()
{
    ASSERT_TRUE(testContext->m_childRemovedCallback);
    ((void (*)(GstChildProxy * obj, GObject * object, gchar * name, gpointer self))
         testContext->m_childRemovedCallback)(reinterpret_cast<GstChildProxy *>(testContext->m_element),
                                              &testContext->m_gObj, "GstAutoVideoSink", &testContext->m_gstPlayer);
}

void GenericTasksTestsBase::triggerSetupElement()
{
    firebolt::rialto::server::tasks::generic::SetupElement task{testContext->m_context, testContext->m_gstWrapper,
                                                                testContext->m_glibWrapper, testContext->m_gstPlayer,
                                                                testContext->m_element};
    task.execute();
}

void GenericTasksTestsBase::setPipelineToNull()
{
    testContext->m_context.pipeline = nullptr;
}

void GenericTasksTestsBase::triggerSetVideoGeometryFailure()
{
    firebolt::rialto::server::tasks::generic::SetVideoGeometry task{testContext->m_context, testContext->m_gstPlayer,
                                                                    kRectangle};
    task.execute();
    EXPECT_EQ(testContext->m_context.pendingGeometry, kRectangle);
}

void GenericTasksTestsBase::shouldSetVideoGeometry()
{
    EXPECT_CALL(testContext->m_gstPlayer, setVideoSinkRectangle());
}

void GenericTasksTestsBase::shouldAddFirstAutoVideoSinkChild()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstIteratorNext(&testContext->m_iterator, _))
        .WillOnce(DoAll(SetArgPointee<1>(testContext->m_value), Return(GST_ITERATOR_OK)));
    EXPECT_CALL(*testContext->m_glibWrapper, gValueGetObject(_)).WillOnce(Return(testContext->m_element));
    EXPECT_CALL(testContext->m_gstPlayer, addAutoVideoSinkChild(reinterpret_cast<GObject *>(testContext->m_element)));
}

void GenericTasksTestsBase::shouldNotAddAutoVideoSinkChild()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstIteratorNext(&testContext->m_iterator, _))
        .WillOnce(DoAll(SetArgPointee<1>(testContext->m_value), Return(GST_ITERATOR_ERROR)));
}

void GenericTasksTestsBase::triggerSetVideoGeometrySuccess()
{
    firebolt::rialto::server::tasks::generic::SetVideoGeometry task{testContext->m_context, testContext->m_gstPlayer,
                                                                    kRectangle};
    task.execute();
    EXPECT_EQ(testContext->m_context.pendingGeometry, kRectangle);
}

void GenericTasksTestsBase::setAllSourcesAttached()
{
    testContext->m_context.wereAllSourcesAttached = true;
}

void GenericTasksTestsBase::shouldScheduleAllSourcesAttached()
{
    EXPECT_CALL(testContext->m_gstPlayer, scheduleAllSourcesAttached());
}

void GenericTasksTestsBase::triggerSetupSource()
{
    firebolt::rialto::server::tasks::generic::SetupSource task{testContext->m_context, testContext->m_gstPlayer,
                                                               testContext->m_element};
    task.execute();
    EXPECT_EQ(testContext->m_context.source, testContext->m_element);
}

void GenericTasksTestsBase::shouldSetGstVolume()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstStreamVolumeSetVolume(_, GST_STREAM_VOLUME_FORMAT_LINEAR, kVolume));
}

void GenericTasksTestsBase::triggerSetVolume()
{
    firebolt::rialto::server::tasks::generic::SetVolume task{testContext->m_context, testContext->m_gstWrapper, kVolume};
    task.execute();
}

void GenericTasksTestsBase::shouldAttachAllAudioSamples()
{
    std::shared_ptr<firebolt::rialto::CodecData> kNullCodecData{};
    EXPECT_CALL(testContext->m_gstPlayer, createBuffer(_)).Times(2).WillRepeatedly(Return(&testContext->m_audioBuffer));
    EXPECT_CALL(testContext->m_gstPlayer, updateAudioCaps(kSampleRate, kNumberOfChannels, kNullCodecData));
    EXPECT_CALL(testContext->m_gstPlayer, updateAudioCaps(kSampleRate, kNumberOfChannels, kCodecDataBuffer));
    EXPECT_CALL(testContext->m_gstPlayer,
                addAudioClippingToBuffer(&testContext->m_audioBuffer, kClippingStart, kClippingEnd))
        .Times(2);
    EXPECT_CALL(testContext->m_gstPlayer, attachAudioData()).Times(2);
    EXPECT_CALL(testContext->m_gstPlayer, notifyNeedMediaData(true, false));
}

void GenericTasksTestsBase::triggerAttachSamplesAudio()
{
    auto samples = buildAudioSamples();
    firebolt::rialto::server::tasks::generic::AttachSamples task{testContext->m_context, testContext->m_gstPlayer,
                                                                 samples};
    task.execute();
    EXPECT_EQ(testContext->m_context.audioBuffers.size(), 2);
}

void GenericTasksTestsBase::shouldAttachAllVideoSamples()
{
    std::shared_ptr<firebolt::rialto::CodecData> kNullCodecData{};
    EXPECT_CALL(testContext->m_gstPlayer, createBuffer(_)).Times(2).WillRepeatedly(Return(&testContext->m_videoBuffer));
    EXPECT_CALL(testContext->m_gstPlayer, updateVideoCaps(kWidth, kHeight, kFrameRate, kNullCodecData));
    EXPECT_CALL(testContext->m_gstPlayer, updateVideoCaps(kWidth, kHeight, kFrameRate, kCodecDataBuffer));
    EXPECT_CALL(testContext->m_gstPlayer, attachVideoData()).Times(2);
    EXPECT_CALL(testContext->m_gstPlayer, notifyNeedMediaData(false, true));
}

void GenericTasksTestsBase::triggerAttachSamplesVideo()
{
    auto samples = buildVideoSamples();
    firebolt::rialto::server::tasks::generic::AttachSamples task{testContext->m_context, testContext->m_gstPlayer,
                                                                 samples};
    task.execute();
    EXPECT_EQ(testContext->m_context.videoBuffers.size(), 2);
}

void GenericTasksTestsBase::shouldAttachAudioSource()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsNewEmptySimple(StrEq("audio/mpeg")))
        .WillOnce(Return(&testContext->m_gstCaps1));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstCapsSetSimpleIntStub(&testContext->m_gstCaps1, StrEq("mpegversion"), G_TYPE_INT, 4));
    expectSetCaps();
}

void GenericTasksTestsBase::triggerAttachAudioSource()
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

void GenericTasksTestsBase::checkAudioSourceAttached()
{
    EXPECT_EQ(1, testContext->m_context.streamInfo.size());
    EXPECT_NE(testContext->m_context.streamInfo.end(),
              testContext->m_context.streamInfo.find(firebolt::rialto::MediaSourceType::AUDIO));
    EXPECT_EQ(&testContext->m_appSrcAudio,
              testContext->m_context.streamInfo.at(firebolt::rialto::MediaSourceType::AUDIO).appSrc);
    EXPECT_FALSE(testContext->m_context.streamInfo.at(firebolt::rialto::MediaSourceType::AUDIO).hasDrm);
}

void GenericTasksTestsBase::shouldAttachAudioSourceWithChannelsAndRate()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsNewEmptySimple(StrEq("audio/x-eac3")))
        .WillOnce(Return(&testContext->m_gstCaps1));

    expectAddChannelAndRateAudioToCaps();
    expectSetCaps();
}

void GenericTasksTestsBase::triggerAttachAudioSourceWithChannelsAndRateAndDrm()
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

void GenericTasksTestsBase::checkAudioSourceAttachedWithDrm()
{
    EXPECT_EQ(1, testContext->m_context.streamInfo.size());
    EXPECT_NE(testContext->m_context.streamInfo.end(),
              testContext->m_context.streamInfo.find(firebolt::rialto::MediaSourceType::AUDIO));
    EXPECT_EQ(&testContext->m_appSrcAudio,
              testContext->m_context.streamInfo.at(firebolt::rialto::MediaSourceType::AUDIO).appSrc);
    EXPECT_TRUE(testContext->m_context.streamInfo.at(firebolt::rialto::MediaSourceType::AUDIO).hasDrm);
}

void GenericTasksTestsBase::shouldAttachAudioSourceWithAudioSpecificConf()
{
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstCodecUtilsOpusCreateCapsFromHeader(arrayMatcher(kCodecBufferVector), kCodecBufferVector.size()))
        .WillOnce(Return(&testContext->m_gstCaps1));
    expectSetCaps();
}

void GenericTasksTestsBase::triggerAttachOpusAudioSourceWithAudioSpecificConf()
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

void GenericTasksTestsBase::shouldAttachBwavAudioSource()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsNewEmptySimple(StrEq("audio/b-wav")))
        .WillOnce(Return(&testContext->m_gstCaps1));

    expectAddChannelAndRateAudioToCaps();
    expectAddRawAudioDataToCaps();
    expectSetCaps();
}

void GenericTasksTestsBase::triggerAttachBwavAudioSource()
{
    firebolt::rialto::AudioConfig audioConfig{kNumberOfChannels, kSampleRate, {}, kFormat, kLayout, kChannelMask};
    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSource> source =
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceAudio>("audio/b-wav", true, audioConfig);
    firebolt::rialto::server::tasks::generic::AttachSource task{testContext->m_context,
                                                                testContext->m_gstWrapper,
                                                                testContext->m_glibWrapper,
                                                                testContext->m_rdkGstreamerUtilsWrapper,
                                                                testContext->m_gstPlayer,
                                                                source};
    task.execute();
}

void GenericTasksTestsBase::shouldAttachXrawAudioSource()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsNewEmptySimple(StrEq("audio/x-raw")))
        .WillOnce(Return(&testContext->m_gstCaps1));

    expectAddChannelAndRateAudioToCaps();
    expectAddRawAudioDataToCaps();
    expectSetCaps();
}

void GenericTasksTestsBase::triggerAttachXrawAudioSource()
{
    firebolt::rialto::AudioConfig audioConfig{kNumberOfChannels, kSampleRate, {}, kFormat, kLayout, kChannelMask};
    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSource> source =
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceAudio>("audio/x-raw", true, audioConfig);
    firebolt::rialto::server::tasks::generic::AttachSource task{testContext->m_context,
                                                                testContext->m_gstWrapper,
                                                                testContext->m_glibWrapper,
                                                                testContext->m_rdkGstreamerUtilsWrapper,
                                                                testContext->m_gstPlayer,
                                                                source};
    task.execute();
}

void GenericTasksTestsBase::expectAddChannelAndRateAudioToCaps()
{
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstCapsSetSimpleIntStub(&testContext->m_gstCaps1, StrEq("channels"), G_TYPE_INT, kNumberOfChannels));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstCapsSetSimpleIntStub(&testContext->m_gstCaps1, StrEq("rate"), G_TYPE_INT, kSampleRate));
}

void GenericTasksTestsBase::expectAddRawAudioDataToCaps()
{
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstCapsSetSimpleStringStub(&testContext->m_gstCaps1, StrEq("format"), G_TYPE_STRING, StrEq("S16LE")));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsSetSimpleStringStub(&testContext->m_gstCaps1, StrEq("layout"),
                                                                       G_TYPE_STRING, StrEq("interleaved")));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsSetSimpleBitMaskStub(&testContext->m_gstCaps1, StrEq("channel-mask"),
                                                                        GST_TYPE_BITMASK, kChannelMask));
}

void GenericTasksTestsBase::expectSetCaps()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsToString(&testContext->m_gstCaps1))
        .WillOnce(Return(&testContext->m_capsStr));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(&testContext->m_capsStr));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementFactoryMake(_, StrEq(kAudName.c_str())))
        .WillOnce(Return(&testContext->m_appSrcAudio));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstAppSrcSetCaps(GST_APP_SRC(&testContext->m_appSrcAudio), &testContext->m_gstCaps1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsUnref(&testContext->m_gstCaps1));
}

void GenericTasksTestsBase::shouldAttachVideoSource(const std::string &mime, const std::string &alignment,
                                                    const std::string &format)
{
    gpointer memory = nullptr;
    expectSetGenericVideoCaps();
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstCapsSetSimpleStringStub(&testContext->m_gstCaps1, StrEq("alignment"), _, StrEq(alignment)));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstCapsSetSimpleStringStub(&testContext->m_gstCaps1, StrEq("stream-format"), _, StrEq(format)));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsNewEmptySimple(StrEq(mime))).WillOnce(Return(&testContext->m_gstCaps1));
    EXPECT_CALL(*testContext->m_glibWrapper, gMemdup(arrayMatcher(kCodecDataBuffer->data), kCodecDataBuffer->data.size()))
        .WillOnce(Return(memory));
    EXPECT_CALL(*testContext->m_gstWrapper, gstBufferNewWrapped(memory, kCodecDataBuffer->data.size()))
        .WillOnce(Return(&(testContext->m_videoBuffer)));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsSetSimpleBufferStub(&testContext->m_gstCaps1, StrEq("codec_data"), _,
                                                                       &(testContext->m_videoBuffer)));
    EXPECT_CALL(*testContext->m_gstWrapper, gstBufferUnref(&(testContext->m_videoBuffer)));
}

void GenericTasksTestsBase::triggerAttachVideoSource(const std::string &mimeType,
                                                     firebolt::rialto::SegmentAlignment segmentAligment,
                                                     firebolt::rialto::StreamFormat streamFormat)
{
    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSource> source =
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceVideo>(mimeType, false, kWidth, kHeight,
                                                                             segmentAligment, streamFormat,
                                                                             kCodecDataBuffer);
    firebolt::rialto::server::tasks::generic::AttachSource task{testContext->m_context,
                                                                testContext->m_gstWrapper,
                                                                testContext->m_glibWrapper,
                                                                testContext->m_rdkGstreamerUtilsWrapper,
                                                                testContext->m_gstPlayer,
                                                                source};
    task.execute();
}

void GenericTasksTestsBase::triggerAttachSubtitleSource()
{
    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSource> source =
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSourceSubtitle>("application/ttml+xml",
                                                                                kTextTrackIdentifier);
    firebolt::rialto::server::tasks::generic::AttachSource task{testContext->m_context,
                                                                testContext->m_gstWrapper,
                                                                testContext->m_glibWrapper,
                                                                testContext->m_rdkGstreamerUtilsWrapper,
                                                                testContext->m_gstPlayer,
                                                                source};
    task.execute();
}

void GenericTasksTestsBase::checkVideoSourceAttached()
{
    EXPECT_EQ(1, testContext->m_context.streamInfo.size());
    EXPECT_NE(testContext->m_context.streamInfo.end(),
              testContext->m_context.streamInfo.find(firebolt::rialto::MediaSourceType::VIDEO));
    EXPECT_EQ(&testContext->m_appSrcVideo,
              testContext->m_context.streamInfo.at(firebolt::rialto::MediaSourceType::VIDEO).appSrc);
    EXPECT_FALSE(testContext->m_context.streamInfo.at(firebolt::rialto::MediaSourceType::VIDEO).hasDrm);
}

void GenericTasksTestsBase::checkSubtitleSourceAttached()
{
    EXPECT_EQ(0, testContext->m_context.streamInfo.size());
}

void GenericTasksTestsBase::shouldAttachVideoSourceWithStringCodecData()
{
    expectSetGenericVideoCaps();
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstCapsSetSimpleStringStub(&testContext->m_gstCaps1, StrEq("alignment"), _, StrEq("au")));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstCapsSetSimpleStringStub(&testContext->m_gstCaps1, StrEq("stream-format"), _, StrEq("avc")));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsNewEmptySimple(StrEq("video/x-h264")))
        .WillOnce(Return(&testContext->m_gstCaps1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsSetSimpleStringStub(&testContext->m_gstCaps1, StrEq("codec_data"),
                                                                       G_TYPE_STRING, StrEq(kCodecStr.c_str())));
}

void GenericTasksTestsBase::triggerAttachVideoSourceWithStringCodecData()
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

void GenericTasksTestsBase::checkVideoSourceAttachedWithDrm()
{
    EXPECT_EQ(1, testContext->m_context.streamInfo.size());
    EXPECT_NE(testContext->m_context.streamInfo.end(),
              testContext->m_context.streamInfo.find(firebolt::rialto::MediaSourceType::VIDEO));
    EXPECT_EQ(&testContext->m_appSrcVideo,
              testContext->m_context.streamInfo.at(firebolt::rialto::MediaSourceType::VIDEO).appSrc);
    EXPECT_TRUE(testContext->m_context.streamInfo.at(firebolt::rialto::MediaSourceType::VIDEO).hasDrm);
}

void GenericTasksTestsBase::shouldAttachVideoSourceWithEmptyCodecData()
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
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementFactoryMake(_, StrEq(kVidName.c_str())))
        .WillOnce(Return(&testContext->m_appSrcVideo));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstAppSrcSetCaps(GST_APP_SRC(&testContext->m_appSrcVideo), &testContext->m_gstCaps1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsUnref(&testContext->m_gstCaps1));
}

void GenericTasksTestsBase::triggerAttachVideoSourceWithEmptyCodecData()
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

void GenericTasksTestsBase::shouldAttachVideoSourceWithDolbyVisionSource()
{
    gpointer memory = nullptr;
    expectSetGenericVideoCaps();
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstCapsSetSimpleStringStub(&testContext->m_gstCaps1, StrEq("alignment"), _, StrEq("au")));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstCapsSetSimpleStringStub(&testContext->m_gstCaps1, StrEq("stream-format"), _, StrEq("avc")));
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

void GenericTasksTestsBase::triggerAttachVideoSourceWithDolbyVisionSource()
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

void GenericTasksTestsBase::expectSetGenericVideoCaps()
{
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstCapsSetSimpleIntStub(&testContext->m_gstCaps1, StrEq("width"), G_TYPE_INT, kWidth));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstCapsSetSimpleIntStub(&testContext->m_gstCaps1, StrEq("height"), G_TYPE_INT, kHeight));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsToString(&testContext->m_gstCaps1))
        .WillOnce(Return(&testContext->m_capsStr));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(&testContext->m_capsStr));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementFactoryMake(_, StrEq(kVidName.c_str())))
        .WillOnce(Return(&testContext->m_appSrcVideo));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstAppSrcSetCaps(GST_APP_SRC(&testContext->m_appSrcVideo), &testContext->m_gstCaps1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsUnref(&testContext->m_gstCaps1));
}

void GenericTasksTestsBase::shouldSwitchAudioSource()
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
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsIsEqual(&testContext->m_gstCaps1, &testContext->m_gstCaps2))
        .WillOnce(Return(FALSE));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsToString(&testContext->m_gstCaps2))
        .WillOnce(Return(testContext->m_xEac3Str));
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
    EXPECT_CALL(testContext->m_gstPlayer, setAudioVideoFlags(true, true));
    EXPECT_CALL(testContext->m_gstPlayer, notifyNeedMediaData(true, false));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsUnref(&testContext->m_gstCaps1));
}

void GenericTasksTestsBase::shouldReattachAudioSource()
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
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsIsEqual(&testContext->m_gstCaps1, &testContext->m_gstCaps2))
        .WillOnce(Return(TRUE));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsUnref(&testContext->m_gstCaps2));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementQueryPosition(_, GST_FORMAT_TIME, _))
        .WillOnce(Invoke(
            [this](GstElement *element, GstFormat format, gint64 *cur)
            {
                *cur = kPosition;
                return TRUE;
            }));
    EXPECT_CALL(testContext->m_gstPlayer, setAudioVideoFlags(true, true));
    EXPECT_CALL(testContext->m_gstPlayer, notifyNeedMediaData(true, false));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsUnref(&testContext->m_gstCaps1));
}

void GenericTasksTestsBase::triggerReattachAudioSource()
{
    testContext->m_context.streamInfo.emplace(firebolt::rialto::MediaSourceType::AUDIO, &testContext->m_appSrcAudio);
    testContext->m_context.audioSourceRemoved = true;
    triggerAttachAudioSource();
}

void GenericTasksTestsBase::checkNewAudioSourceAttached()
{
    EXPECT_TRUE(testContext->m_context.audioNeedData);
    EXPECT_FALSE(testContext->m_context.audioSourceRemoved);
    EXPECT_EQ(testContext->m_context.lastAudioSampleTimestamps, kPosition);
}

void GenericTasksTestsBase::shouldNotSwitchAudioSourceWhenMimeTypeIsEmpty()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsNewEmpty()).WillOnce(Return(&testContext->m_gstCaps2));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsToString(&(testContext->m_gstCaps2)))
        .WillOnce(Return(&testContext->m_capsStr));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(&testContext->m_capsStr));
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsUnref(&testContext->m_gstCaps2));
}

void GenericTasksTestsBase::triggerReattachAudioSourceWithEmptyMimeType()
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

void GenericTasksTestsBase::shouldQueryPositionAndSetToZero()
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

void GenericTasksTestsBase::triggerCheckAudioUnderflowNoNotification()
{
    testContext->m_context.lastAudioSampleTimestamps = kPositionOverUnderflowMargin;
    firebolt::rialto::server::tasks::generic::CheckAudioUnderflow task{testContext->m_context, testContext->m_gstPlayer,
                                                                       &testContext->m_gstPlayerClient,
                                                                       testContext->m_gstWrapper};
    task.execute();

    EXPECT_FALSE(testContext->m_context.audioUnderflowOccured);
}

void GenericTasksTestsBase::shouldNotifyAudioUnderflow()
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

void GenericTasksTestsBase::triggerCheckAudioUnderflow()
{
    testContext->m_context.lastAudioSampleTimestamps = 0;
    firebolt::rialto::server::tasks::generic::CheckAudioUnderflow task{testContext->m_context, testContext->m_gstPlayer,
                                                                       &testContext->m_gstPlayerClient,
                                                                       testContext->m_gstWrapper};
    task.execute();

    EXPECT_TRUE(testContext->m_context.audioUnderflowOccured);
}

void GenericTasksTestsBase::shouldNotRegisterCallbackWhenPtrsAreNotEqual()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectParent(testContext->m_element)).WillOnce(Return(&testContext->m_obj1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectCast(&testContext->m_bin)).WillOnce(Return(&testContext->m_obj2));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(nullptr));
}

void GenericTasksTestsBase::constructDeepElementAdded()
{
    firebolt::rialto::server::tasks::generic::DeepElementAdded task{testContext->m_context,
                                                                    testContext->m_gstPlayer,
                                                                    testContext->m_gstWrapper,
                                                                    testContext->m_glibWrapper,
                                                                    GST_BIN(&testContext->m_pipeline),
                                                                    &testContext->m_bin,
                                                                    testContext->m_element};
}

void GenericTasksTestsBase::shouldNotRegisterCallbackWhenElementIsNull()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectParent(testContext->m_element)).WillOnce(Return(&testContext->m_obj1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectCast(&testContext->m_bin)).WillOnce(Return(&testContext->m_obj1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetName(testContext->m_element)).WillOnce(Return(nullptr));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(nullptr));
}

void GenericTasksTestsBase::shouldNotRegisterCallbackWhenElementNameIsNotTypefind()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectParent(testContext->m_element)).WillOnce(Return(&testContext->m_obj1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectCast(&testContext->m_bin)).WillRepeatedly(Return(&testContext->m_obj1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetName(testContext->m_element))
        .WillOnce(Return(testContext->m_elementName));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_elementName, StrEq("typefind")))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(testContext->m_elementName));
}

void GenericTasksTestsBase::shouldRegisterCallbackForTypefindElement()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectParent(testContext->m_element)).WillOnce(Return(&testContext->m_obj1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectCast(&testContext->m_bin)).WillRepeatedly(Return(&testContext->m_obj1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetName(testContext->m_element))
        .WillOnce(Return(testContext->m_typefindElementName));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_typefindElementName, StrEq("typefind")))
        .WillOnce(Return(testContext->m_typefindElementName));
    EXPECT_CALL(*testContext->m_glibWrapper,
                gSignalConnect(G_OBJECT(testContext->m_element), StrEq("have-type"), _, &testContext->m_gstPlayer))
        .WillOnce(Return(kSignalId));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(testContext->m_typefindElementName));
}

void GenericTasksTestsBase::shouldUpdatePlaybackGroupWhenCallbackIsCalled()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectParent(testContext->m_element)).WillOnce(Return(&testContext->m_obj1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectCast(&testContext->m_bin)).WillOnce(Return(&testContext->m_obj1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetName(testContext->m_element))
        .WillOnce(Return(testContext->m_typefindElementName));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_typefindElementName, StrEq("typefind")))
        .WillOnce(Return(testContext->m_typefindElementName));
    EXPECT_CALL(*testContext->m_glibWrapper,
                gSignalConnect(G_OBJECT(testContext->m_element), StrEq("have-type"), _, &testContext->m_gstPlayer))
        .WillOnce(Invoke(
            [&](gpointer instance, const gchar *detailed_signal, GCallback c_handler, gpointer data)
            {
                ((void (*)(GstElement *, guint, const GstCaps *, gpointer))c_handler)(testContext->m_element, 0,
                                                                                      &testContext->m_gstCaps1,
                                                                                      &testContext->m_gstPlayer);
                return kSignalId;
            }));
    EXPECT_CALL(testContext->m_gstPlayer, updatePlaybackGroup(testContext->m_element, &testContext->m_gstCaps1));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(testContext->m_typefindElementName));
}

void GenericTasksTestsBase::shouldSetTypefindElement()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectCast(nullptr)).WillOnce(Return(nullptr));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_typefindElementName, StrEq("audiosink")))
        .WillOnce(Return(nullptr));
}

void GenericTasksTestsBase::triggerDeepElementAdded()
{
    firebolt::rialto::server::tasks::generic::DeepElementAdded task{testContext->m_context,
                                                                    testContext->m_gstPlayer,
                                                                    testContext->m_gstWrapper,
                                                                    testContext->m_glibWrapper,
                                                                    GST_BIN(&testContext->m_pipeline),
                                                                    &testContext->m_bin,
                                                                    testContext->m_element};
    task.execute();
}

void GenericTasksTestsBase::checkTypefindPlaybackGroupAdded()
{
    EXPECT_EQ(testContext->m_context.playbackGroup.m_gstPipeline, GST_ELEMENT(&testContext->m_pipeline));
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioTypefind, testContext->m_element);
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioParse, nullptr);
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioDecoder, nullptr);
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioPlaysinkBin, nullptr);
}

void GenericTasksTestsBase::checkPipelinePlaybackGroupAdded()
{
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioTypefind, nullptr);
    EXPECT_EQ(testContext->m_context.playbackGroup.m_gstPipeline, GST_ELEMENT(&testContext->m_pipeline));
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioParse, nullptr);
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioDecoder, nullptr);
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioPlaysinkBin, nullptr);
}

void GenericTasksTestsBase::shouldSetParseElement()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectParent(testContext->m_element)).WillOnce(Return(&testContext->m_obj1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectCast(&testContext->m_bin)).WillRepeatedly(Return(&testContext->m_obj1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetName(testContext->m_element))
        .WillOnce(Return(testContext->m_parseElementName));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_parseElementName, StrEq("typefind")))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(testContext->m_parseElementName));

    testContext->m_context.playbackGroup.m_curAudioDecodeBin = &testContext->m_audioDecodeBin;
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectCast(&testContext->m_audioDecodeBin))
        .WillOnce(Return(&testContext->m_obj1));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_parseElementName, StrEq("parse")))
        .WillOnce(Return(testContext->m_parseElementName));
}

void GenericTasksTestsBase::checkParsePlaybackGroupAdded()
{
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioTypefind, nullptr);
    EXPECT_EQ(testContext->m_context.playbackGroup.m_gstPipeline, GST_ELEMENT(&testContext->m_pipeline));
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioParse, testContext->m_element);
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioDecoder, nullptr);
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioPlaysinkBin, nullptr);
}

void GenericTasksTestsBase::shouldSetDecoderElement()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectParent(testContext->m_element)).WillOnce(Return(&testContext->m_obj1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectCast(&testContext->m_bin)).WillRepeatedly(Return(&testContext->m_obj1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetName(testContext->m_element))
        .WillOnce(Return(testContext->m_decoderElementName));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_decoderElementName, StrEq("typefind")))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(testContext->m_decoderElementName));

    testContext->m_context.playbackGroup.m_curAudioDecodeBin = &testContext->m_audioDecodeBin;
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectCast(&testContext->m_audioDecodeBin))
        .WillOnce(Return(&testContext->m_obj1));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_decoderElementName, StrEq("parse")))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_decoderElementName, StrEq("dec")))
        .WillOnce(Return(testContext->m_decoderElementName));
}

void GenericTasksTestsBase::checkDecoderPlaybackGroupAdded()
{
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioTypefind, nullptr);
    EXPECT_EQ(testContext->m_context.playbackGroup.m_gstPipeline, GST_ELEMENT(&testContext->m_pipeline));
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioParse, nullptr);
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioDecoder, testContext->m_element);
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioPlaysinkBin, nullptr);
}

void GenericTasksTestsBase::shouldSetGenericElement()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectCast(nullptr)).WillOnce(Return(nullptr));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_elementName, StrEq("audiosink")))
        .WillOnce(Return(nullptr));
}

void GenericTasksTestsBase::shouldSetAudioSinkElement()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectParent(testContext->m_element)).WillOnce(Return(&testContext->m_obj1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectCast(&testContext->m_bin)).WillRepeatedly(Return(&testContext->m_obj1));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetName(testContext->m_element))
        .WillOnce(Return(testContext->m_audioSinkElementName));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_audioSinkElementName, StrEq("typefind")))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(testContext->m_audioSinkElementName));

    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectCast(nullptr)).WillOnce(Return(nullptr));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_audioSinkElementName, StrEq("audiosink")))
        .WillOnce(Return(testContext->m_audioSinkElementName));
}

void GenericTasksTestsBase::shouldHaveNullParentSink()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetParent(testContext->m_element))
        .WillOnce(Return(GST_OBJECT(&testContext->m_audioParentSink)));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetName(&testContext->m_audioParentSink)).WillOnce(Return(nullptr));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(nullptr));
}

void GenericTasksTestsBase::shouldHaveNonBinParentSink()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetParent(testContext->m_element))
        .WillOnce(Return(GST_OBJECT(&testContext->m_audioParentSink)));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetName(&testContext->m_audioParentSink))
        .WillOnce(Return(testContext->m_elementName));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_elementName, StrEq("bin"))).WillOnce(Return(nullptr));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(testContext->m_elementName));
}

void GenericTasksTestsBase::shouldHaveBinParentSink()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetParent(testContext->m_element))
        .WillOnce(Return(GST_OBJECT(&testContext->m_audioParentSink)));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetName(&testContext->m_audioParentSink))
        .WillOnce(Return(testContext->m_binElementName));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_binElementName, StrEq("bin")))
        .WillOnce(Return(testContext->m_binElementName));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(testContext->m_binElementName));
}

void GenericTasksTestsBase::checkAudioSinkPlaybackGroupAdded()
{
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioTypefind, nullptr);
    EXPECT_EQ(testContext->m_context.playbackGroup.m_gstPipeline, GST_ELEMENT(&testContext->m_pipeline));
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioParse, nullptr);
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioDecoder, nullptr);
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioPlaysinkBin, &testContext->m_audioParentSink);
}

void GenericTasksTestsBase::triggerUpdatePlaybackGroupNoCaps()
{
    firebolt::rialto::server::tasks::generic::UpdatePlaybackGroup task{testContext->m_context, testContext->m_gstWrapper,
                                                                       testContext->m_glibWrapper,
                                                                       testContext->m_element, nullptr};
    task.execute();
}

void GenericTasksTestsBase::checkNoPlaybackGroupAdded()
{
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioDecodeBin, nullptr);
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioTypefind, nullptr);
}

void GenericTasksTestsBase::shouldReturnNullCaps()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsToString(&testContext->m_gstCaps1)).WillOnce(Return(nullptr));
}

void GenericTasksTestsBase::triggerUpdatePlaybackGroup()
{
    firebolt::rialto::server::tasks::generic::UpdatePlaybackGroup task{testContext->m_context, testContext->m_gstWrapper,
                                                                       testContext->m_glibWrapper,
                                                                       testContext->m_element, &testContext->m_gstCaps1};
    task.execute();
}

void GenericTasksTestsBase::shouldDoNothingForVideoCaps()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsToString(&testContext->m_gstCaps1))
        .WillOnce(Return(testContext->m_videoStr));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_videoStr, StrEq("audio/"))).WillOnce(Return(nullptr));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(testContext->m_videoStr));
}

void GenericTasksTestsBase::shouldDoNothingWhenTypefindParentIsNull()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsToString(&testContext->m_gstCaps1))
        .WillOnce(Return(testContext->m_audioStr));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_audioStr, StrEq("audio/")))
        .WillOnce(Return(testContext->m_audioStr));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetParent(testContext->m_element)).WillOnce(Return(nullptr));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(testContext->m_audioStr));
}

void GenericTasksTestsBase::shouldDoNothingWhenElementOtherThanDecodebin()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsToString(&testContext->m_gstCaps1))
        .WillOnce(Return(testContext->m_audioStr));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_audioStr, StrEq("audio/")))
        .WillOnce(Return(testContext->m_audioStr));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetParent(testContext->m_element))
        .WillOnce(Return(GST_OBJECT(&testContext->m_audioParentSink)));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetName(&testContext->m_audioParentSink))
        .WillOnce(Return(testContext->m_elementName));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_elementName, StrEq("decodebin")))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(testContext->m_elementName));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(testContext->m_audioStr));
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectUnref(&testContext->m_audioParentSink));
}

void GenericTasksTestsBase::shouldSuccessfullyFindTypefindAndParent()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstCapsToString(&testContext->m_gstCaps1))
        .WillOnce(Return(testContext->m_audioStr));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_audioStr, StrEq("audio/")))
        .WillOnce(Return(testContext->m_audioStr));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetParent(testContext->m_element))
        .WillOnce(Return(GST_OBJECT(&testContext->m_audioParentSink)));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetName(&testContext->m_audioParentSink))
        .WillOnce(Return(testContext->m_decoderElementName));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrrstr(testContext->m_decoderElementName, StrEq("decodebin")))
        .WillOnce(Return(testContext->m_decoderElementName));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementGetName(testContext->m_element))
        .WillOnce(Return(testContext->m_typefindElementName));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(testContext->m_decoderElementName));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(testContext->m_typefindElementName));
    EXPECT_CALL(*testContext->m_glibWrapper, gFree(testContext->m_audioStr));
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectUnref(&testContext->m_audioParentSink));
}

void GenericTasksTestsBase::checkPlaybackGroupAdded()
{
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioDecodeBin, &testContext->m_audioParentSink);
    EXPECT_EQ(testContext->m_context.playbackGroup.m_curAudioTypefind, testContext->m_element);
}

void GenericTasksTestsBase::shouldStopGstPlayer()
{
    testContext->m_context.videoNeedData = true;
    testContext->m_context.audioNeedData = true;
    EXPECT_CALL(testContext->m_gstPlayer, stopPositionReportingAndCheckAudioUnderflowTimer());
    EXPECT_CALL(testContext->m_gstPlayer, changePipelineState(GST_STATE_NULL));
}

void GenericTasksTestsBase::triggerStop()
{
    firebolt::rialto::server::tasks::generic::Stop task{testContext->m_context, testContext->m_gstPlayer};
    task.execute();
}

void GenericTasksTestsBase::checkNoMoreNeedData()
{
    EXPECT_FALSE(testContext->m_context.videoNeedData);
    EXPECT_FALSE(testContext->m_context.audioNeedData);
}

void GenericTasksTestsBase::triggerEnoughDataAudio()
{
    firebolt::rialto::server::tasks::generic::EnoughData task{testContext->m_context,
                                                              GST_APP_SRC(&testContext->m_appSrcAudio)};
    task.execute();
}

void GenericTasksTestsBase::triggerEnoughDataVideo()
{
    firebolt::rialto::server::tasks::generic::EnoughData task{testContext->m_context,
                                                              GST_APP_SRC(&testContext->m_appSrcVideo)};
    task.execute();
}

void GenericTasksTestsBase::checkNeedDataForBothSources()
{
    EXPECT_TRUE(testContext->m_context.audioNeedData);
    EXPECT_TRUE(testContext->m_context.videoNeedData);
}

void GenericTasksTestsBase::checkNeedDataForAudioOnly()
{
    EXPECT_TRUE(testContext->m_context.audioNeedData);
    EXPECT_FALSE(testContext->m_context.videoNeedData);
}

void GenericTasksTestsBase::checkNeedDataForVideoOnly()
{
    EXPECT_FALSE(testContext->m_context.audioNeedData);
    EXPECT_TRUE(testContext->m_context.videoNeedData);
}

void GenericTasksTestsBase::triggerEosAudio()
{
    firebolt::rialto::server::tasks::generic::Eos task{testContext->m_context, testContext->m_gstPlayer,
                                                       testContext->m_gstWrapper,
                                                       firebolt::rialto::MediaSourceType::AUDIO};
    task.execute();
}

void GenericTasksTestsBase::triggerEosVideo()
{
    firebolt::rialto::server::tasks::generic::Eos task{testContext->m_context, testContext->m_gstPlayer,
                                                       testContext->m_gstWrapper,
                                                       firebolt::rialto::MediaSourceType::VIDEO};
    task.execute();
}

void GenericTasksTestsBase::shouldGstAppSrcEndOfStreamSuccess()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstAppSrcEndOfStream(_)).WillOnce(Return(GST_FLOW_OK));
}

void GenericTasksTestsBase::shouldGstAppSrcEndOfStreamFailure()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstAppSrcEndOfStream(_)).WillOnce(Return(GST_FLOW_ERROR));
}

void GenericTasksTestsBase::shouldCancelUnderflow()
{
    bool underflow = true;
    EXPECT_CALL(testContext->m_gstPlayer, cancelUnderflow(underflow));
}

void GenericTasksTestsBase::setUnderflowFlag(bool isUnderflowFlag)
{
    testContext->m_underflowFlag = isUnderflowFlag;
}

void GenericTasksTestsBase::setUnderflowEnabled(bool isUnderflowEnabled)
{
    testContext->m_underflowEnabled = isUnderflowEnabled;
}

void GenericTasksTestsBase::triggerVideoUnderflow()
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

void GenericTasksTestsBase::checkUnderflowFlag(bool expected)
{
    EXPECT_EQ(testContext->m_underflowFlag, expected);
}

void GenericTasksTestsBase::shouldNotifyVideoUnderflow()
{
    EXPECT_CALL(testContext->m_gstPlayerClient, notifyBufferUnderflow(firebolt::rialto::MediaSourceType::VIDEO));
}

void GenericTasksTestsBase::shouldStopWorkerThread()
{
    EXPECT_CALL(testContext->m_gstPlayer, stopWorkerThread());
}

void GenericTasksTestsBase::triggerShutdown()
{
    firebolt::rialto::server::tasks::generic::Shutdown task{testContext->m_gstPlayer};
    task.execute();
}

void GenericTasksTestsBase::triggerSetMute()
{
    firebolt::rialto::server::tasks::generic::SetMute task{testContext->m_context, testContext->m_gstWrapper, kMute};
    task.execute();
}

void GenericTasksTestsBase::shouldGstSetMute()
{
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstStreamVolumeSetMute(GST_STREAM_VOLUME(testContext->m_context.pipeline), kMute));
}

void GenericTasksTestsBase::triggerSetPositionNullClient()
{
    firebolt::rialto::server::tasks::generic::SetPosition task{testContext->m_context, testContext->m_gstPlayer,
                                                               nullptr, testContext->m_gstWrapper, kPosition};
    task.execute();
}

void GenericTasksTestsBase::triggerSetPosition()
{
    firebolt::rialto::server::tasks::generic::SetPosition task{testContext->m_context, testContext->m_gstPlayer,
                                                               &testContext->m_gstPlayerClient,
                                                               testContext->m_gstWrapper, kPosition};
    task.execute();
}

void GenericTasksTestsBase::checkNeedDataPendingForBothSources()
{
    EXPECT_TRUE(testContext->m_context.audioNeedDataPending);
    EXPECT_TRUE(testContext->m_context.videoNeedDataPending);
}

void GenericTasksTestsBase::checkBuffersDoExist()
{
    EXPECT_FALSE(testContext->m_context.audioBuffers.empty());
    EXPECT_FALSE(testContext->m_context.videoBuffers.empty());
}

void GenericTasksTestsBase::checkDoNotNeedDataForBothSources()
{
    EXPECT_FALSE(testContext->m_context.audioNeedData);
    EXPECT_FALSE(testContext->m_context.videoNeedData);
}

void GenericTasksTestsBase::checkNoNeedDataPendingForBothSources()
{
    EXPECT_FALSE(testContext->m_context.audioNeedDataPending);
    EXPECT_FALSE(testContext->m_context.videoNeedDataPending);
}

void GenericTasksTestsBase::checkBuffersEmpty()
{
    EXPECT_TRUE(testContext->m_context.audioBuffers.empty());
    EXPECT_TRUE(testContext->m_context.videoBuffers.empty());
}

void GenericTasksTestsBase::shouldExtractBuffers()
{
    EXPECT_CALL(testContext->m_gstPlayerClient, notifyPlaybackState(firebolt::rialto::PlaybackState::SEEKING));
    EXPECT_CALL(testContext->m_gstPlayerClient, clearActiveRequestsCache());
    EXPECT_CALL(*testContext->m_gstWrapper, gstBufferUnref(&testContext->m_audioBuffer));
    EXPECT_CALL(*testContext->m_gstWrapper, gstBufferUnref(&testContext->m_videoBuffer));
}

void GenericTasksTestsBase::shouldNotifyFailure()
{
    EXPECT_CALL(testContext->m_gstPlayerClient, notifyPlaybackState(firebolt::rialto::PlaybackState::FAILURE));
}

void GenericTasksTestsBase::shouldSeekFailure()
{
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstElementSeek(&testContext->m_pipeline, 1.0, GST_FORMAT_TIME,
                               static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH), GST_SEEK_TYPE_SET, kPosition,
                               GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE))
        .WillOnce(Return(false));
    shouldNotifyFailure();
}

void GenericTasksTestsBase::shouldSeekSuccess()
{
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstElementSeek(&testContext->m_pipeline, testContext->m_context.playbackRate, GST_FORMAT_TIME,
                               static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH), GST_SEEK_TYPE_SET, kPosition,
                               GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE))
        .WillOnce(Return(true));
    EXPECT_CALL(testContext->m_gstPlayerClient, notifyPlaybackState(firebolt::rialto::PlaybackState::SEEK_DONE));
    EXPECT_CALL(testContext->m_gstPlayerClient, notifyNeedMediaData(firebolt::rialto::MediaSourceType::AUDIO))
        .WillOnce(Return(true));
    EXPECT_CALL(testContext->m_gstPlayerClient, notifyNeedMediaData(firebolt::rialto::MediaSourceType::VIDEO))
        .WillOnce(Return(true));
}

void GenericTasksTestsBase::checkNoEos()
{
    EXPECT_TRUE(testContext->m_context.endOfStreamInfo.empty());
    EXPECT_FALSE(testContext->m_context.eosNotified);
}

void GenericTasksTestsBase::shouldChangeStatePlayingSuccess()
{
    EXPECT_CALL(testContext->m_gstPlayer, changePipelineState(GST_STATE_PLAYING)).WillOnce(Return(true));
}

void GenericTasksTestsBase::shouldChangeStatePlayingFailure()
{
    EXPECT_CALL(testContext->m_gstPlayer, changePipelineState(GST_STATE_PLAYING)).WillOnce(Return(false));
}

void GenericTasksTestsBase::triggerPlay()
{
    firebolt::rialto::server::tasks::generic::Play task{testContext->m_gstPlayer};
    task.execute();
}

void GenericTasksTestsBase::triggerPing()
{
    firebolt::rialto::server::tasks::generic::Ping task{std::make_unique<StrictMock<HeartbeatHandlerMock>>()};
    task.execute();
}

void GenericTasksTestsBase::shouldPause()
{
    EXPECT_CALL(testContext->m_gstPlayer, stopPositionReportingAndCheckAudioUnderflowTimer());
    EXPECT_CALL(testContext->m_gstPlayer, changePipelineState(GST_STATE_PAUSED));
}

void GenericTasksTestsBase::triggerPause()
{
    firebolt::rialto::server::tasks::generic::Pause task{testContext->m_context, testContext->m_gstPlayer};
    task.execute();
}

void GenericTasksTestsBase::checkContextPaused()
{
    EXPECT_FALSE(testContext->m_context.isPlaying);
}

void GenericTasksTestsBase::shouldReportPosition()
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

void GenericTasksTestsBase::triggerReportPosition()
{
    firebolt::rialto::server::tasks::generic::ReportPosition task{testContext->m_context, &testContext->m_gstPlayerClient,
                                                                  testContext->m_gstWrapper};
    task.execute();
}

void GenericTasksTestsBase::shouldFailToReportPosition()
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

void GenericTasksTestsBase::shouldFinishSetupSource()
{
    EXPECT_CALL(*testContext->m_gstSrc,
                setupAndAddAppArc(std::dynamic_pointer_cast<firebolt::rialto::server::IDecryptionService>(
                                      testContext->m_decryptionServiceMock)
                                      .get(),
                                  testContext->m_element, testContext->m_streamInfoAudio, _, &testContext->m_gstPlayer,
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
                                  testContext->m_element, testContext->m_streamInfoVideo, _, &testContext->m_gstPlayer,
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
    EXPECT_CALL(*testContext->m_gstSrc, allAppSrcsAdded(testContext->m_element));
    EXPECT_CALL(testContext->m_gstPlayerClient, notifyPlaybackState(firebolt::rialto::PlaybackState::IDLE));
}

void GenericTasksTestsBase::triggerFinishSetupSource()
{
    firebolt::rialto::server::tasks::generic::FinishSetupSource task{testContext->m_context, testContext->m_gstPlayer,
                                                                     &testContext->m_gstPlayerClient};
    task.execute();
}

void GenericTasksTestsBase::shouldScheduleNeedMediaDataAudio()
{
    EXPECT_CALL(testContext->m_gstPlayer, scheduleNeedMediaData(GST_APP_SRC(&testContext->m_appSrcAudio)));
}

void GenericTasksTestsBase::triggerAudioCallbackNeedData()
{
    ASSERT_TRUE(testContext->m_audioCallbacks.need_data);
    ASSERT_TRUE(testContext->m_audioUserData);
    ((void (*)(GstAppSrc *, guint,
               gpointer))testContext->m_audioCallbacks.need_data)(GST_APP_SRC(&testContext->m_appSrcAudio), kDataLength,
                                                                  testContext->m_audioUserData);
}

void GenericTasksTestsBase::shouldScheduleNeedMediaDataVideo()
{
    EXPECT_CALL(testContext->m_gstPlayer, scheduleNeedMediaData(GST_APP_SRC(&testContext->m_appSrcVideo)));
}

void GenericTasksTestsBase::triggerVideoCallbackNeedData()
{
    ASSERT_TRUE(testContext->m_videoCallbacks.need_data);
    ASSERT_TRUE(testContext->m_videoUserData);
    ((void (*)(GstAppSrc *, guint,
               gpointer))testContext->m_videoCallbacks.need_data)(GST_APP_SRC(&testContext->m_appSrcVideo), kDataLength,
                                                                  testContext->m_videoUserData);
}

void GenericTasksTestsBase::shouldScheduleEnoughDataAudio()
{
    EXPECT_CALL(testContext->m_gstPlayer, scheduleEnoughData(GST_APP_SRC(&testContext->m_appSrcAudio)));
}

void GenericTasksTestsBase::triggerAudioCallbackEnoughData()
{
    ASSERT_TRUE(testContext->m_audioCallbacks.enough_data);
    ASSERT_TRUE(testContext->m_audioUserData);
    ((void (*)(GstAppSrc *, gpointer))testContext->m_audioCallbacks.enough_data)(GST_APP_SRC(&testContext->m_appSrcAudio),
                                                                                 testContext->m_audioUserData);
}

void GenericTasksTestsBase::shouldScheduleEnoughDataVideo()
{
    EXPECT_CALL(testContext->m_gstPlayer, scheduleEnoughData(GST_APP_SRC(&testContext->m_appSrcVideo)));
}

void GenericTasksTestsBase::triggerVideoCallbackEnoughData()
{
    ASSERT_TRUE(testContext->m_videoCallbacks.enough_data);
    ASSERT_TRUE(testContext->m_videoUserData);
    ((void (*)(GstAppSrc *, gpointer))testContext->m_videoCallbacks.enough_data)(GST_APP_SRC(&testContext->m_appSrcVideo),
                                                                                 testContext->m_videoUserData);
}

void GenericTasksTestsBase::triggerAudioCallbackSeekData()
{
    EXPECT_TRUE(testContext->m_audioCallbacks.seek_data);
    EXPECT_TRUE(testContext->m_audioUserData);
    ((gboolean(*)(GstAppSrc *, guint64,
                  gpointer))testContext->m_audioCallbacks.seek_data)(GST_APP_SRC(&testContext->m_appSrcAudio), kOffset,
                                                                     testContext->m_audioUserData);
}

void GenericTasksTestsBase::triggerVideoCallbackSeekData()
{
    EXPECT_TRUE(testContext->m_videoCallbacks.seek_data);
    EXPECT_TRUE(testContext->m_videoUserData);
    ((gboolean(*)(GstAppSrc *, guint64,
                  gpointer))testContext->m_videoCallbacks.seek_data)(GST_APP_SRC(&testContext->m_appSrcVideo), kOffset,
                                                                     testContext->m_videoUserData);
}

void GenericTasksTestsBase::checkSourcesAttached()
{
    EXPECT_TRUE(testContext->m_context.wereAllSourcesAttached);
}

void GenericTasksTestsBase::checkSetupSourceFinished()
{
    EXPECT_TRUE(testContext->m_context.setupSourceFinished);
}

void GenericTasksTestsBase::checkSetupSourceUnfinished()
{
    EXPECT_FALSE(testContext->m_context.setupSourceFinished);
}

void GenericTasksTestsBase::triggerNeedDataAudio()
{
    firebolt::rialto::server::tasks::generic::NeedData task{testContext->m_context, &testContext->m_gstPlayerClient,
                                                            GST_APP_SRC(&testContext->m_appSrcAudio)};
    task.execute();
}

void GenericTasksTestsBase::triggerNeedDataVideo()
{
    firebolt::rialto::server::tasks::generic::NeedData task{testContext->m_context, &testContext->m_gstPlayerClient,
                                                            GST_APP_SRC(&testContext->m_appSrcVideo)};
    task.execute();
}

void GenericTasksTestsBase::triggerNeedDataUnknownSrc()
{
    GstAppSrc unknownSrc{};
    firebolt::rialto::server::tasks::generic::NeedData task{testContext->m_context, &testContext->m_gstPlayerClient,
                                                            &unknownSrc};
    task.execute();
}

void GenericTasksTestsBase::shouldNotifyNeedAudioDataSuccess()
{
    EXPECT_CALL(testContext->m_gstPlayerClient, notifyNeedMediaData(firebolt::rialto::MediaSourceType::AUDIO))
        .WillOnce(Return(true));
}

void GenericTasksTestsBase::shouldNotifyNeedVideoDataSuccess()
{
    EXPECT_CALL(testContext->m_gstPlayerClient, notifyNeedMediaData(firebolt::rialto::MediaSourceType::VIDEO))
        .WillOnce(Return(true));
}

void GenericTasksTestsBase::checkNeedDataPendingForAudioOnly()
{
    EXPECT_TRUE(testContext->m_context.audioNeedDataPending);
    EXPECT_FALSE(testContext->m_context.videoNeedDataPending);
}

void GenericTasksTestsBase::checkNeedDataPendingForVideoOnly()
{
    EXPECT_FALSE(testContext->m_context.audioNeedDataPending);
    EXPECT_TRUE(testContext->m_context.videoNeedDataPending);
}

void GenericTasksTestsBase::shouldNotifyNeedAudioDataFailure()
{
    EXPECT_CALL(testContext->m_gstPlayerClient, notifyNeedMediaData(firebolt::rialto::MediaSourceType::AUDIO))
        .WillOnce(Return(false));
}

void GenericTasksTestsBase::shouldNotifyNeedVideoDataFailure()
{
    EXPECT_CALL(testContext->m_gstPlayerClient, notifyNeedMediaData(firebolt::rialto::MediaSourceType::VIDEO))
        .WillOnce(Return(false));
}

void GenericTasksTestsBase::triggerSetPlaybackRate()
{
    firebolt::rialto::server::tasks::generic::SetPlaybackRate task{testContext->m_context, testContext->m_gstWrapper,
                                                                   testContext->m_glibWrapper, kRate};
    task.execute();
}

void GenericTasksTestsBase::checkNoPendingPlaybackRate()
{
    EXPECT_EQ(testContext->m_context.pendingPlaybackRate, firebolt::rialto::server::kNoPendingPlaybackRate);
}

void GenericTasksTestsBase::checkPendingPlaybackRate()
{
    EXPECT_EQ(testContext->m_context.pendingPlaybackRate, kRate);
}

void GenericTasksTestsBase::checkPlaybackRateSet()
{
    EXPECT_EQ(testContext->m_context.playbackRate, kRate);
}

void GenericTasksTestsBase::checkPlaybackRateDefault()
{
    EXPECT_EQ(testContext->m_context.playbackRate, 1.0);
}

void GenericTasksTestsBase::setPipelinePlaying()
{
    GST_STATE(&testContext->m_pipeline) = GST_STATE_PLAYING;
}

void GenericTasksTestsBase::shouldSetPlaybackRateAudioSinkNullSuccess()
{
    EXPECT_CALL(*testContext->m_glibWrapper, gObjectGetStub(_, StrEq("audio-sink"), _));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstStructureNewDoubleStub(StrEq("custom-instant-rate-change"), StrEq("rate"), G_TYPE_DOUBLE, kRate))
        .WillOnce(Return(&testContext->m_structure));
    EXPECT_CALL(*testContext->m_gstWrapper, gstEventNewCustom(GST_EVENT_CUSTOM_DOWNSTREAM_OOB, &testContext->m_structure))
        .WillOnce(Return(&testContext->m_event));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementSendEvent(_, &testContext->m_event)).WillOnce(Return(TRUE));
}

void GenericTasksTestsBase::shouldSetPlaybackRateAudioSinkNullFailure()
{
    EXPECT_CALL(*testContext->m_glibWrapper, gObjectGetStub(_, StrEq("audio-sink"), _));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstStructureNewDoubleStub(StrEq("custom-instant-rate-change"), StrEq("rate"), G_TYPE_DOUBLE, kRate))
        .WillOnce(Return(&testContext->m_structure));
    EXPECT_CALL(*testContext->m_gstWrapper, gstEventNewCustom(GST_EVENT_CUSTOM_DOWNSTREAM_OOB, &testContext->m_structure))
        .WillOnce(Return(&testContext->m_event));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementSendEvent(_, &testContext->m_event)).WillOnce(Return(FALSE));
}

void GenericTasksTestsBase::shouldSetPlaybackRateAudioSinkOtherThanAmlhala()
{
    EXPECT_CALL(*testContext->m_glibWrapper, gObjectGetStub(_, StrEq("audio-sink"), _))
        .WillOnce(Invoke(
            [&](gpointer object, const gchar *first_property_name, void *element)
            {
                GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                *elementPtr = testContext->m_element;
            }));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrHasPrefix(_, StrEq("amlhalasink"))).WillOnce(Return(FALSE));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstStructureNewDoubleStub(StrEq("custom-instant-rate-change"), StrEq("rate"), G_TYPE_DOUBLE, kRate))
        .WillOnce(Return(&testContext->m_structure));
    EXPECT_CALL(*testContext->m_gstWrapper, gstEventNewCustom(GST_EVENT_CUSTOM_DOWNSTREAM_OOB, &testContext->m_structure))
        .WillOnce(Return(&testContext->m_event));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementSendEvent(_, &testContext->m_event)).WillOnce(Return(TRUE));
    EXPECT_CALL(*testContext->m_glibWrapper, gObjectUnref(testContext->m_element));
}

void GenericTasksTestsBase::shouldFailToSetPlaybackRateAudioSinkOtherThanAmlhala()
{
    EXPECT_CALL(*testContext->m_glibWrapper, gObjectGetStub(_, StrEq("audio-sink"), _))
        .WillOnce(Invoke(
            [&](gpointer object, const gchar *first_property_name, void *element)
            {
                GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                *elementPtr = testContext->m_element;
            }));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrHasPrefix(_, StrEq("amlhalasink"))).WillOnce(Return(FALSE));
    EXPECT_CALL(*testContext->m_gstWrapper,
                gstStructureNewDoubleStub(StrEq("custom-instant-rate-change"), StrEq("rate"), G_TYPE_DOUBLE, kRate))
        .WillOnce(Return(&testContext->m_structure));
    EXPECT_CALL(*testContext->m_gstWrapper, gstEventNewCustom(GST_EVENT_CUSTOM_DOWNSTREAM_OOB, &testContext->m_structure))
        .WillOnce(Return(&testContext->m_event));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementSendEvent(_, &testContext->m_event)).WillOnce(Return(FALSE));
    EXPECT_CALL(*testContext->m_glibWrapper, gObjectUnref(testContext->m_element));
}

void GenericTasksTestsBase::shouldSetPlaybackRateAmlhalaAudioSink()
{
    EXPECT_CALL(*testContext->m_glibWrapper, gObjectGetStub(_, StrEq("audio-sink"), _))
        .WillOnce(Invoke(
            [&](gpointer object, const gchar *first_property_name, void *element)
            {
                GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                *elementPtr = testContext->m_element;
            }));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrHasPrefix(_, StrEq("amlhalasink"))).WillOnce(Return(TRUE));
    EXPECT_CALL(*testContext->m_gstWrapper, gstSegmentNew()).WillOnce(Return(&testContext->m_segment));
    EXPECT_CALL(*testContext->m_gstWrapper, gstSegmentInit(&testContext->m_segment, GST_FORMAT_TIME));
    EXPECT_CALL(*testContext->m_gstWrapper, gstEventNewSegment(&testContext->m_segment))
        .WillOnce(Return(&testContext->m_event));
    EXPECT_CALL(*testContext->m_gstWrapper, gstPadSendEvent(_, &testContext->m_event)).WillOnce(Return(TRUE));
    EXPECT_CALL(*testContext->m_gstWrapper, gstSegmentFree(&testContext->m_segment));
    EXPECT_CALL(*testContext->m_glibWrapper, gObjectUnref(testContext->m_element));
}

void GenericTasksTestsBase::shouldFailToSetPlaybackRateAmlhalaAudioSink()
{
    EXPECT_CALL(*testContext->m_glibWrapper, gObjectGetStub(_, StrEq("audio-sink"), _))
        .WillOnce(Invoke(
            [&](gpointer object, const gchar *first_property_name, void *element)
            {
                GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                *elementPtr = testContext->m_element;
            }));
    EXPECT_CALL(*testContext->m_glibWrapper, gStrHasPrefix(_, StrEq("amlhalasink"))).WillOnce(Return(TRUE));
    EXPECT_CALL(*testContext->m_gstWrapper, gstSegmentNew()).WillOnce(Return(&testContext->m_segment));
    EXPECT_CALL(*testContext->m_gstWrapper, gstSegmentInit(&testContext->m_segment, GST_FORMAT_TIME));
    EXPECT_CALL(*testContext->m_gstWrapper, gstEventNewSegment(&testContext->m_segment))
        .WillOnce(Return(&testContext->m_event));
    EXPECT_CALL(*testContext->m_gstWrapper, gstPadSendEvent(_, &testContext->m_event)).WillOnce(Return(FALSE));
    EXPECT_CALL(*testContext->m_gstWrapper, gstSegmentFree(&testContext->m_segment));
    EXPECT_CALL(*testContext->m_glibWrapper, gObjectUnref(testContext->m_element));
}

void GenericTasksTestsBase::checkSegmentInfo()
{
    EXPECT_EQ(testContext->m_segment.rate, kRate);
    EXPECT_EQ(testContext->m_segment.start, GST_CLOCK_TIME_NONE);
    EXPECT_EQ(testContext->m_segment.position, GST_CLOCK_TIME_NONE);
}

void GenericTasksTestsBase::shouldRenderFrame()
{
    EXPECT_CALL(*testContext->m_glibWrapper, gObjectGetStub(&testContext->m_pipeline, StrEq("video-sink"), _))
        .WillOnce(Invoke(
            [&](gpointer object, const gchar *first_property_name, void *element)
            {
                GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                *elementPtr = testContext->m_element;
            }));

    EXPECT_CALL(testContext->m_gstPlayer, getSinkChildIfAutoVideoSink(testContext->m_element))
        .WillOnce(Return(&testContext->m_childElement));
    EXPECT_CALL(*testContext->m_glibWrapper, gObjectClassFindProperty(G_OBJECT_GET_CLASS(&testContext->m_childElement),
                                                                      StrEq("frame-step-on-preroll")))
        .WillOnce(Return(&testContext->m_paramSpec));
    EXPECT_CALL(*testContext->m_glibWrapper,
                gObjectSetStub(&testContext->m_childElement, StrEq("frame-step-on-preroll")))
        .Times(2);
    EXPECT_CALL(*testContext->m_gstWrapper, gstEventNewStep(GST_FORMAT_BUFFERS, 1, 1.0, true, false))
        .WillOnce(Return(&testContext->m_event));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementSendEvent(&testContext->m_childElement, &testContext->m_event));
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectUnref(GST_OBJECT(testContext->m_element)));
}

void GenericTasksTestsBase::triggerRenderFrame()
{
    firebolt::rialto::server::tasks::generic::RenderFrame task{testContext->m_context, testContext->m_gstWrapper,
                                                               testContext->m_glibWrapper, testContext->m_gstPlayer};
    task.execute();
}

void GenericTasksTestsBase::shouldGetVideoSinkFailure()
{
    EXPECT_CALL(*testContext->m_glibWrapper, gObjectGetStub(&testContext->m_pipeline, StrEq("video-sink"), _))
        .WillOnce(Invoke(
            [&](gpointer object, const gchar *first_property_name, void *element)
            {
                GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                *elementPtr = nullptr;
            }));
}

void GenericTasksTestsBase::shouldFindPropertyFailure()
{
    EXPECT_CALL(*testContext->m_glibWrapper, gObjectGetStub(&testContext->m_pipeline, StrEq("video-sink"), _))
        .WillOnce(Invoke(
            [&](gpointer object, const gchar *first_property_name, void *element)
            {
                GstElement **elementPtr = reinterpret_cast<GstElement **>(element);
                *elementPtr = testContext->m_element;
            }));

    EXPECT_CALL(testContext->m_gstPlayer, getSinkChildIfAutoVideoSink(testContext->m_element))
        .WillOnce(Return(testContext->m_element));
    EXPECT_CALL(*testContext->m_glibWrapper,
                gObjectClassFindProperty(G_OBJECT_GET_CLASS(testContext->m_element), StrEq("frame-step-on-preroll")))
        .WillOnce(Return(nullptr));
    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectUnref(GST_OBJECT(testContext->m_element)));
}

void GenericTasksTestsBase::shouldInvalidateActiveAudioRequests()
{
    EXPECT_CALL(testContext->m_gstPlayerClient, invalidateActiveRequests(firebolt::rialto::MediaSourceType::AUDIO));
}

void GenericTasksTestsBase::shouldDisableAudioFlag()
{
    EXPECT_CALL(testContext->m_gstPlayer, setAudioVideoFlags(false, true));
}

void GenericTasksTestsBase::triggerRemoveSourceAudio()
{
    firebolt::rialto::server::tasks::generic::RemoveSource task{testContext->m_context, testContext->m_gstPlayer,
                                                                &testContext->m_gstPlayerClient,
                                                                testContext->m_gstWrapper,
                                                                firebolt::rialto::MediaSourceType::AUDIO};
    task.execute();
}

void GenericTasksTestsBase::triggerRemoveSourceVideo()
{
    firebolt::rialto::server::tasks::generic::RemoveSource task{testContext->m_context, testContext->m_gstPlayer,
                                                                &testContext->m_gstPlayerClient,
                                                                testContext->m_gstWrapper,
                                                                firebolt::rialto::MediaSourceType::VIDEO};
    task.execute();
}

void GenericTasksTestsBase::checkAudioSourceRemoved()
{
    EXPECT_TRUE(testContext->m_context.audioSourceRemoved);
}

void GenericTasksTestsBase::checkAudioSourceNotRemoved()
{
    EXPECT_FALSE(testContext->m_context.audioSourceRemoved);
}

void GenericTasksTestsBase::shouldFlushAudioSrcSuccess()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstEventNewFlushStart()).WillOnce(Return(&testContext->m_event));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementSendEvent(&testContext->m_appSrcAudio, &testContext->m_event))
        .WillOnce(Return(TRUE));
    EXPECT_CALL(*testContext->m_gstWrapper, gstEventNewFlushStop(kResetTime)).WillOnce(Return(&testContext->m_event2));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementSendEvent(&testContext->m_appSrcAudio, &testContext->m_event2))
        .WillOnce(Return(TRUE));
}

void GenericTasksTestsBase::shouldFlushAudioSrcFailure()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstEventNewFlushStart()).WillOnce(Return(&testContext->m_event));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementSendEvent(&testContext->m_appSrcAudio, &testContext->m_event))
        .WillOnce(Return(FALSE));
    EXPECT_CALL(*testContext->m_gstWrapper, gstEventNewFlushStop(kResetTime)).WillOnce(Return(&testContext->m_event2));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementSendEvent(&testContext->m_appSrcAudio, &testContext->m_event2))
        .WillOnce(Return(FALSE));
}

void GenericTasksTestsBase::shouldReadAudioData()
{
    EXPECT_CALL(*testContext->m_dataReader, readData()).WillOnce(Invoke([&]() { return buildAudioSamples(); }));
}

void GenericTasksTestsBase::shouldReadVideoData()
{
    EXPECT_CALL(*testContext->m_dataReader, readData()).WillOnce(Invoke([&]() { return buildVideoSamples(); }));
}

void GenericTasksTestsBase::triggerReadShmDataAndAttachSamplesAudio()
{
    firebolt::rialto::server::tasks::generic::ReadShmDataAndAttachSamples task{testContext->m_context,
                                                                               testContext->m_gstPlayer,
                                                                               testContext->m_dataReader};
    task.execute();
    EXPECT_EQ(testContext->m_context.audioBuffers.size(), 2);
}

void GenericTasksTestsBase::triggerReadShmDataAndAttachSamplesVideo()
{
    firebolt::rialto::server::tasks::generic::ReadShmDataAndAttachSamples task{testContext->m_context,
                                                                               testContext->m_gstPlayer,
                                                                               testContext->m_dataReader};
    task.execute();
    EXPECT_EQ(testContext->m_context.videoBuffers.size(), 2);
}

void GenericTasksTestsBase::shouldFlushAudio()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstBufferUnref(&testContext->m_audioBuffer));
    EXPECT_CALL(testContext->m_gstPlayerClient, invalidateActiveRequests(firebolt::rialto::MediaSourceType::AUDIO));
    EXPECT_CALL(testContext->m_gstPlayerClient, notifySourceFlushed(firebolt::rialto::MediaSourceType::AUDIO));
}

void GenericTasksTestsBase::shouldFlushVideo()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstBufferUnref(&testContext->m_videoBuffer));
    EXPECT_CALL(testContext->m_gstPlayerClient, invalidateActiveRequests(firebolt::rialto::MediaSourceType::VIDEO));
    EXPECT_CALL(testContext->m_gstPlayerClient, notifySourceFlushed(firebolt::rialto::MediaSourceType::VIDEO));
}

void GenericTasksTestsBase::triggerFlush(firebolt::rialto::MediaSourceType sourceType)
{
    firebolt::rialto::server::tasks::generic::Flush task{testContext->m_context, &testContext->m_gstPlayerClient,
                                                         testContext->m_gstWrapper, sourceType, kResetTime};
    task.execute();
}

void GenericTasksTestsBase::checkAudioFlushed()
{
    EXPECT_EQ(testContext->m_context.audioBuffers.size(), 0);
    EXPECT_EQ(testContext->m_context.videoBuffers.size(), 1);
    EXPECT_FALSE(testContext->m_context.audioNeedData);
    EXPECT_FALSE(testContext->m_context.audioNeedDataPending);
    EXPECT_TRUE(testContext->m_context.videoNeedData);
    EXPECT_TRUE(testContext->m_context.videoNeedDataPending);
    EXPECT_FALSE(testContext->m_context.eosNotified);
    EXPECT_EQ(testContext->m_context.endOfStreamInfo.find(firebolt::rialto::MediaSourceType::AUDIO),
              testContext->m_context.endOfStreamInfo.end());
    EXPECT_NE(testContext->m_context.endOfStreamInfo.find(firebolt::rialto::MediaSourceType::VIDEO),
              testContext->m_context.endOfStreamInfo.end());
}

void GenericTasksTestsBase::checkVideoFlushed()
{
    EXPECT_EQ(testContext->m_context.audioBuffers.size(), 1);
    EXPECT_EQ(testContext->m_context.videoBuffers.size(), 0);
    EXPECT_TRUE(testContext->m_context.audioNeedData);
    EXPECT_TRUE(testContext->m_context.audioNeedDataPending);
    EXPECT_FALSE(testContext->m_context.videoNeedData);
    EXPECT_FALSE(testContext->m_context.videoNeedDataPending);
    EXPECT_FALSE(testContext->m_context.eosNotified);
    EXPECT_NE(testContext->m_context.endOfStreamInfo.find(firebolt::rialto::MediaSourceType::AUDIO),
              testContext->m_context.endOfStreamInfo.end());
    EXPECT_EQ(testContext->m_context.endOfStreamInfo.find(firebolt::rialto::MediaSourceType::VIDEO),
              testContext->m_context.endOfStreamInfo.end());
}

void GenericTasksTestsBase::shouldFlushVideoSrcSuccess()
{
    EXPECT_CALL(*testContext->m_gstWrapper, gstEventNewFlushStart()).WillOnce(Return(&testContext->m_event));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementSendEvent(&testContext->m_appSrcVideo, &testContext->m_event))
        .WillOnce(Return(TRUE));
    EXPECT_CALL(*testContext->m_gstWrapper, gstEventNewFlushStop(kResetTime)).WillOnce(Return(&testContext->m_event2));
    EXPECT_CALL(*testContext->m_gstWrapper, gstElementSendEvent(&testContext->m_appSrcVideo, &testContext->m_event2))
        .WillOnce(Return(TRUE));
}

void GenericTasksTestsBase::triggerSetSourcePosition(firebolt::rialto::MediaSourceType sourceType)
{
    firebolt::rialto::server::tasks::generic::SetSourcePosition task{testContext->m_context,
                                                                     &testContext->m_gstPlayerClient,
                                                                     testContext->m_gstWrapper,
                                                                     sourceType,
                                                                     kPosition,
                                                                     kResetTime};
    task.execute();
}

void GenericTasksTestsBase::checkInitialPositionSet(firebolt::rialto::MediaSourceType sourceType)
{
    GstElement *source = sourceType == firebolt::rialto::MediaSourceType::AUDIO ? &testContext->m_appSrcAudio
                                                                                : &testContext->m_appSrcVideo;
    ASSERT_NE(testContext->m_context.initialPositions.end(), testContext->m_context.initialPositions.find(source));
    ASSERT_EQ(testContext->m_context.initialPositions.at(source).size(), 1);
    EXPECT_EQ(testContext->m_context.initialPositions.at(source)[0].position, kPosition);
    EXPECT_EQ(testContext->m_context.initialPositions.at(source)[0].resetTime, kResetTime);
}

void GenericTasksTestsBase::checkInitialPositionNotSet(firebolt::rialto::MediaSourceType sourceType)
{
    GstElement *source = sourceType == firebolt::rialto::MediaSourceType::AUDIO ? &testContext->m_appSrcAudio
                                                                                : &testContext->m_appSrcVideo;
    EXPECT_EQ(testContext->m_context.initialPositions.end(), testContext->m_context.initialPositions.find(source));
}

void GenericTasksTestsBase::triggerProcessAudioGap()
{
    firebolt::rialto::server::tasks::generic::ProcessAudioGap task{testContext->m_context,
                                                                   testContext->m_gstWrapper,
                                                                   testContext->m_glibWrapper,
                                                                   testContext->m_rdkGstreamerUtilsWrapper,
                                                                   kPosition,
                                                                   static_cast<uint32_t>(kDuration),
                                                                   kDiscontinuityGap,
                                                                   kIsAudioAac};
    task.execute();
}

void GenericTasksTestsBase::shouldProcessAudioGap()
{
    EXPECT_CALL(*(testContext->m_rdkGstreamerUtilsWrapper),
                processAudioGap(testContext->m_context.pipeline, kPosition, static_cast<uint32_t>(kDuration),
                                kDiscontinuityGap, kIsAudioAac));
}

void GenericTasksTestsBase::triggerFailToCastAudioSource()
{
    std::unique_ptr<IMediaPipeline::MediaSource> source =
        std::make_unique<MediaAudioSourceTest>(SourceConfigType::AUDIO, kId);
    firebolt::rialto::server::tasks::generic::AttachSource task{testContext->m_context,
                                                                testContext->m_gstWrapper,
                                                                testContext->m_glibWrapper,
                                                                testContext->m_rdkGstreamerUtilsWrapper,
                                                                testContext->m_gstPlayer,
                                                                source};
    task.execute();
    EXPECT_EQ(0, testContext->m_context.streamInfo.size());
    EXPECT_EQ(testContext->m_context.streamInfo.end(),
              testContext->m_context.streamInfo.find(firebolt::rialto::MediaSourceType::AUDIO));
}

void GenericTasksTestsBase::triggerFailToCastVideoSource()
{
    std::unique_ptr<IMediaPipeline::MediaSource> source =
        std::make_unique<MediaVideoSourceTest>(SourceConfigType::VIDEO, kId);
    firebolt::rialto::server::tasks::generic::AttachSource task{testContext->m_context,
                                                                testContext->m_gstWrapper,
                                                                testContext->m_glibWrapper,
                                                                testContext->m_rdkGstreamerUtilsWrapper,
                                                                testContext->m_gstPlayer,
                                                                source};
    task.execute();
    EXPECT_EQ(0, testContext->m_context.streamInfo.size());
    EXPECT_EQ(testContext->m_context.streamInfo.end(),
              testContext->m_context.streamInfo.find(firebolt::rialto::MediaSourceType::VIDEO));
}

void GenericTasksTestsBase::triggerFailToCastDolbyVisionSource()
{
    std::unique_ptr<IMediaPipeline::MediaSource> source =
        std::make_unique<MediaVideoDolbyVisionSourceTest>(SourceConfigType::VIDEO_DOLBY_VISION, "video/h264",
                                                          kDolbyVisionProfile);
    firebolt::rialto::server::tasks::generic::AttachSource task{testContext->m_context,
                                                                testContext->m_gstWrapper,
                                                                testContext->m_glibWrapper,
                                                                testContext->m_rdkGstreamerUtilsWrapper,
                                                                testContext->m_gstPlayer,
                                                                source};
    task.execute();
    EXPECT_EQ(0, testContext->m_context.streamInfo.size());
    EXPECT_EQ(testContext->m_context.streamInfo.end(),
              testContext->m_context.streamInfo.find(firebolt::rialto::MediaSourceType::VIDEO));
}

void GenericTasksTestsBase::shouldFailToSetImmediateOutputIfSinkIsNull()
{
    EXPECT_CALL(testContext->m_gstPlayer, getSink(MediaSourceType::VIDEO)).WillOnce(Return(nullptr));
}

void GenericTasksTestsBase::shouldFailToSetImmediateOutputIfPropertyDoesntExist()
{
    GstElement *videoSink = testContext->m_element;
    EXPECT_CALL(testContext->m_gstPlayer, getSink(MediaSourceType::VIDEO)).WillOnce(Return(videoSink));

    EXPECT_CALL(*testContext->m_glibWrapper,
                gObjectClassFindProperty(G_OBJECT_GET_CLASS(videoSink), StrEq("immediate-output")))
        .WillOnce(Return(nullptr));
}

void GenericTasksTestsBase::shouldSetImmediateOutput()
{
    GstElement *videoSink = testContext->m_element;
    EXPECT_CALL(testContext->m_gstPlayer, getSink(MediaSourceType::VIDEO)).WillOnce(Return(videoSink));

    GParamSpec prop{};
    EXPECT_CALL(*testContext->m_glibWrapper,
                gObjectClassFindProperty(G_OBJECT_GET_CLASS(videoSink), StrEq("immediate-output")))
        .WillOnce(Return(&prop));

    EXPECT_CALL(testContext->m_gstPlayer, getSinkChildIfAutoVideoSink(testContext->m_element)).WillOnce(Return(videoSink));

    EXPECT_CALL(*testContext->m_glibWrapper, gObjectSetStub(_, StrEq("immediate-output"))).Times(1);

    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectUnref(videoSink)).Times(1);
}

void GenericTasksTestsBase::triggerSetImmediateOutput()
{
    firebolt::rialto::server::tasks::generic::SetImmediateOutput task{testContext->m_gstPlayer, testContext->m_gstWrapper,
                                                                      testContext->m_glibWrapper,
                                                                      MediaSourceType::VIDEO, true};
    task.execute();
}

void GenericTasksTestsBase::shouldFailToSetLowLatencyIfSinkIsNull()
{
    EXPECT_CALL(testContext->m_gstPlayer, getSink(MediaSourceType::AUDIO)).WillOnce(Return(nullptr));
}

void GenericTasksTestsBase::shouldFailToSetLowLatencyIfPropertyDoesntExist()
{
    GstElement *audioSink = testContext->m_element;
    EXPECT_CALL(testContext->m_gstPlayer, getSink(MediaSourceType::AUDIO)).WillOnce(Return(audioSink));

    EXPECT_CALL(*testContext->m_glibWrapper,
                gObjectClassFindProperty(G_OBJECT_GET_CLASS(audioSink), StrEq("low-latency")))
        .WillOnce(Return(nullptr));
}

void GenericTasksTestsBase::shouldSetLowLatency()
{
    GstElement *audioSink = testContext->m_element;
    EXPECT_CALL(testContext->m_gstPlayer, getSink(MediaSourceType::AUDIO)).WillOnce(Return(audioSink));

    GParamSpec prop{};
    EXPECT_CALL(*testContext->m_glibWrapper,
                gObjectClassFindProperty(G_OBJECT_GET_CLASS(audioSink), StrEq("low-latency")))
        .WillOnce(Return(&prop));

    // EXPECT_CALL(testContext->m_gstPlayer, getSinkChildIfAutoVideoSink(audioSink)).WillOnce(Return(audioSink));

    EXPECT_CALL(*testContext->m_glibWrapper, gObjectSetStub(_, StrEq("low-latency"))).Times(1);

    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectUnref(audioSink)).Times(1);
}

void GenericTasksTestsBase::triggerSetLowLatency()
{
    firebolt::rialto::server::tasks::generic::SetLowLatency task{testContext->m_gstPlayer, testContext->m_gstWrapper,
                                                                 testContext->m_glibWrapper, true};
    task.execute();
}

void GenericTasksTestsBase::shouldFailToSetSyncIfSinkIsNull()
{
    EXPECT_CALL(testContext->m_gstPlayer, getSink(MediaSourceType::AUDIO)).WillOnce(Return(nullptr));
}

void GenericTasksTestsBase::shouldFailToSetSyncIfPropertyDoesntExist()
{
    GstElement *audioSink = testContext->m_element;
    EXPECT_CALL(testContext->m_gstPlayer, getSink(MediaSourceType::AUDIO)).WillOnce(Return(audioSink));

    EXPECT_CALL(*testContext->m_glibWrapper, gObjectClassFindProperty(G_OBJECT_GET_CLASS(audioSink), StrEq("sync")))
        .WillOnce(Return(nullptr));
}

void GenericTasksTestsBase::shouldSetSync()
{
    GstElement *audioSink = testContext->m_element;
    EXPECT_CALL(testContext->m_gstPlayer, getSink(MediaSourceType::AUDIO)).WillOnce(Return(audioSink));

    GParamSpec prop{};
    EXPECT_CALL(*testContext->m_glibWrapper, gObjectClassFindProperty(G_OBJECT_GET_CLASS(audioSink), StrEq("sync")))
        .WillOnce(Return(&prop));

    // EXPECT_CALL(testContext->m_gstPlayer, getSinkChildIfAutoVideoSink(audioSink)).WillOnce(Return(audioSink));

    EXPECT_CALL(*testContext->m_glibWrapper, gObjectSetStub(_, StrEq("sync"))).Times(1);

    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectUnref(audioSink)).Times(1);
}

void GenericTasksTestsBase::triggerSetSync()
{
    firebolt::rialto::server::tasks::generic::SetSync task{testContext->m_gstPlayer, testContext->m_gstWrapper,
                                                           testContext->m_glibWrapper, true};
    task.execute();
}

void GenericTasksTestsBase::shouldFailToSetSyncOffIfDecoderIsNull()
{
    EXPECT_CALL(testContext->m_gstPlayer, getDecoder(MediaSourceType::AUDIO)).WillOnce(Return(nullptr));
}

void GenericTasksTestsBase::shouldFailToSetSyncOffIfPropertyDoesntExist()
{
    GstElement *audioDecoder = testContext->m_element;
    EXPECT_CALL(testContext->m_gstPlayer, getDecoder(MediaSourceType::AUDIO)).WillOnce(Return(audioDecoder));

    EXPECT_CALL(*testContext->m_glibWrapper,
                gObjectClassFindProperty(G_OBJECT_GET_CLASS(audioDecoder), StrEq("sync-off")))
        .WillOnce(Return(nullptr));
}

void GenericTasksTestsBase::shouldSetSyncOff()
{
    GstElement *audioDecoder = testContext->m_element;
    EXPECT_CALL(testContext->m_gstPlayer, getDecoder(MediaSourceType::AUDIO)).WillOnce(Return(audioDecoder));

    GParamSpec prop{};
    EXPECT_CALL(*testContext->m_glibWrapper,
                gObjectClassFindProperty(G_OBJECT_GET_CLASS(audioDecoder), StrEq("sync-off")))
        .WillOnce(Return(&prop));

    EXPECT_CALL(*testContext->m_glibWrapper, gObjectSetStub(_, StrEq("sync-off"))).Times(1);

    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectUnref(audioDecoder)).Times(1);
}

void GenericTasksTestsBase::triggerSetSyncOff()
{
    firebolt::rialto::server::tasks::generic::SetSyncOff task{testContext->m_gstPlayer, testContext->m_gstWrapper,
                                                              testContext->m_glibWrapper, true};
    task.execute();
}

void GenericTasksTestsBase::shouldFailToSetStreamSyncModeIfDecoderIsNull()
{
    EXPECT_CALL(testContext->m_gstPlayer, getDecoder(MediaSourceType::AUDIO)).WillOnce(Return(nullptr));
}

void GenericTasksTestsBase::shouldFailToSetStreamSyncModeIfPropertyDoesntExist()
{
    GstElement *audioDecoder = testContext->m_element;
    EXPECT_CALL(testContext->m_gstPlayer, getDecoder(MediaSourceType::AUDIO)).WillOnce(Return(audioDecoder));

    EXPECT_CALL(*testContext->m_glibWrapper,
                gObjectClassFindProperty(G_OBJECT_GET_CLASS(audioDecoder), StrEq("stream-sync-mode")))
        .WillOnce(Return(nullptr));
}

void GenericTasksTestsBase::shouldSetStreamSyncMode()
{
    GstElement *audioDecoder = testContext->m_element;
    EXPECT_CALL(testContext->m_gstPlayer, getDecoder(MediaSourceType::AUDIO)).WillOnce(Return(audioDecoder));

    GParamSpec prop{};
    EXPECT_CALL(*testContext->m_glibWrapper,
                gObjectClassFindProperty(G_OBJECT_GET_CLASS(audioDecoder), StrEq("stream-sync-mode")))
        .WillOnce(Return(&prop));

    EXPECT_CALL(*testContext->m_glibWrapper, gObjectSetStub(_, StrEq("stream-sync-mode"))).Times(1);

    EXPECT_CALL(*testContext->m_gstWrapper, gstObjectUnref(audioDecoder)).Times(1);
}

void GenericTasksTestsBase::triggerSetStreamSyncMode()
{
    firebolt::rialto::server::tasks::generic::SetStreamSyncMode task{testContext->m_gstPlayer, testContext->m_gstWrapper,
                                                                     testContext->m_glibWrapper, true};
    task.execute();
}
