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

#include "MediaPipelineTest.h"
#include "ActionTraits.h"
#include "ConfigureAction.h"
#include "Constants.h"
#include "ExpectMessage.h"
#include "IMediaFrameWriter.h"
#include "Matchers.h"
#include "MediaCommon.h"
#include "MessageBuilders.h"
#include "SegmentBuilder.h"
#include <gst/audio/audio.h>
#include <string>
#include <vector>

using testing::_;
using testing::AtLeast;
using testing::DoAll;
using testing::Invoke;
using testing::Return;
using testing::SaveArg;
using testing::SaveArgPointee;
using testing::SetArgPointee;
using testing::StrEq;

namespace
{
const std::string kNullStateName{"NULL"};
const std::string kPausedStateName{"PAUSED"};
const std::string kPlayingStateName{"PLAYING"};
constexpr GType kGstPlayFlagsType{static_cast<GType>(123)};
constexpr unsigned kNeededDataLength{1};
constexpr std::chrono::milliseconds kWorkerTimeout{200};
GstAudioClippingMeta kClippingMeta{};
} // namespace

namespace firebolt::rialto::server::ct
{
MediaPipelineTest::MediaPipelineTest()
{
    willConfigureSocket();
    configureSutInActiveState();
    connectClient();
    initShm();
}

MediaPipelineTest::~MediaPipelineTest()
{
    positionUpdatesShouldNotBeReceivedFromNow();
}

void MediaPipelineTest::gstPlayerWillBeCreated()
{
    m_gstreamerStub.setupPipeline();
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryFind(StrEq("rialtosrc"))).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstElementRegister(0, StrEq("rialtosrc"), _, _));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(StrEq("playbin"), _)).WillOnce(Return(&m_pipeline));
    EXPECT_CALL(*m_glibWrapperMock, gTypeFromName(StrEq("GstPlayFlags"))).Times(4).WillRepeatedly(Return(kGstPlayFlagsType));
    EXPECT_CALL(*m_glibWrapperMock, gTypeClassRef(kGstPlayFlagsType)).Times(4).WillRepeatedly(Return(&m_flagsClass));

    EXPECT_CALL(*m_glibWrapperMock, gFlagsGetValueByNick(&m_flagsClass, StrEq("audio"))).WillOnce(Return(&m_audioFlag));
    EXPECT_CALL(*m_glibWrapperMock, gFlagsGetValueByNick(&m_flagsClass, StrEq("video"))).WillOnce(Return(&m_videoFlag));
    EXPECT_CALL(*m_glibWrapperMock, gFlagsGetValueByNick(&m_flagsClass, StrEq("native-video")))
        .WillOnce(Return(&m_nativeVideoFlag));
    EXPECT_CALL(*m_glibWrapperMock, gFlagsGetValueByNick(&m_flagsClass, StrEq("text"))).WillOnce(Return(&m_subtitleFlag));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryFind(StrEq("brcmaudiosink"))).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_pipeline, StrEq("flags")));
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_pipeline, StrEq("uri")));
    EXPECT_CALL(*m_gstWrapperMock, gstBinGetByName(GST_BIN(&m_pipeline), StrEq("playsink"))).WillOnce(Return(&m_playsink));
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_playsink, StrEq("send-event-mode")));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_playsink));

    // In case of longer testruns, GstPlayer may request to query position
    EXPECT_CALL(*m_gstWrapperMock, gstElementQueryPosition(&m_pipeline, GST_FORMAT_TIME, _))
        .Times(AtLeast(0))
        .WillRepeatedly(Invoke(
            [](GstElement *, GstFormat, gint64 *cur)
            {
                *cur = kCurrentPosition;
                return true;
            }));
}

void MediaPipelineTest::gstPlayerWillBeDestructed()
{
    EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(&m_pipeline, GST_STATE_NULL))
        .WillOnce(Return(GST_STATE_CHANGE_SUCCESS));
    EXPECT_CALL(*m_gstWrapperMock, gstPipelineGetBus(GST_PIPELINE(&m_pipeline))).WillOnce(Return(&m_bus));
    EXPECT_CALL(*m_gstWrapperMock, gstBusSetSyncHandler(&m_bus, nullptr, nullptr, nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_bus));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_pipeline));
}

void MediaPipelineTest::audioSourceWillBeAttached()
{
    EXPECT_CALL(*m_gstWrapperMock, gstCapsNewEmptySimple(StrEq("audio/mpeg"))).WillOnce(Return(&m_audioCaps));
    EXPECT_CALL(*m_gstWrapperMock,
                gstCapsSetSimpleStringStub(&m_audioCaps, StrEq("alignment"), G_TYPE_STRING, StrEq("nal")));
    EXPECT_CALL(*m_gstWrapperMock,
                gstCapsSetSimpleStringStub(&m_audioCaps, StrEq("stream-format"), G_TYPE_STRING, StrEq("raw")));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&m_audioCaps, StrEq("mpegversion"), G_TYPE_INT, 4));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&m_audioCaps, StrEq("channels"), G_TYPE_INT, kNumOfChannels));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&m_audioCaps, StrEq("rate"), G_TYPE_INT, kSampleRate));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsToString(&m_audioCaps)).WillOnce(Return(&m_audioCapsStr));
    EXPECT_CALL(*m_glibWrapperMock, gFree(&m_audioCapsStr));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(StrEq("appsrc"), StrEq("audsrc")))
        .WillOnce(Return(GST_ELEMENT(&m_audioAppSrc)));
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetCaps(&m_audioAppSrc, &m_audioCaps));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&m_audioCaps)).WillOnce(Invoke(this, &MediaPipelineTest::workerFinished));
}

