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

class RemoveSourceTest : public GenericTasksTestsBase
{
protected:
    RemoveSourceTest()
    {
        setContextStreamInfo(firebolt::rialto::MediaSourceType::AUDIO);
        setContextStreamInfo(firebolt::rialto::MediaSourceType::VIDEO);
        setContextNeedDataAudioOnly();
        setContextAudioBuffer();
        setContextNeedDataPendingAudioOnly(true);
    }
};

TEST_F(RemoveSourceTest, shouldRemoveAudioSourceWithoutFlushing)
{
    setContextStreamInfoEmpty();
    shouldInvalidateActiveAudioRequests();
    triggerRemoveSourceAudio();
    checkAudioSourceRemoved();
}

TEST_F(RemoveSourceTest, shouldNotRemoveVideoSource)
{
    triggerRemoveSourceVideo();
    checkNeedDataForAudioOnly();
    checkNeedDataPendingForAudioOnly();
    checkAudioSourceNotRemoved();
}

TEST_F(RemoveSourceTest, shouldRemoveAudioSource)
{
    shouldInvalidateActiveAudioRequests();
    shouldDisableAudioFlag();
    shouldClearAudioNeedDataFlag();
    shouldFlushAudioSrcSuccess();
    triggerRemoveSourceAudio();
    checkNoMoreNeedData();
    checkNoNeedDataPendingForBothSources();
    checkAudioSourceRemoved();
    checkBuffersEmpty();
}

TEST_F(RemoveSourceTest, shouldRemoveAudioSourceFlushEventError)
{
    shouldInvalidateActiveAudioRequests();
    shouldDisableAudioFlag();
    shouldClearAudioNeedDataFlag();
    shouldFlushAudioSrcFailure();
    triggerRemoveSourceAudio();
    checkNoMoreNeedData();
    checkNoNeedDataPendingForBothSources();
    checkAudioSourceRemoved();
}
