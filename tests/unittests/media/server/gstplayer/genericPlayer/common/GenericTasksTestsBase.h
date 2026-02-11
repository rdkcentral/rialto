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

#ifndef GENERIC_TASKS_TESTS_BASE_H_
#define GENERIC_TASKS_TESTS_BASE_H_

#include "MediaCommon.h"

#include <gmock/gmock.h>
#include <gst/gst.h>
#include <gtest/gtest.h>

#include <string>

using ::testing::_;
using ::testing::A;
using ::testing::ByMove;
using ::testing::DoAll;
using ::testing::ElementsAreArray;
using ::testing::Invoke;
using ::testing::Ne;
using ::testing::Ref;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::SaveArg;
using ::testing::SetArgPointee;
using ::testing::StrEq;
using ::testing::StrictMock;

/**
 * @brief GenericTasksTest Base class
 *
 * This class exists to create a common place for all gstreamer objects and mocks to coexist.
 * Moving all gstreamer dependancies into one file reduces the compile time dramatically.
 */
class GenericTasksTestsBase : public ::testing::Test
{
public:
    GenericTasksTestsBase();
    virtual ~GenericTasksTestsBase();

protected:
    // Set context methods
    void setContextStreamInfo(firebolt::rialto::MediaSourceType sourceType);
    void setContextPlaying();
    void setContextNeedData(bool doNeedData);
    void setContextAudioUnderflowOccured(bool isUnderflow);
    void setContextVideoUnderflowOccured(bool isUnderflow);
    void setContextEndOfStream(firebolt::rialto::MediaSourceType sourceType, bool state = true);
    void setContextEndOfStreamNotified();
    void setContextPipelineNull();
    void setContextNeedDataPending(bool isNeedDataPending);
    void setContextNeedDataPendingAudioOnly(bool isNeedDataPending);
    void setContextNeedDataPendingVideoOnly(bool isNeedDataPending);
    void setContextAudioBuffer();
    void setContextVideoBuffer();
    void setContextPlaybackRate();
    void setContextSourceNull();
    void setContextStreamInfoEmpty();
    void setContextNeedDataAudioOnly();
    void setContextSetupSourceFinished();

    // SetupElement test methods
    void shouldSetupVideoSinkElementOnly();
    void shouldSetupVideoDecoderElementOnly();
    void shouldSetupVideoElementWithPendingGeometry();
    void shouldSetupVideoElementWithPendingImmediateOutput();
    void shouldSetupAudioSinkElementWithPendingLowLatency();
    void shouldSetupAudioSinkElementWithPendingSync();
    void shouldSetupAudioDecoderElementWithPendingSyncOff();
    void shouldSetupAudioDecoderElementWithPendingStreamSyncMode();
    void shouldSetupVideoParserElementWithPendingStreamSyncMode();
    void shouldSetupAudioDecoderElementWithPendingBufferingLimit();
    void shouldSetupVideoSinkElementWithPendingRenderFrame();
    void shouldSetupVideoSinkElementWithPendingShowVideoWindow();
    void shouldSetupAudioElementAmlhalasinkWhenNoVideo();
    void shouldSetupAudioElementAmlhalasinkWhenVideoExists();
    void shouldSetupAudioElementBrcmAudioSink();
    void shouldSetupVideoElementAutoVideoSink();
    void shouldSetupAudioElementAutoAudioSink();
    void shouldSetupVideoElementAutoVideoSinkWithMultipleChildren();
    void shouldSetupAudioElementAutoAudioSinkWithMultipleChildren();
    void shouldSetupAudioSinkElementOnly();
    void shouldSetupAudioDecoderElementOnly();
    void shouldSetVideoUnderflowCallback();
    void shouldSetupBaseParse();
    void triggerSetupElement();
    void triggerVideoUnderflowCallback();
    void shouldSetAudioUnderflowCallback();
    void triggerAudioUnderflowCallback();
    void shouldAddFirstAutoVideoSinkChild();
    void shouldAddFirstAutoAudioSinkChild();
    void shouldNotAddAutoVideoSinkChild();
    void shouldNotAddAutoAudioSinkChild();
    void shouldAddAutoVideoSinkChildCallback();
    void shouldAddAutoAudioSinkChildCallback();
    void triggerAutoVideoSinkChildAddedCallback();
    void triggerAutoAudioSinkChildAddedCallback();
    void shouldRemoveAutoVideoSinkChildCallback();
    void shouldRemoveAutoAudioSinkChildCallback();
    void triggerAutoVideoSinkChildRemovedCallback();
    void triggerAutoAudioSinkChildRemovedCallback();
    void shouldSetupTextTrackSink();
    void shouldSetupVideoDecoderForTextTrack();
    void shouldSetupVideoDecoderForTextTrackWesterosSinkWithDecoder();
    void shouldSetupVideoDecoderForTextTrackWesterosSinkWithoutDecoder();