void MediaPipelineTest::videoSourceWillBeAttached()
{
    EXPECT_CALL(*m_gstWrapperMock, gstCapsNewEmptySimple(StrEq("video/x-h264"))).WillOnce(Return(&m_videoCaps));
    EXPECT_CALL(*m_gstWrapperMock,
                gstCapsSetSimpleStringStub(&m_videoCaps, StrEq("alignment"), G_TYPE_STRING, StrEq("nal")));
    EXPECT_CALL(*m_gstWrapperMock,
                gstCapsSetSimpleStringStub(&m_videoCaps, StrEq("stream-format"), G_TYPE_STRING, StrEq("raw")));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&m_videoCaps, StrEq("width"), G_TYPE_INT, kWidth));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&m_videoCaps, StrEq("height"), G_TYPE_INT, kHeight));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsToString(&m_videoCaps)).WillOnce(Return(&m_videoCapsStr));
    EXPECT_CALL(*m_glibWrapperMock, gFree(&m_videoCapsStr));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryMake(StrEq("appsrc"), StrEq("vidsrc")))
        .WillOnce(Return(GST_ELEMENT(&m_videoAppSrc)));
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetCaps(&m_videoAppSrc, &m_videoCaps));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&m_videoCaps)).WillOnce(Invoke(this, &MediaPipelineTest::workerFinished));
}

void MediaPipelineTest::sourceWillBeSetup()
{
    EXPECT_CALL(*m_gstWrapperMock, gstObjectRef(&m_rialtoSource));
    // Source will be unreferenced when GstPlayer is destructed.
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_rialtoSource));
}

void MediaPipelineTest::willSetupAndAddSource(GstAppSrc *appSrc)
{
    const guint64 kMaxBytes = ((appSrc == &m_audioAppSrc) ? (512 * 1024) : (8 * 1024 * 1024));
    m_gstreamerStub.setupAppSrcCallbacks(appSrc);
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(GST_ELEMENT(appSrc), StrEq("block")));
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(GST_ELEMENT(appSrc), StrEq("format")));
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(GST_ELEMENT(appSrc), StrEq("stream-type")));
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(GST_ELEMENT(appSrc), StrEq("min-percent")));
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(GST_ELEMENT(appSrc), StrEq("handle-segment-change")));
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetMaxBytes(appSrc, kMaxBytes));
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetStreamType(appSrc, GST_APP_STREAM_TYPE_SEEKABLE));
    EXPECT_CALL(*m_glibWrapperMock, gStrdupPrintfStub(_)).WillOnce(Return(m_sourceName.data())).RetiresOnSaturation();
    EXPECT_CALL(*m_gstWrapperMock, gstBinAdd(GST_BIN(&m_rialtoSource), GST_ELEMENT(appSrc)));
    EXPECT_CALL(*m_gstWrapperMock, gstElementGetStaticPad(GST_ELEMENT(appSrc), StrEq("src"))).WillOnce(Return(&m_pad));
    EXPECT_CALL(*m_gstWrapperMock, gstGhostPadNew(StrEq(m_sourceName), &m_pad))
        .WillOnce(Return(&m_ghostPad))
        .RetiresOnSaturation();
    EXPECT_CALL(*m_gstWrapperMock, gstPadSetQueryFunction(&m_ghostPad, NotNullMatcher())).RetiresOnSaturation();
    EXPECT_CALL(*m_gstWrapperMock, gstPadSetActive(&m_ghostPad, TRUE)).RetiresOnSaturation();
    EXPECT_CALL(*m_gstWrapperMock, gstElementAddPad(GST_ELEMENT(&m_rialtoSource), &m_ghostPad)).RetiresOnSaturation();
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_pad)).RetiresOnSaturation();
    EXPECT_CALL(*m_gstWrapperMock, gstElementSyncStateWithParent(GST_ELEMENT(appSrc)));
    EXPECT_CALL(*m_glibWrapperMock, gFree(PtrStrMatcher(m_sourceName.data()))).RetiresOnSaturation();
}

void MediaPipelineTest::willFinishSetupAndAddSource()
{
    EXPECT_CALL(*m_gstWrapperMock, gstElementNoMorePads(GST_ELEMENT(&m_rialtoSource)));
}

// Only need to wait/notify here if there is no waiting for the NeedData Event
void MediaPipelineTest::willPushAudioData(const std::unique_ptr<IMediaPipeline::MediaSegment> &segment,
                                          GstBuffer &buffer, GstCaps &capsCopy, bool shouldNotify)
{
    std::string dataCopy(segment->getData(), segment->getData() + segment->getDataLength());
    EXPECT_CALL(*m_gstWrapperMock, gstBufferNewAllocate(nullptr, segment->getDataLength(), nullptr))
        .InSequence(m_bufferAllocateSeq)
        .WillOnce(Return(&buffer))
        .RetiresOnSaturation();
    EXPECT_CALL(*m_gstWrapperMock, gstBufferFill(&buffer, 0, BufferMatcher(dataCopy), segment->getDataLength()))
        .WillOnce(Return(segment->getDataLength()))
        .RetiresOnSaturation();
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcGetCaps(&m_audioAppSrc)).WillOnce(Return(&m_audioCaps)).RetiresOnSaturation();
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCopy(&m_audioCaps)).WillOnce(Return(&capsCopy)).RetiresOnSaturation();
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&capsCopy, StrEq("rate"), G_TYPE_INT, kSampleRate));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&capsCopy, StrEq("channels"), G_TYPE_INT, kNumOfChannels));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferAddAudioClippingMeta(&buffer, GST_FORMAT_TIME, kClippingStart, kClippingEnd))
        .WillOnce(Return(&kClippingMeta));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsEqual(&m_audioCaps, &capsCopy)).WillOnce(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetCaps(&m_audioAppSrc, &capsCopy));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&m_audioCaps)).RetiresOnSaturation();
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&capsCopy));
    if (!shouldNotify)
    {
        EXPECT_CALL(*m_gstWrapperMock, gstAppSrcPushBuffer(&m_audioAppSrc, _))
            .InSequence(m_writeBufferSeq)
            .RetiresOnSaturation();
    }
    else
    {
        EXPECT_CALL(*m_gstWrapperMock, gstAppSrcPushBuffer(&m_audioAppSrc, _))
            .InSequence(m_writeBufferSeq)
            .WillOnce(Invoke(
                [this](GstAppSrc *appsrc, GstBuffer *buffer)
                {
                    workerFinished();
                    return GST_FLOW_OK;
                }))
            .RetiresOnSaturation();
    }
}

