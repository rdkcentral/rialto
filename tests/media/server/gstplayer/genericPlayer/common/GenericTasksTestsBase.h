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
#include <gtest/gtest.h>

using ::testing::_;
using ::testing::A;
using ::testing::ByMove;
using ::testing::DoAll;
using ::testing::ElementsAreArray;
using ::testing::Invoke;
using ::testing::Ref;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::SaveArg;
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
    void setContextAudioAppSrc();
    void setContextEndOfStream(firebolt::rialto::MediaSourceType sourceType);
    void setContextEndOfStreamNotified();
    void setContextPipelineNull();
    void setContextNeedDataPending(bool isNeedDataPending);
    void setContextNeedDataPendingAudioOnly(bool isNeedDataPending);
    void setContextNeedDataPendingVideoOnly(bool isNeedDataPending);
    void setContextAudioBuffer();
    void setContextVideoBuffer();
    void setContextPlaybackRate();
    void setContextSourceNull();
    void setContextAudioSourceRemoved();
    void setContextStreamInfoEmpty();
    void setContextNeedDataAudioOnly();

    // SetupElement test methods
    void shouldSetupVideoElementOnly();
    void shouldSetupVideoElementWesterossink();
    void shouldSetupVideoElementAmlhalasink();
    void shouldSetupVideoElementPendingGeometryNonWesterissink();
    void shouldSetupAudioElementOnly();
    void shouldSetVideoUnderflowCallback();
    void triggerSetupElement();
    void triggerVideoUnderflowCallback();
    void shouldSetAudioUnderflowCallback();
    void triggerAudioUnderflowCallback();

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

    // AttachSamples test methods
    void shouldAttachAllAudioSamples();
    void triggerAttachSamplesAudio();
    void shouldAttachAllVideoSamples();
    void triggerAttachSamplesVideo();

    // AttachSource test methods
    void shouldAttachAudioSource();
    void triggerAttachAudioSource();
    void checkAudioSourceAttached();
    void shouldAttachAudioSourceWithChannelsAndRate();
    void triggerAttachAudioSourceWithChannelsAndRateAndDrm();
    void checkAudioSourceAttachedWithDrm();
    void shouldAttachAudioSourceWithAudioSpecificConf();
    void triggerAttachOpusAudioSourceWithAudioSpecificConf();
    void shouldAttachVideoSource();
    void triggerAttachVideoSource();
    void checkVideoSourceAttached();
    void shouldAttachVideoSourceWithStringCodecData();
    void triggerAttachVideoSourceWithStringCodecData();
    void checkVideoSourceAttachedWithDrm();
    void shouldAttachVideoSourceWithEmptyCodecData();
    void triggerAttachVideoSourceWithEmptyCodecData();
    void shouldAttachVideoSourceWithDolbyVisionSource();
    void triggerAttachVideoSourceWithDolbyVisionSource();
    void shouldSwitchAudioSource();
    void triggerSwitchAudioSource();
    void checkNewAudioSourceAttached();
    void shouldNotSwitchAudioSourceWhenMimeTypeIsEmpty();
    void triggerSwitchAudioSourceWithEmptyMimeType();

    // CheckAudioUnderflow test methods
    void shouldQueryPositionAndSetToZero();
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

    // Eos test methods
    void triggerEosAudio();
    void triggerEosVideo();
    void shouldGstAppSrcEndOfStreamSuccess();
    void shouldGstAppSrcEndOfStreamFailure();
    void shouldCancelUnderflow();

    // Underflow test methods
    void setUnderflowFlag(bool isUnderflowFlag);
    void setUnderflowEnabled(bool isUnderflowEnabled);
    void triggerVideoUnderflow();
    void checkUnderflowFlag(bool expected);
    void shouldNotifyVideoUnderflow();

    // Shutdown test methods
    void shouldStopWorkerThread();
    void triggerShutdown();

    // SetMute test methods
    void triggerSetMute();
    void shouldGstSetMute();

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
    void shouldGetVideoSinkFailure();
    void shouldFindPropertyFailure();
    void shouldInvalidateActiveAudioRequests();
    void triggerRemoveSourceAudio();
    void triggerRemoveSourceVideo();
    void checkAudioSourceRemoved();
    void checkAudioSourceNotRemoved();
    void shouldFlushAudioSrcSuccess();
    void shouldFlushAudioSrcFailure();

    // ReadShmDataAndAttachSamples test methods
    void shouldReadAudioData();
    void shouldReadVideoData();
    void triggerReadShmDataAndAttachSamplesAudio();
    void triggerReadShmDataAndAttachSamplesVideo();

private:
    // SetupElement helper methods
    void expectSetupVideoElement();
    void expectSetupAudioElement();

    // AttachSource helper methods
    void expectSetGenericVideoCaps();
};

#endif // GENERIC_TASKS_TESTS_BASE_H_