    // SetVideoGeometry test methods
    void setPipelineToNull();
    void triggerSetVideoGeometryFailure();
    void shouldSetVideoGeometry();
    void triggerSetVideoGeometrySuccess();

    // SetupSource test methods
    void setAllSourcesAttached();
    void shouldScheduleAllSourcesAttached();
    void triggerSetupSource();

    // SetVolume test methods
    void shouldSetGstVolume();
    void triggerSetVolume();
    void triggerSetVolumeEaseTypeLinear();
    void triggerSetVolumeEaseTypeCubicIn();
    void triggerSetVolumeEaseTypeCubicOut();
    void shouldSetAudioFadeAndEaseTypeLinear();
    void shouldSetAudioFadeAndEaseTypeCubicIn();
    void shouldSetAudioFadeAndEaseTypeCubicOut();
    void shouldSetAudioFadeInSocWithLinearEaseType();
    void shouldSetAudioFadeInSocWithCubicInEaseType();
    void shouldSetAudioFadeInSocWithCubicOutEaseType();

    // AttachSamples test methods
    void shouldAttachAllAudioSamples();
    void shouldAttachData(firebolt::rialto::MediaSourceType sourceType);
    void triggerAttachSamplesAudio();
    void shouldAttachAllVideoSamples();
    void triggerAttachSamplesVideo();
    void shouldAttachAllSubtitleSamples();
    void shouldSkipAttachingSubtitleSamples();
    void triggerAttachSamplesSubtitle();
    void checkSubtitleSamplesAttached();

    // AttachSource test methods
    void shouldAttachAudioSource();
    void triggerAttachAudioSource();
    void checkAudioSourceAttached();
    void shouldAttachAudioSourceWithChannelsAndRate();
    void triggerAttachAudioSourceWithChannelsAndRateAndDrm();
    void checkAudioSourceAttachedWithDrm();
    void shouldAttachAudioSourceWithAudioSpecificConf();
    void triggerAttachOpusAudioSourceWithAudioSpecificConf();
    void shouldAttachBwavAudioSource();
    void triggerAttachBwavAudioSource();
    void shouldAttachXrawAudioSource();
    void triggerAttachXrawAudioSource();
    void shouldAttachFlacAudioSource();
    void triggerAttachFlacAudioSource();
    void shouldAttachMp3AudioSource();
    void triggerAttachMp3AudioSource();
    void shouldAttachVideoSource(const std::string &mime, const std::string &alignment, const std::string &format);
    void triggerAttachVideoSource(const std::string &mimeType, firebolt::rialto::SegmentAlignment segmentAligment,
                                  firebolt::rialto::StreamFormat streamFormat);
    void triggerAttachUnknownSource();
    void checkVideoSourceAttached();
    void shouldAttachSubtitleSource();
    void checkSubtitleSourceAttached();
    void triggerAttachSubtitleSource();
    void shouldAttachVideoSourceWithStringCodecData();
    void triggerAttachVideoSourceWithStringCodecData();
    void checkVideoSourceAttachedWithDrm();
    void shouldAttachVideoSourceWithEmptyCodecData();
    void triggerAttachVideoSourceWithEmptyCodecData();
    void shouldAttachVideoSourceWithDolbyVisionSource();
    void triggerAttachVideoSourceWithDolbyVisionSource();
    void shouldReattachAudioSource();
    void shouldEnableAudioFlagsAndSendNeedData();
    void shouldFailToReattachAudioSource();
    void triggerReattachAudioSource();
    void checkNewAudioSourceAttached();
    void triggerFailToCastAudioSource();
    void triggerFailToCastVideoSource();
    void triggerFailToCastDolbyVisionSource();

    // CheckAudioUnderflow test methods
    void shouldQueryPositionAndSetToZero();
    void shouldBeInWrongStateForQueryPosition();
    void triggerCheckAudioUnderflowNoNotification();
    void shouldNotifyAudioUnderflow();
    void triggerCheckAudioUnderflow();