// Only need to wait/notify here if there is no waiting for the NeedData Event
void MediaPipelineTest::willPushVideoData(const std::unique_ptr<IMediaPipeline::MediaSegment> &segment,
                                          GstBuffer &buffer, GstCaps &capsCopy, bool shouldNotify)
{
    std::string dataCopy(segment->getData(), segment->getData() + segment->getDataLength());
    EXPECT_CALL(*m_gstWrapperMock, gstBufferNewAllocate(nullptr, segment->getDataLength(), nullptr))
        .InSequence(m_bufferAllocateSeq)
        .WillOnce(Return(&buffer))
        .RetiresOnSaturation();
    EXPECT_CALL(*m_gstWrapperMock, gstBufferFill(&buffer, 0, BufferMatcher(dataCopy), segment->getDataLength()))
        .WillOnce(Return(segment->getDataLength()))
        .RetiresOnSaturation();
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcGetCaps(&m_videoAppSrc)).WillOnce(Return(&m_videoCaps)).RetiresOnSaturation();
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCopy(&m_videoCaps)).WillOnce(Return(&capsCopy)).RetiresOnSaturation();
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&capsCopy, StrEq("width"), G_TYPE_INT, kWidth));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&capsCopy, StrEq("height"), G_TYPE_INT, kHeight));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleFractionStub(&capsCopy, StrEq("framerate"), GST_TYPE_FRACTION, _, _));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsEqual(&m_videoCaps, &capsCopy)).WillOnce(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetCaps(&m_videoAppSrc, &capsCopy));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&m_videoCaps)).RetiresOnSaturation();
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&capsCopy));
    if (!shouldNotify)
    {
        EXPECT_CALL(*m_gstWrapperMock, gstAppSrcPushBuffer(&m_videoAppSrc, _))
            .InSequence(m_writeBufferSeq)
            .RetiresOnSaturation();
    }
    else
    {
        EXPECT_CALL(*m_gstWrapperMock, gstAppSrcPushBuffer(&m_videoAppSrc, _))
            .InSequence(m_writeBufferSeq)
            .WillOnce(Invoke(
                [this](GstAppSrc *appsrc, GstBuffer *buffer)
                {
                    workerFinished();
                    return GST_FLOW_OK;
                }))
            .RetiresOnSaturation();
    }
}

void MediaPipelineTest::willPushAudioSample(const std::unique_ptr<IMediaPipeline::MediaSegment> &segment,
                                            GstBuffer &buffer, GstCaps &capsCopy)
{
    std::string dataCopy(segment->getData(), segment->getData() + segment->getDataLength());
    EXPECT_CALL(*m_gstWrapperMock, gstBufferNewAllocate(nullptr, segment->getDataLength(), nullptr))
        .InSequence(m_bufferAllocateSeq)
        .WillOnce(Return(&buffer))
        .RetiresOnSaturation();
    EXPECT_CALL(*m_gstWrapperMock, gstBufferFill(&buffer, 0, BufferMatcher(dataCopy), segment->getDataLength()))
        .WillOnce(Return(segment->getDataLength()))
        .RetiresOnSaturation();
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcGetCaps(&m_audioAppSrc))
        .Times(2)
        .WillRepeatedly(Return(&m_audioCaps))
        .RetiresOnSaturation();
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCopy(&m_audioCaps)).WillOnce(Return(&capsCopy)).RetiresOnSaturation();
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&capsCopy, StrEq("rate"), G_TYPE_INT, kSampleRate));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&capsCopy, StrEq("channels"), G_TYPE_INT, kNumOfChannels));
    EXPECT_CALL(*m_gstWrapperMock, gstBufferAddAudioClippingMeta(&buffer, GST_FORMAT_TIME, kClippingStart, kClippingEnd))
        .WillOnce(Return(&kClippingMeta));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsEqual(&m_audioCaps, &capsCopy)).WillOnce(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetCaps(&m_audioAppSrc, &capsCopy));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&m_audioCaps)).Times(2).RetiresOnSaturation();
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&capsCopy));
    EXPECT_CALL(*m_gstWrapperMock, gstSegmentNew()).WillOnce(Return(&m_segment));
    EXPECT_CALL(*m_gstWrapperMock, gstSegmentInit(&m_segment, GST_FORMAT_TIME));
    EXPECT_CALL(*m_gstWrapperMock,
                gstSegmentDoSeek(&m_segment, kRate, GST_FORMAT_TIME, GST_SEEK_FLAG_NONE, GST_SEEK_TYPE_SET, kPosition,
                                 GST_SEEK_TYPE_SET, kStopPosition, nullptr))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_gstWrapperMock, gstSampleNew(nullptr, &m_audioCaps, &m_segment, nullptr)).WillOnce(Return(m_sample));
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcPushSample(&m_audioAppSrc, m_sample));
    EXPECT_CALL(*m_gstWrapperMock, gstSampleUnref(m_sample));
    EXPECT_CALL(*m_gstWrapperMock, gstSegmentFree(&m_segment));
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcPushBuffer(&m_audioAppSrc, &buffer)).RetiresOnSaturation();
}

