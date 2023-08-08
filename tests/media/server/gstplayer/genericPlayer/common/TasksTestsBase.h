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

#ifndef TASKS_TESTS_BASE_H_
#define TASKS_TESTS_BASE_H_

#include "MediaCommon.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::_;
using ::testing::A;
using ::testing::ByMove;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Ref;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::SaveArg;
using ::testing::StrictMock;
using ::testing::ElementsAreArray;
using ::testing::StrEq;

/**
 * @brief TasksTest Base class
 *
 * This class exists to create a common place for all gstreamer objects and mocks to coexist.
 * Moving all gstreamer dependancies into one file reduces the compile time dramatically.
 */
class TasksTestsBase : public ::testing::Test
{
public:
    TasksTestsBase();
    virtual ~TasksTestsBase();

protected:
    // Set context methods
    void setContextStreamInfo(firebolt::rialto::MediaSourceType sourceType);
    void setContextPlaying();

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

private:
    // SetupElement helper methods
    void expectSetupVideoElement();
    void expectSetupAudioElement();

    // AttachSource helper methods
    void expectSetGenericVideoCaps();
};

#endif // TASKS_TESTS_BASE_H_