    // DeepElementAdded test methods
    void shouldNotRegisterCallbackWhenPtrsAreNotEqual();
    void constructDeepElementAdded();
    void shouldNotRegisterCallbackWhenElementIsNull();
    void shouldNotRegisterCallbackWhenElementNameIsNotTypefind();
    void shouldRegisterCallbackForTypefindElement();
    void shouldUpdatePlaybackGroupWhenCallbackIsCalled();
    void shouldSetTypefindElement();
    void triggerDeepElementAdded();
    void checkTypefindPlaybackGroupAdded();
    void checkPipelinePlaybackGroupAdded();
    void shouldSetParseElement();
    void checkParsePlaybackGroupAdded();
    void shouldSetDecoderElement();
    void checkDecoderPlaybackGroupAdded();
    void shouldSetGenericElement();
    void shouldSetAudioSinkElement();
    void shouldHaveNullParentSink();
    void shouldHaveNonBinParentSink();
    void shouldHaveBinParentSink();
    void checkAudioSinkPlaybackGroupAdded();

    // UpdatePlaybackGroup test methods
    void triggerUpdatePlaybackGroupNoCaps();
    void checkNoPlaybackGroupAdded();
    void shouldReturnNullCaps();
    void triggerUpdatePlaybackGroup();
    void shouldDoNothingForVideoCaps();
    void shouldDoNothingWhenTypefindParentIsNull();
    void shouldDoNothingWhenElementOtherThanDecodebin();
    void shouldSuccessfullyFindTypefindAndParent();
    void checkPlaybackGroupAdded();
    void setUseBufferingPending();
    void shouldTriggerSetUseBuffering();

    // Stop test methods
    void shouldStopGstPlayer();
    void triggerStop();
    void checkNoMoreNeedData();

    // EnoughData test methods
    void triggerEnoughDataAudio();
    void triggerEnoughDataVideo();
    void checkNeedDataForBothSources();
    void checkNeedDataForAudioOnly();
    void checkNeedDataForVideoOnly();
    void setStreamIsDataPushed(firebolt::rialto::MediaSourceType sourceType);

    // Eos test methods
    void triggerEosAudio();
    void triggerEosVideo();
    void shouldGstAppSrcEndOfStreamSuccess();
    void shouldGstAppSrcEndOfStreamFailure();
    void shouldCancelUnderflow(firebolt::rialto::MediaSourceType sourceType);
    void shouldSetEos(firebolt::rialto::MediaSourceType sourceType);
    void shouldSetEosPending(firebolt::rialto::MediaSourceType sourceType);

    // Underflow test methods
    void setUnderflowEnabled(bool isUnderflowEnabled);
    void triggerVideoUnderflow();
    void shouldNotifyVideoUnderflow();

    // Shutdown test methods
    void shouldStopWorkerThread();
    void triggerShutdown();

    // SetMute test methods
    void triggerSetAudioMute();
    void triggerSetVideoMute();
    void triggerSetSubtitleMute();
    void triggerSetUnknownMute();
    void setContextSubtitleSink();
    void shouldSetAudioMute();
    void shouldSetVideoMute();
    void shouldSetSubtitleMute();

    // immediate-output sink property test methods
    void shouldSetImmediateOutput();
    void triggerSetImmediateOutput();

    // low-latency sink property test methods
    void shouldSetLowLatency();
    void triggerSetLowLatency();

    // sync sink property test methods
    void shouldSetSync();
    void triggerSetSync();

    // sync-off decoder property test methods
    void shouldSetSyncOff();
    void triggerSetSyncOff();

    // stream-sync-mode decoder property test methods
    void shouldSetStreamSyncMode();
    void triggerSetStreamSyncMode();

    // buffering limit property test methods
    void shouldSetBufferingLimit();
    void triggerSetBufferingLimit();

    // use buffering property test methods
    void shouldSetUseBuffering();
    void triggerSetUseBuffering();

    // SetPosition test methods
    void triggerSetPositionNullClient();
    void triggerSetPosition();
    void checkNeedDataPendingForBothSources();
    void checkBuffersDoExist();
    void checkDoNotNeedDataForBothSources();
    void checkNoNeedDataPendingForBothSources();
    void checkBuffersEmpty();
    void shouldExtractBuffers();
    void shouldNotifyFailure();
    void shouldSeekFailure();
    void shouldSeekSuccess();
    void checkNoEos();

    // Play test methods
    void shouldChangeStatePlayingSuccess();
    void shouldChangeStatePlayingFailure();
    void triggerPlay();

    // Ping test methods
    void triggerPing();

    // Pause test methods
    void shouldPause();
    void triggerPause();
    void checkContextPaused();

    // ReportPosition test methods
    void shouldReportPosition();
    void triggerReportPosition();
    void shouldFailToReportPosition();