void MediaPipelineTest::willPushVideoSample(const std::unique_ptr<IMediaPipeline::MediaSegment> &segment,
                                            GstBuffer &buffer, GstCaps &capsCopy)
{
    std::string dataCopy(segment->getData(), segment->getData() + segment->getDataLength());
    EXPECT_CALL(*m_gstWrapperMock, gstBufferNewAllocate(nullptr, segment->getDataLength(), nullptr))
        .InSequence(m_bufferAllocateSeq)
        .WillOnce(Return(&buffer))
        .RetiresOnSaturation();
    EXPECT_CALL(*m_gstWrapperMock, gstBufferFill(&buffer, 0, BufferMatcher(dataCopy), segment->getDataLength()))
        .WillOnce(Return(segment->getDataLength()))
        .RetiresOnSaturation();
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcGetCaps(&m_videoAppSrc))
        .Times(2)
        .WillRepeatedly(Return(&m_videoCaps))
        .RetiresOnSaturation();
    EXPECT_CALL(*m_gstWrapperMock, gstCapsCopy(&m_videoCaps)).WillOnce(Return(&capsCopy)).RetiresOnSaturation();
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&capsCopy, StrEq("width"), G_TYPE_INT, kWidth));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleIntStub(&capsCopy, StrEq("height"), G_TYPE_INT, kHeight));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsSetSimpleFractionStub(&capsCopy, StrEq("framerate"), GST_TYPE_FRACTION, _, _));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsIsEqual(&m_videoCaps, &capsCopy)).WillOnce(Return(false));
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetCaps(&m_videoAppSrc, &capsCopy));
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&m_videoCaps)).Times(2).RetiresOnSaturation();
    EXPECT_CALL(*m_gstWrapperMock, gstCapsUnref(&capsCopy));
    EXPECT_CALL(*m_gstWrapperMock, gstSegmentNew()).WillOnce(Return(&m_segment));
    EXPECT_CALL(*m_gstWrapperMock, gstSegmentInit(&m_segment, GST_FORMAT_TIME));
    EXPECT_CALL(*m_gstWrapperMock,
                gstSegmentDoSeek(&m_segment, kRate, GST_FORMAT_TIME, GST_SEEK_FLAG_NONE, GST_SEEK_TYPE_SET, kPosition,
                                 GST_SEEK_TYPE_SET, kStopPosition, nullptr))
        .WillOnce(Return(true));
    EXPECT_CALL(*m_gstWrapperMock, gstSampleNew(nullptr, &m_videoCaps, &m_segment, nullptr)).WillOnce(Return(m_sample));
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcPushSample(&m_videoAppSrc, m_sample));
    EXPECT_CALL(*m_gstWrapperMock, gstSampleUnref(m_sample));
    EXPECT_CALL(*m_gstWrapperMock, gstSegmentFree(&m_segment));
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcPushBuffer(&m_videoAppSrc, &buffer)).RetiresOnSaturation();
}

void MediaPipelineTest::willPause()
{
    EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(&m_pipeline, GST_STATE_PAUSED))
        .WillOnce(Return(GST_STATE_CHANGE_SUCCESS));
}

void MediaPipelineTest::willNotifyPaused()
{
    EXPECT_CALL(*m_gstWrapperMock, gstMessageParseStateChanged(_, _, _, _))
        .WillRepeatedly(DoAll(SetArgPointee<1>(GST_STATE_NULL), SetArgPointee<2>(GST_STATE_PAUSED),
                              SetArgPointee<3>(GST_STATE_NULL)));
    EXPECT_CALL(*m_gstWrapperMock, gstElementStateGetName(GST_STATE_NULL)).WillRepeatedly(Return(kNullStateName.c_str()));
    EXPECT_CALL(*m_gstWrapperMock, gstElementStateGetName(GST_STATE_PAUSED))
        .WillRepeatedly(Return(kPausedStateName.c_str()));
    EXPECT_CALL(*m_gstWrapperMock, gstDebugBinToDotFileWithTs(GST_BIN(&m_pipeline), _, _));
}

void MediaPipelineTest::willPlay()
{
    EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(&m_pipeline, GST_STATE_PLAYING))
        .WillOnce(Return(GST_STATE_CHANGE_SUCCESS));
    EXPECT_CALL(*m_gstWrapperMock, gstMessageParseStateChanged(_, _, _, _))
        .WillRepeatedly(DoAll(SetArgPointee<1>(GST_STATE_NULL), SetArgPointee<2>(GST_STATE_PLAYING),
                              SetArgPointee<3>(GST_STATE_NULL)));
    EXPECT_CALL(*m_gstWrapperMock, gstElementStateGetName(GST_STATE_NULL)).WillRepeatedly(Return(kNullStateName.c_str()));
    EXPECT_CALL(*m_gstWrapperMock, gstElementStateGetName(GST_STATE_PLAYING))
        .WillRepeatedly(Return(kPlayingStateName.c_str()));
    EXPECT_CALL(*m_gstWrapperMock, gstDebugBinToDotFileWithTs(GST_BIN(&m_pipeline), _, _));
}

void MediaPipelineTest::willEos(GstAppSrc *appSrc)
{
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcEndOfStream(appSrc)).WillOnce(Return(GST_FLOW_OK));
}

void MediaPipelineTest::willRemoveAudioSource()
{
    EXPECT_CALL(*m_gstWrapperMock, gstEventNewFlushStart()).WillOnce(Return(&m_flushStartEvent));
    EXPECT_CALL(*m_gstWrapperMock, gstElementSendEvent(GST_ELEMENT(&m_audioAppSrc), &m_flushStartEvent))
        .WillOnce(Return(TRUE));
    EXPECT_CALL(*m_gstWrapperMock, gstEventNewFlushStop(0)).WillOnce(Return(&m_flushStopEvent));
    EXPECT_CALL(*m_gstWrapperMock, gstElementSendEvent(GST_ELEMENT(&m_audioAppSrc), &m_flushStopEvent))
        .WillOnce(Return(TRUE));

    EXPECT_CALL(*m_glibWrapperMock, gTypeFromName(StrEq("GstPlayFlags"))).Times(3).WillRepeatedly(Return(kGstPlayFlagsType));
    EXPECT_CALL(*m_glibWrapperMock, gTypeClassRef(kGstPlayFlagsType)).Times(3).WillRepeatedly(Return(&m_flagsClass));
    EXPECT_CALL(*m_glibWrapperMock, gFlagsGetValueByNick(&m_flagsClass, StrEq("video"))).WillOnce(Return(&m_videoFlag));
    EXPECT_CALL(*m_glibWrapperMock, gFlagsGetValueByNick(&m_flagsClass, StrEq("native-video")))
        .WillOnce(Return(&m_nativeVideoFlag));
    EXPECT_CALL(*m_glibWrapperMock, gFlagsGetValueByNick(&m_flagsClass, StrEq("text"))).WillOnce(Return(&m_subtitleFlag));
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_pipeline, StrEq("flags")))
        .WillOnce(Invoke(this, &MediaPipelineTest::workerFinished));
}

