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

#include "GenericTasksTestsBase.h"

class SetupElementTest : public GenericTasksTestsBase
{
};

TEST_F(SetupElementTest, shouldSetupVideoElement)
{
    shouldSetupVideoSinkElementOnly();
    triggerSetupElement();
}

TEST_F(SetupElementTest, shouldSetupVideoElementWithPendingGeometry)
{
    shouldSetupVideoElementWithPendingGeometry();
    triggerSetupElement();
}

TEST_F(SetupElementTest, shouldSetupVideoElementWithPendingImmediateOutput)
{
    shouldSetupVideoElementWithPendingImmediateOutput();
    triggerSetupElement();
}

TEST_F(SetupElementTest, shouldSetupAudioElementWithPendingLowLatency)
{
    shouldSetupAudioSinkElementWithPendingLowLatency();
    triggerSetupElement();
}

TEST_F(SetupElementTest, shouldSetupAudioElementWithPendingSync)
{
    shouldSetupAudioSinkElementWithPendingSync();
    triggerSetupElement();
}

TEST_F(SetupElementTest, shouldSetupAudioElementWithPendingSyncOff)
{
    shouldSetupAudioDecoderElementWithPendingSyncOff();
    triggerSetupElement();
}

TEST_F(SetupElementTest, shouldSetupAudioElementWithPendingStreamSyncMode)
{
    shouldSetupAudioDecoderElementWithPendingStreamSyncMode();
    triggerSetupElement();
}

TEST_F(SetupElementTest, shouldSetupVideoElementWithPendingStreamSyncMode)
{
    shouldSetupVideoParserElementWithPendingStreamSyncMode();
    triggerSetupElement();
}

TEST_F(SetupElementTest, shouldSetupAudioElementWithPendingBufferingLimit)
{
    shouldSetupAudioDecoderElementWithPendingBufferingLimit();
    triggerSetupElement();
}

TEST_F(SetupElementTest, shouldSetupVideoElementWithPendingRenderFrame)
{
    shouldSetupVideoSinkElementWithPendingRenderFrame();
    triggerSetupElement();
}

TEST_F(SetupElementTest, shouldSetupAudioElementAmlhalasinkWhenNoVideo)
{
    shouldSetupAudioElementAmlhalasinkWhenNoVideo();
    triggerSetupElement();
}

TEST_F(SetupElementTest, shouldSetupAudioElementAmlhalasinkWhenVideoExists)
{
    shouldSetupAudioElementAmlhalasinkWhenVideoExists();
    triggerSetupElement();
}

TEST_F(SetupElementTest, shouldSetupAudioElementForBrcmAudioSink)
{
    shouldSetupAudioElementBrcmAudioSink();
    triggerSetupElement();
}

TEST_F(SetupElementTest, shouldSetupVideoElementWithChildSinkForAutoVideoSink)
{
    shouldSetupVideoElementAutoVideoSink();
    shouldAddFirstAutoVideoSinkChild();
    triggerSetupElement();
}

TEST_F(SetupElementTest, shouldSetupAudioElementWithChildSinkForAutoAudioSink)
{
    shouldSetupAudioElementAutoAudioSink();
    shouldAddFirstAutoAudioSinkChild();
    triggerSetupElement();
}

TEST_F(SetupElementTest, shouldSetupVideoElementWithoutChildSinkForAutoVideoSink)
{
    shouldSetupVideoElementAutoVideoSink();
    shouldNotAddAutoVideoSinkChild();
    triggerSetupElement();
}

TEST_F(SetupElementTest, shouldSetupAudioElementWithoutChildSinkForAutoAudioSink)
{
    shouldSetupAudioElementAutoAudioSink();
    shouldNotAddAutoAudioSinkChild();
    triggerSetupElement();
}

TEST_F(SetupElementTest, shouldSetupVideoElementWithMultpileChildSinkForAutoVideoSink)
{
    shouldSetupVideoElementAutoVideoSinkWithMultipleChildren();
    shouldAddFirstAutoVideoSinkChild();
    triggerSetupElement();
}

TEST_F(SetupElementTest, shouldSetupAudioElementWithMultpileChildSinkForAutoAudioSink)
{
    shouldSetupAudioElementAutoAudioSinkWithMultipleChildren();
    shouldAddFirstAutoAudioSinkChild();
    triggerSetupElement();
}

TEST_F(SetupElementTest, shouldSetupAudioElement)
{
    shouldSetupAudioSinkElementOnly();
    triggerSetupElement();
}

TEST_F(SetupElementTest, shouldReportVideoUnderflow)
{
    shouldSetupVideoDecoderElementOnly();
    triggerSetupElement();

    shouldSetVideoUnderflowCallback();
    triggerVideoUnderflowCallback();
}

TEST_F(SetupElementTest, shouldReportAudioUnderflow)
{
    shouldSetupAudioDecoderElementOnly();
    triggerSetupElement();

    shouldSetAudioUnderflowCallback();
    triggerAudioUnderflowCallback();
}

TEST_F(SetupElementTest, shouldReportAutoVideoSinkChildAdded)
{
    shouldSetupVideoElementAutoVideoSink();
    shouldNotAddAutoVideoSinkChild();
    triggerSetupElement();

    shouldAddAutoVideoSinkChildCallback();
    triggerAutoVideoSinkChildAddedCallback();
}

TEST_F(SetupElementTest, shouldReportAutoAudioSinkChildAdded)
{
    shouldSetupAudioElementAutoAudioSink();
    shouldNotAddAutoAudioSinkChild();
    triggerSetupElement();

    shouldAddAutoAudioSinkChildCallback();
    triggerAutoAudioSinkChildAddedCallback();
}

TEST_F(SetupElementTest, shouldReportAutoVideoSinkChildRemoved)
{
    shouldSetupVideoElementAutoVideoSink();
    shouldNotAddAutoVideoSinkChild();
    triggerSetupElement();

    shouldRemoveAutoVideoSinkChildCallback();
    triggerAutoVideoSinkChildRemovedCallback();
}

TEST_F(SetupElementTest, shouldReportAutoAudioSinkChildRemoved)
{
    shouldSetupAudioElementAutoAudioSink();
    shouldNotAddAutoAudioSinkChild();
    triggerSetupElement();

    shouldRemoveAutoAudioSinkChildCallback();
    triggerAutoAudioSinkChildRemovedCallback();
}

TEST_F(SetupElementTest, shouldSetupTextTrackSink)
{
    shouldSetupTextTrackSink();
    triggerSetupElement();
}

TEST_F(SetupElementTest, shouldSetupBaseParse)
{
    shouldSetupBaseParse();
    triggerSetupElement();
}