    // FinishSetupSource test methods
    void shouldFinishSetupSource();
    void triggerFinishSetupSource();
    void shouldScheduleNeedMediaDataAudio();
    void triggerAudioCallbackNeedData();
    void shouldScheduleNeedMediaDataVideo();
    void triggerVideoCallbackNeedData();
    void shouldScheduleEnoughDataAudio();
    void triggerAudioCallbackEnoughData();
    void shouldScheduleEnoughDataVideo();
    void triggerVideoCallbackEnoughData();
    void triggerAudioCallbackSeekData();
    void triggerVideoCallbackSeekData();
    void checkSourcesAttached();
    void checkSetupSourceFinished();
    void checkSetupSourceUnfinished();

    // NeedData test methods
    void triggerNeedDataAudio();
    void triggerNeedDataVideo();
    void triggerNeedDataUnknownSrc();
    void shouldNotifyNeedAudioDataSuccess();
    void shouldNotifyNeedVideoDataSuccess();
    void checkNeedDataPendingForAudioOnly();
    void checkNeedDataPendingForVideoOnly();
    void shouldNotifyNeedAudioDataFailure();
    void shouldNotifyNeedVideoDataFailure();

    // SetPlaybackRate test methods
    void triggerSetPlaybackRate();
    void checkNoPendingPlaybackRate();
    void checkPendingPlaybackRate();
    void checkPlaybackRateSet();
    void checkPlaybackRateDefault();
    void setPipelinePlaying();
    void shouldSetPlaybackRateAudioSinkNullSuccess();
    void shouldSetPlaybackRateAudioSinkNullFailure();
    void shouldSetPlaybackRateAudioSinkOtherThanAmlhala();
    void shouldFailToSetPlaybackRateAudioSinkOtherThanAmlhala();
    void shouldSetPlaybackRateAmlhalaAudioSink();
    void shouldFailToSetPlaybackRateAmlhalaAudioSink();
    void checkSegmentInfo();

    // RenderFrame test methods
    void shouldRenderFrame();
    void triggerRenderFrame();
    void shouldFlushAudioSrcSuccess();
    void shouldFlushAudioSrcFailure();

    // ReadShmDataAndAttachSamples test methods
    void shouldReadAudioData();
    void shouldReadVideoData();
    void shouldReadSubtitleData();
    void shouldReadUnknownData();
    void shouldNotAttachUnknownSamples();
    void triggerReadShmDataAndAttachSamplesAudio();
    void triggerReadShmDataAndAttachSamplesVideo();
    void triggerReadShmDataAndAttachSamples();

    // Flush test methods
    void shouldFlushAudio();
    void shouldFlushVideo();
    void triggerFlush(firebolt::rialto::MediaSourceType sourceType);
    void checkAudioFlushed();
    void checkVideoFlushed();
    void shouldFlushVideoSrcSuccess();
    void shouldPostponeVideoFlush();

    // Set Source Position test methods
    void shouldSetSubtitleSourcePosition();

    void triggerSetSourcePosition(firebolt::rialto::MediaSourceType sourceType);
    void checkInitialPositionSet(firebolt::rialto::MediaSourceType sourceType);
    void checkInitialPositionNotSet(firebolt::rialto::MediaSourceType sourceType);

    // Set Subtitle Offset test methods
    void shouldSetSubtitleOffset();
    void triggerSetSubtitleOffset();

    // ProcessAudioGap test methods
    void triggerProcessAudioGap();
    void shouldProcessAudioGap();

    // SetTextTrackIdentifier test methods
    void shouldSetTextTrackIdentifier();
    void triggerSetTextTrackIdentifier();

    // SwitchSource test methods
    void triggerSwitchMpegSource();

private:
    // SetupElement helper methods
    void expectVideoUnderflowSignalConnection();
    void expectAudioUnderflowSignalConnection();
    void expectSetupVideoSinkElement();
    void expectSetupVideoDecoderElement();
    void expectSetupAudioSinkElement();
    void expectSetupAudioDecoderElement();
    void expectSetupVideoParserElement();
    void expectSetupBaseParseElement();

    // AttachSource helper methods
    void expectSetGenericVideoCaps();
    void expectSetChannelAndRateAudioCaps();
    void expectAddChannelAndRateAudioToCaps();
    void expectAddRawAudioDataToCaps();
    void expectAddStreamHeaderToCaps();
    void expectAddFramedToCaps();
    void expectSetCaps();

    // Set property helpers
    template <typename T> void expectSetProperty(const std::string &propertyName, const T &value);
    void expectPropertyDoesntExist(const std::string &propertyName);
    std::string getFadeString(double targetVolume, uint32_t volumeDuration, firebolt::rialto::EaseType easeType);
};

#endif // GENERIC_TASKS_TESTS_BASE_H_