void MediaPipelineTest::willStop()
{
    EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(&m_pipeline, GST_STATE_NULL))
        .WillOnce(Return(GST_STATE_CHANGE_SUCCESS));
    EXPECT_CALL(*m_gstWrapperMock, gstMessageParseStateChanged(_, _, _, _))
        .WillRepeatedly(DoAll(SetArgPointee<1>(GST_STATE_NULL), SetArgPointee<2>(GST_STATE_NULL),
                              SetArgPointee<3>(GST_STATE_NULL)));
    EXPECT_CALL(*m_gstWrapperMock, gstElementStateGetName(GST_STATE_NULL)).WillRepeatedly(Return(kNullStateName.c_str()));
    EXPECT_CALL(*m_gstWrapperMock, gstDebugBinToDotFileWithTs(GST_BIN(&m_pipeline), _, _));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_bus));
}

void MediaPipelineTest::willSetAudioAndVideoFlags()
{
    EXPECT_CALL(*m_glibWrapperMock, gTypeFromName(StrEq("GstPlayFlags"))).Times(4).WillRepeatedly(Return(kGstPlayFlagsType));
    EXPECT_CALL(*m_glibWrapperMock, gTypeClassRef(kGstPlayFlagsType)).Times(4).WillRepeatedly(Return(&m_flagsClass));
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryFind(StrEq("brcmaudiosink"))).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_glibWrapperMock, gFlagsGetValueByNick(&m_flagsClass, StrEq("audio"))).WillOnce(Return(&m_audioFlag));
    EXPECT_CALL(*m_glibWrapperMock, gFlagsGetValueByNick(&m_flagsClass, StrEq("video"))).WillOnce(Return(&m_videoFlag));
    EXPECT_CALL(*m_glibWrapperMock, gFlagsGetValueByNick(&m_flagsClass, StrEq("native-video")))
        .WillOnce(Return(&m_nativeVideoFlag));
    EXPECT_CALL(*m_glibWrapperMock, gFlagsGetValueByNick(&m_flagsClass, StrEq("text"))).WillOnce(Return(&m_subtitleFlag));
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_pipeline, StrEq("flags")));
}

void MediaPipelineTest::createSession()
{
    // Use matchResponse to store session id
    auto request{createCreateSessionRequest(kWidth, kHeight)};
    ConfigureAction<CreateSession>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse([&](const ::firebolt::rialto::CreateSessionResponse &resp) { m_sessionId = resp.session_id(); });
}

void MediaPipelineTest::load()
{
    ExpectMessage<firebolt::rialto::NetworkStateChangeEvent> expectedNetworkStateChange{m_clientStub};

    auto request = createLoadRequest(m_sessionId);
    ConfigureAction<Load>(m_clientStub).send(request).expectSuccess();

    auto receivedNetworkStateChange = expectedNetworkStateChange.getMessage();
    ASSERT_TRUE(receivedNetworkStateChange);
    EXPECT_EQ(receivedNetworkStateChange->session_id(), m_sessionId);
    EXPECT_EQ(receivedNetworkStateChange->state(), ::firebolt::rialto::NetworkStateChangeEvent_NetworkState_BUFFERING);
}

void MediaPipelineTest::attachAudioSource()
{
    auto attachAudioSourceReq{createAttachAudioSourceRequest(m_sessionId)};
    ConfigureAction<AttachSource>(m_clientStub)
        .send(attachAudioSourceReq)
        .expectSuccess()
        .matchResponse([&](const auto &resp) { m_audioSourceId = resp.source_id(); });
    waitWorker();
}

void MediaPipelineTest::attachVideoSource()
{
    auto attachVideoSourceReq{createAttachVideoSourceRequest(m_sessionId)};
    ConfigureAction<AttachSource>(m_clientStub)
        .send(attachVideoSourceReq)
        .expectSuccess()
        .matchResponse([&](const auto &resp) { m_videoSourceId = resp.source_id(); });
    waitWorker();
}

void MediaPipelineTest::setupSource()
{
    m_gstreamerStub.setupRialtoSource();
}

void MediaPipelineTest::indicateAllSourcesAttached()
{
    ExpectMessage<firebolt::rialto::PlaybackStateChangeEvent> expectedPlaybackStateChange(m_clientStub);

    auto allSourcesAttachedReq{createAllSourcesAttachedRequest(m_sessionId)};
    ConfigureAction<AllSourcesAttached>(m_clientStub).send(allSourcesAttachedReq).expectSuccess();

    auto receivedPlaybackStateChange{expectedPlaybackStateChange.getMessage()};
    ASSERT_TRUE(receivedPlaybackStateChange);
    EXPECT_EQ(receivedPlaybackStateChange->session_id(), m_sessionId);
    EXPECT_EQ(receivedPlaybackStateChange->state(), ::firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_IDLE);
}

void MediaPipelineTest::pause()
{
    auto pauseReq{createPauseRequest(m_sessionId)};
    ConfigureAction<Pause>(m_clientStub).send(pauseReq).expectSuccess();
    positionUpdatesShouldNotBeReceivedFromNow();
}

void MediaPipelineTest::notifyPaused()
{
    ExpectMessage<firebolt::rialto::PlaybackStateChangeEvent> expectedPlaybackStateChange{m_clientStub};

    m_gstreamerStub.sendStateChanged(GST_STATE_NULL, GST_STATE_PAUSED, GST_STATE_NULL);

    auto receivedPlaybackStateChange{expectedPlaybackStateChange.getMessage()};
    ASSERT_TRUE(receivedPlaybackStateChange);
    EXPECT_EQ(receivedPlaybackStateChange->session_id(), m_sessionId);
    EXPECT_EQ(receivedPlaybackStateChange->state(), ::firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_PAUSED);
}

