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

class DeepElementAddedTest : public GenericTasksTestsBase
{
};

TEST_F(DeepElementAddedTest, shouldNotRegisterCallbackWhenPtrsAreNotEqual)
{
    shouldNotRegisterCallbackWhenPtrsAreNotEqual();
    constructDeepElementAdded();
}

TEST_F(DeepElementAddedTest, shouldNotRegisterCallbackWhenElementIsNull)
{
    shouldNotRegisterCallbackWhenElementIsNull();
    constructDeepElementAdded();
}
TEST_F(DeepElementAddedTest, shouldNotRegisterCallbackWhenElementNameIsNotTypefind)
{
    shouldNotRegisterCallbackWhenElementNameIsNotTypefind();
    constructDeepElementAdded();
}

TEST_F(DeepElementAddedTest, shouldRegisterCallbackForTypefindElement)
{
    shouldRegisterCallbackForTypefindElement();
    constructDeepElementAdded();
}

TEST_F(DeepElementAddedTest, shouldUpdatePlaybackGroupWhenCallbackIsCalled)
{
    shouldUpdatePlaybackGroupWhenCallbackIsCalled();
    constructDeepElementAdded();
}

TEST_F(DeepElementAddedTest, shouldAddSignalIdOfRegisteredCallbackToPlayerContext)
{
    shouldRegisterCallbackForTypefindElement();
    shouldSetTypefindElement();
    triggerDeepElementAdded();
    checkTypefindPlaybackGroupAdded();
}

TEST_F(DeepElementAddedTest, shouldAssignPipelineOnlyWhenElementNameIsNull)
{
    shouldNotRegisterCallbackWhenPtrsAreNotEqual();
    triggerDeepElementAdded();
    checkPipelinePlaybackGroupAdded();
}

TEST_F(DeepElementAddedTest, shouldDetectParseElement)
{
    shouldSetParseElement();
    triggerDeepElementAdded();
    checkParsePlaybackGroupAdded();
}

TEST_F(DeepElementAddedTest, shouldDetectDecElement)
{
    shouldSetDecoderElement();
    triggerDeepElementAdded();
    checkDecoderPlaybackGroupAdded();
}

TEST_F(DeepElementAddedTest, shouldDoNothingForNotHandledElementName)
{
    shouldNotRegisterCallbackWhenElementNameIsNotTypefind();
    shouldSetGenericElement();
    triggerDeepElementAdded();
    checkPipelinePlaybackGroupAdded();
}

TEST_F(DeepElementAddedTest, shouldDoNothingWhenAudiosinkParentNameIsNull)
{
    shouldSetAudioSinkElement();
    shouldHaveNullParentSink();
    triggerDeepElementAdded();
    checkPipelinePlaybackGroupAdded();
}

TEST_F(DeepElementAddedTest, shouldDoNothingWhenAudiosinkParentIsNotBin)
{
    shouldSetAudioSinkElement();
    shouldHaveNonBinParentSink();
    triggerDeepElementAdded();
    checkPipelinePlaybackGroupAdded();
}

TEST_F(DeepElementAddedTest, shouldFindAudioSinkBin)
{
    shouldSetAudioSinkElement();
    shouldHaveBinParentSink();
    triggerDeepElementAdded();
    checkAudioSinkPlaybackGroupAdded();
}
