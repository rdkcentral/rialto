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

class SetPositionTest : public GenericTasksTestsBase
{
protected:
    SetPositionTest()
    {
        setContextStreamInfo(firebolt::rialto::MediaSourceType::AUDIO);
        setContextStreamInfo(firebolt::rialto::MediaSourceType::VIDEO);
        setContextNeedData(true);
        setContextNeedDataPending(true);
        setContextEndOfStreamNotified();
        setContextEndOfStream(firebolt::rialto::MediaSourceType::AUDIO);
        setContextEndOfStream(firebolt::rialto::MediaSourceType::VIDEO);
        setContextAudioBuffer();
        setContextVideoBuffer();
    }
};

TEST_F(SetPositionTest, shouldFailToSetPositionWhenClientIsNull)
{
    triggerSetPositionNullClient();
    checkNeedDataForBothSources();
    checkNeedDataPendingForBothSources();
    checkBuffersDoExist();
}

TEST_F(SetPositionTest, shouldFailToSetPositionWhenPipelineIsNull)
{
    setContextPipelineNull();
    shouldExtractBuffers();
    shouldNotifyFailure();
    triggerSetPosition();
    checkDoNotNeedDataForBothSources();
    checkNoNeedDataPendingForBothSources();
    checkBuffersEmpty();
}

TEST_F(SetPositionTest, shouldFailToSetPositionWhenSeekFailed)
{
    shouldExtractBuffers();
    shouldSeekFailure();
    triggerSetPosition();
    checkDoNotNeedDataForBothSources();
    checkNoNeedDataPendingForBothSources();
    checkBuffersEmpty();
}

TEST_F(SetPositionTest, shouldSetPosition)
{
    shouldExtractBuffers();
    shouldSeekSuccess();
    triggerSetPosition();
    checkNeedDataForBothSources();
    checkNeedDataPendingForBothSources();
    checkBuffersEmpty();
}

TEST_F(SetPositionTest, shouldSetPositionWithChangedPlaybackRate)
{
    setContextPlaybackRate();
    shouldExtractBuffers();
    shouldSeekSuccess();
    triggerSetPosition();
    checkNeedDataForBothSources();
    checkNeedDataPendingForBothSources();
    checkBuffersEmpty();
    checkNoEos();
}