void MediaPipelineTest::gstNeedData(GstAppSrc *appSrc, int frameCount)
{
    const int kSourceId = ((appSrc == &m_audioAppSrc) ? m_audioSourceId : m_videoSourceId);
    auto &needDataPtr = ((appSrc == &m_audioAppSrc) ? m_lastAudioNeedData : m_lastVideoNeedData);

    ExpectMessage<firebolt::rialto::NeedMediaDataEvent> expectedNeedData{m_clientStub};
    m_gstreamerStub.needData(appSrc, kNeededDataLength);
    auto receivedNeedData{expectedNeedData.getMessage()};
    ASSERT_TRUE(receivedNeedData);
    EXPECT_EQ(receivedNeedData->session_id(), m_sessionId);
    EXPECT_EQ(receivedNeedData->source_id(), kSourceId);
    EXPECT_EQ(receivedNeedData->frame_count(), frameCount);
    needDataPtr = receivedNeedData;
}

void MediaPipelineTest::pushAudioData(unsigned dataCountToPush, int needDataFrameCount)
{
    // First, generate new data
    std::vector<std::unique_ptr<IMediaPipeline::MediaSegment>> segments(dataCountToPush);
    std::generate(segments.begin(), segments.end(),
                  [&]() { return SegmentBuilder().basicAudioSegment(m_audioSourceId)(); });
    std::vector<GstBuffer> buffers(dataCountToPush);
    std::vector<GstCaps> copies(dataCountToPush);

    // Next, create frame writer
    ASSERT_TRUE(m_lastAudioNeedData);
    std::shared_ptr<MediaPlayerShmInfo> shmInfo{
        std::make_shared<MediaPlayerShmInfo>(MediaPlayerShmInfo{m_lastAudioNeedData->shm_info().max_metadata_bytes(),
                                                                m_lastAudioNeedData->shm_info().metadata_offset(),
                                                                m_lastAudioNeedData->shm_info().media_data_offset(),
                                                                m_lastAudioNeedData->shm_info().max_media_bytes()})};
    auto writer{common::IMediaFrameWriterFactory::getFactory()->createFrameWriter(m_shmHandle.getShm(), shmInfo)};

    // Write frames to shm and add gst expects
    for (unsigned i = 0; i < dataCountToPush; ++i)
    {
        EXPECT_EQ(writer->writeFrame(segments[i]), AddSegmentStatus::OK);
        willPushAudioData(segments[i], buffers[i], copies[i], false);
    }

    // Finally, send HaveData and receive new NeedData
    ExpectMessage<firebolt::rialto::NeedMediaDataEvent> expectedNeedData{m_clientStub};
    auto haveDataReq{createHaveDataRequest(m_sessionId, writer->getNumFrames(), m_lastAudioNeedData->request_id())};
    ConfigureAction<HaveData>(m_clientStub).send(haveDataReq).expectSuccess();
    auto receivedNeedData{expectedNeedData.getMessage()};
    ASSERT_TRUE(receivedNeedData);
    EXPECT_EQ(receivedNeedData->session_id(), m_sessionId);
    EXPECT_EQ(receivedNeedData->source_id(), m_audioSourceId);
    EXPECT_EQ(receivedNeedData->frame_count(), needDataFrameCount);
    m_lastAudioNeedData = receivedNeedData;
}

void MediaPipelineTest::pushVideoData(unsigned dataCountToPush, int needDataFrameCount)
{
    // First, generate new data
    std::vector<std::unique_ptr<IMediaPipeline::MediaSegment>> segments(dataCountToPush);
    std::generate(segments.begin(), segments.end(),
                  [&]() { return SegmentBuilder().basicVideoSegment(m_videoSourceId)(); });
    std::vector<GstBuffer> buffers(dataCountToPush);
    std::vector<GstCaps> copies(dataCountToPush);

    // Next, create frame writer
    ASSERT_TRUE(m_lastVideoNeedData);
    std::shared_ptr<MediaPlayerShmInfo> shmInfo{
        std::make_shared<MediaPlayerShmInfo>(MediaPlayerShmInfo{m_lastVideoNeedData->shm_info().max_metadata_bytes(),
                                                                m_lastVideoNeedData->shm_info().metadata_offset(),
                                                                m_lastVideoNeedData->shm_info().media_data_offset(),
                                                                m_lastVideoNeedData->shm_info().max_media_bytes()})};
    auto writer{common::IMediaFrameWriterFactory::getFactory()->createFrameWriter(m_shmHandle.getShm(), shmInfo)};

    // Write frames to shm and add gst expects
    for (unsigned i = 0; i < dataCountToPush; ++i)
    {
        EXPECT_EQ(writer->writeFrame(segments[i]), AddSegmentStatus::OK);
        willPushVideoData(segments[i], buffers[i], copies[i], false);
    }

    // Finally, send HaveData and receive new NeedData
    ExpectMessage<firebolt::rialto::NeedMediaDataEvent> expectedNeedData{m_clientStub};
    auto haveDataReq{createHaveDataRequest(m_sessionId, writer->getNumFrames(), m_lastVideoNeedData->request_id())};
    ConfigureAction<HaveData>(m_clientStub).send(haveDataReq).expectSuccess();
    auto receivedNeedData{expectedNeedData.getMessage()};
    ASSERT_TRUE(receivedNeedData);
    EXPECT_EQ(receivedNeedData->session_id(), m_sessionId);
    EXPECT_EQ(receivedNeedData->source_id(), m_videoSourceId);
    EXPECT_EQ(receivedNeedData->frame_count(), needDataFrameCount);
    m_lastVideoNeedData = receivedNeedData;
}

void MediaPipelineTest::pushAudioSample(int needDataFrameCount)
{
    // First, generate new data
    std::unique_ptr<IMediaPipeline::MediaSegment> segment{SegmentBuilder().basicAudioSegment(m_audioSourceId)()};
    GstBuffer buffer{};
    GstCaps capsCopy{};

    // Next, create frame writer
    ASSERT_TRUE(m_lastAudioNeedData);
    std::shared_ptr<MediaPlayerShmInfo> shmInfo{
        std::make_shared<MediaPlayerShmInfo>(MediaPlayerShmInfo{m_lastAudioNeedData->shm_info().max_metadata_bytes(),
                                                                m_lastAudioNeedData->shm_info().metadata_offset(),
                                                                m_lastAudioNeedData->shm_info().media_data_offset(),
                                                                m_lastAudioNeedData->shm_info().max_media_bytes()})};
    auto writer{common::IMediaFrameWriterFactory::getFactory()->createFrameWriter(m_shmHandle.getShm(), shmInfo)};

    // Write frames to shm and add gst expects
    EXPECT_EQ(writer->writeFrame(segment), AddSegmentStatus::OK);
    willPushAudioSample(segment, buffer, capsCopy);

    // Finally, send HaveData and receive new NeedData
    ExpectMessage<firebolt::rialto::NeedMediaDataEvent> expectedNeedData{m_clientStub};
    auto haveDataReq{createHaveDataRequest(m_sessionId, writer->getNumFrames(), m_lastAudioNeedData->request_id())};
    ConfigureAction<HaveData>(m_clientStub).send(haveDataReq).expectSuccess();
    auto receivedNeedData{expectedNeedData.getMessage()};
    ASSERT_TRUE(receivedNeedData);
    EXPECT_EQ(receivedNeedData->session_id(), m_sessionId);
    EXPECT_EQ(receivedNeedData->source_id(), m_audioSourceId);
    EXPECT_EQ(receivedNeedData->frame_count(), needDataFrameCount);
    m_lastAudioNeedData = receivedNeedData;
}

void MediaPipelineTest::pushVideoSample(int needDataFrameCount)
{
    // First, generate new data
    std::unique_ptr<IMediaPipeline::MediaSegment> segment{SegmentBuilder().basicVideoSegment(m_videoSourceId)()};
    GstBuffer buffer{};
    GstCaps capsCopy{};

    // Next, create frame writer
    ASSERT_TRUE(m_lastVideoNeedData);
    std::shared_ptr<MediaPlayerShmInfo> shmInfo{
        std::make_shared<MediaPlayerShmInfo>(MediaPlayerShmInfo{m_lastVideoNeedData->shm_info().max_metadata_bytes(),
                                                                m_lastVideoNeedData->shm_info().metadata_offset(),
                                                                m_lastVideoNeedData->shm_info().media_data_offset(),
                                                                m_lastVideoNeedData->shm_info().max_media_bytes()})};
    auto writer{common::IMediaFrameWriterFactory::getFactory()->createFrameWriter(m_shmHandle.getShm(), shmInfo)};

    // Write frames to shm and add gst expects
    EXPECT_EQ(writer->writeFrame(segment), AddSegmentStatus::OK);
    willPushVideoSample(segment, buffer, capsCopy);

    // Finally, send HaveData and receive new NeedData
    ExpectMessage<firebolt::rialto::NeedMediaDataEvent> expectedNeedData{m_clientStub};
    auto haveDataReq{createHaveDataRequest(m_sessionId, writer->getNumFrames(), m_lastVideoNeedData->request_id())};
    ConfigureAction<HaveData>(m_clientStub).send(haveDataReq).expectSuccess();
    auto receivedNeedData{expectedNeedData.getMessage()};
    ASSERT_TRUE(receivedNeedData);
    EXPECT_EQ(receivedNeedData->session_id(), m_sessionId);
    EXPECT_EQ(receivedNeedData->source_id(), m_videoSourceId);
    EXPECT_EQ(receivedNeedData->frame_count(), needDataFrameCount);
    m_lastVideoNeedData = receivedNeedData;
}

void MediaPipelineTest::eosAudio(unsigned dataCountToPush)
{
    // First, generate new data
    std::vector<std::unique_ptr<IMediaPipeline::MediaSegment>> segments(dataCountToPush);
    std::generate(segments.begin(), segments.end(),
                  [&]() { return SegmentBuilder().basicAudioSegment(m_audioSourceId)(); });
    std::vector<GstBuffer> buffers(dataCountToPush);
    std::vector<GstCaps> copies(dataCountToPush);

    // Next, create frame writer
    ASSERT_TRUE(m_lastAudioNeedData);
    std::shared_ptr<MediaPlayerShmInfo> shmInfo{
        std::make_shared<MediaPlayerShmInfo>(MediaPlayerShmInfo{m_lastAudioNeedData->shm_info().max_metadata_bytes(),
                                                                m_lastAudioNeedData->shm_info().metadata_offset(),
                                                                m_lastAudioNeedData->shm_info().media_data_offset(),
                                                                m_lastAudioNeedData->shm_info().max_media_bytes()})};
    auto writer{common::IMediaFrameWriterFactory::getFactory()->createFrameWriter(m_shmHandle.getShm(), shmInfo)};

    // Write frames to shm and add gst expects
    for (unsigned i = 0; i < dataCountToPush; ++i)
    {
        EXPECT_EQ(writer->writeFrame(segments[i]), AddSegmentStatus::OK);
        // On the last frame, trigger notification
        if (i < dataCountToPush - 1)
        {
            willPushAudioData(segments[i], buffers[i], copies[i], false);
        }
        else
        {
            willPushAudioData(segments[i], buffers[i], copies[i], true);
        }
    }

    // Finally, send HaveData with EOS status
    ExpectMessage<firebolt::rialto::NeedMediaDataEvent> expectedNeedData{m_clientStub};
    auto haveDataReq{createHaveDataRequest(m_sessionId, writer->getNumFrames(), m_lastAudioNeedData->request_id())};
    haveDataReq.set_status(HaveDataRequest_MediaSourceStatus_EOS);
    ConfigureAction<HaveData>(m_clientStub).send(haveDataReq).expectSuccess();
    waitWorker();
}

void MediaPipelineTest::eosVideo(unsigned dataCountToPush)
{
    // First, generate new data
    std::vector<std::unique_ptr<IMediaPipeline::MediaSegment>> segments(dataCountToPush);
    std::generate(segments.begin(), segments.end(),
                  [&]() { return SegmentBuilder().basicVideoSegment(m_videoSourceId)(); });
    std::vector<GstBuffer> buffers(dataCountToPush);
    std::vector<GstCaps> copies(dataCountToPush);

    // Next, create frame writer
    ASSERT_TRUE(m_lastVideoNeedData);
    std::shared_ptr<MediaPlayerShmInfo> shmInfo{
        std::make_shared<MediaPlayerShmInfo>(MediaPlayerShmInfo{m_lastVideoNeedData->shm_info().max_metadata_bytes(),
                                                                m_lastVideoNeedData->shm_info().metadata_offset(),
                                                                m_lastVideoNeedData->shm_info().media_data_offset(),
                                                                m_lastVideoNeedData->shm_info().max_media_bytes()})};
    auto writer{common::IMediaFrameWriterFactory::getFactory()->createFrameWriter(m_shmHandle.getShm(), shmInfo)};

    // Write frames to shm and add gst expects
    for (unsigned i = 0; i < dataCountToPush; ++i)
    {
        EXPECT_EQ(writer->writeFrame(segments[i]), AddSegmentStatus::OK);
        // On the last frame, trigger notification
        if (i < dataCountToPush - 1)
        {
            willPushVideoData(segments[i], buffers[i], copies[i], false);
        }
        else
        {
            willPushVideoData(segments[i], buffers[i], copies[i], true);
        }
    }

    // Finally, send HaveData with EOS status
    ExpectMessage<firebolt::rialto::NeedMediaDataEvent> expectedNeedData{m_clientStub};
    auto haveDataReq{createHaveDataRequest(m_sessionId, writer->getNumFrames(), m_lastVideoNeedData->request_id())};
    haveDataReq.set_status(HaveDataRequest_MediaSourceStatus_EOS);
    ConfigureAction<HaveData>(m_clientStub).send(haveDataReq).expectSuccess();
    waitWorker();
}

void MediaPipelineTest::play()
{
    auto playReq{createPlayRequest(m_sessionId)};
    ConfigureAction<Play>(m_clientStub).send(playReq).expectSuccess();

    ExpectMessage<firebolt::rialto::PlaybackStateChangeEvent> expectedPlaybackStateChange{m_clientStub};

    mayReceivePositionUpdates();

    m_gstreamerStub.sendStateChanged(GST_STATE_NULL, GST_STATE_PLAYING, GST_STATE_NULL);

    auto receivedPlaybackStateChange{expectedPlaybackStateChange.getMessage()};
    ASSERT_TRUE(receivedPlaybackStateChange);
    EXPECT_EQ(receivedPlaybackStateChange->session_id(), m_sessionId);
    EXPECT_EQ(receivedPlaybackStateChange->state(), ::firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_PLAYING);
}

void MediaPipelineTest::gstNotifyEos()
{
    ExpectMessage<firebolt::rialto::PlaybackStateChangeEvent> expectedPlaybackStateChange{m_clientStub};

    m_gstreamerStub.sendEos();

    auto receivedPlaybackStateChange{expectedPlaybackStateChange.getMessage()};
    ASSERT_TRUE(receivedPlaybackStateChange);
    EXPECT_EQ(receivedPlaybackStateChange->session_id(), m_sessionId);
    EXPECT_EQ(receivedPlaybackStateChange->state(),
              ::firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_END_OF_STREAM);
}

void MediaPipelineTest::removeSource(int sourceId)
{
    auto removeSourceReq{createRemoveSourceRequest(m_sessionId, sourceId)};
    ConfigureAction<RemoveSource>(m_clientStub).send(removeSourceReq).expectSuccess();

    // Sources other than audio do not do anything for RemoveSource
    if (m_audioSourceId == sourceId)
        waitWorker();
}

void MediaPipelineTest::stop()
{
    auto stopReq{createStopRequest(m_sessionId)};
    ConfigureAction<Stop>(m_clientStub).send(stopReq).expectSuccess();

    ExpectMessage<firebolt::rialto::PlaybackStateChangeEvent> expectedPlaybackStateChange{m_clientStub};

    m_gstreamerStub.sendStateChanged(GST_STATE_NULL, GST_STATE_NULL, GST_STATE_NULL);

    auto receivedPlaybackStateChange{expectedPlaybackStateChange.getMessage()};
    ASSERT_TRUE(receivedPlaybackStateChange);
    EXPECT_EQ(receivedPlaybackStateChange->session_id(), m_sessionId);
    EXPECT_EQ(receivedPlaybackStateChange->state(), ::firebolt::rialto::PlaybackStateChangeEvent_PlaybackState_STOPPED);

    positionUpdatesShouldNotBeReceivedFromNow();
}

void MediaPipelineTest::destroySession()
{
    auto destroySessionReq{createDestroySessionRequest(m_sessionId)};
    ConfigureAction<DestroySession>(m_clientStub).send(destroySessionReq).expectSuccess();
}

void MediaPipelineTest::initShm()
{
    auto getShmReq{createGetSharedMemoryRequest()};
    ConfigureAction<GetSharedMemory>(m_clientStub)
        .send(getShmReq)
        .expectSuccess()
        .matchResponse([&](const auto &resp) { m_shmHandle.init(resp.fd(), resp.size()); });
}

void MediaPipelineTest::mayReceivePositionUpdates()
{
    if (-1 == m_positionChangeEventSuppressionId)
    {
        m_positionChangeEventSuppressionId = m_clientStub.addSuppression<firebolt::rialto::PositionChangeEvent>();
    }
}

void MediaPipelineTest::positionUpdatesShouldNotBeReceivedFromNow()
{
    if (-1 != m_positionChangeEventSuppressionId)
    {
        m_clientStub.removeSuppression(m_positionChangeEventSuppressionId);
        m_positionChangeEventSuppressionId = -1;
    }
}

void MediaPipelineTest::waitWorker()
{
    std::unique_lock<std::mutex> locker(m_workerLock);
    if (!m_workerDone)
    {
        bool status = m_workerCond.wait_for(locker, kWorkerTimeout, [this]() { return m_workerDone; });
        ASSERT_TRUE(status);
    }
    m_workerDone = false;
}

void MediaPipelineTest::workerFinished()
{
    std::unique_lock<std::mutex> locker(m_workerLock);
    m_workerDone = true;
    m_workerCond.notify_all();
}
} // namespace firebolt::rialto::server::ct
