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

class NeedDataTest : public GenericTasksTestsBase
{
};

TEST_F(NeedDataTest, shouldDoNothingWhenAudioAppSourceIsNotPresent)
{
    triggerNeedDataAudio();
}

TEST_F(NeedDataTest, shouldDoNothingWhenVideoAppSourceIsNotPresent)
{
    triggerNeedDataVideo();
}

TEST_F(NeedDataTest, shouldDoNothingForUnknownAppSource)
{
    setContextStreamInfo(firebolt::rialto::MediaSourceType::AUDIO);
    setContextStreamInfo(firebolt::rialto::MediaSourceType::VIDEO);
    triggerNeedDataUnknownSrc();
}

TEST_F(NeedDataTest, shouldNotifyNeedAudioData)
{
    setContextStreamInfo(firebolt::rialto::MediaSourceType::AUDIO);
    setContextStreamInfo(firebolt::rialto::MediaSourceType::VIDEO);
    shouldNotifyNeedAudioDataSuccess();
    triggerNeedDataAudio();
    checkNeedDataForAudioOnly();
    checkNeedDataPendingForAudioOnly();
}

TEST_F(NeedDataTest, shouldFailToNotifyNeedAudioData)
{
    setContextStreamInfo(firebolt::rialto::MediaSourceType::AUDIO);
    setContextStreamInfo(firebolt::rialto::MediaSourceType::VIDEO);
    shouldNotifyNeedAudioDataFailure();
    triggerNeedDataAudio();
    checkNeedDataForAudioOnly();
    checkNoNeedDataPendingForBothSources();
}

TEST_F(NeedDataTest, shouldSkipToNotifyNeedAudioDataWhenAnotherOneIsPending)
{
    setContextStreamInfo(firebolt::rialto::MediaSourceType::AUDIO);
    setContextStreamInfo(firebolt::rialto::MediaSourceType::VIDEO);
    setContextNeedDataPendingAudioOnly(true);
    triggerNeedDataAudio();
    checkNeedDataForAudioOnly();
    checkNeedDataPendingForAudioOnly();
}

TEST_F(NeedDataTest, shouldNotifyNeedVideoData)
{
    setContextStreamInfo(firebolt::rialto::MediaSourceType::AUDIO);
    setContextStreamInfo(firebolt::rialto::MediaSourceType::VIDEO);
    shouldNotifyNeedVideoDataSuccess();
    triggerNeedDataVideo();
    checkNeedDataPendingForVideoOnly();
    checkNeedDataForVideoOnly();
}

TEST_F(NeedDataTest, shouldFailToNotifyNeedVideoData)
{
    setContextStreamInfo(firebolt::rialto::MediaSourceType::AUDIO);
    setContextStreamInfo(firebolt::rialto::MediaSourceType::VIDEO);
    shouldNotifyNeedVideoDataFailure();
    triggerNeedDataVideo();
    checkNeedDataForVideoOnly();
    checkNoNeedDataPendingForBothSources();
}

TEST_F(NeedDataTest, shouldSkipToNotifyNeedVideoData)
{
    setContextStreamInfo(firebolt::rialto::MediaSourceType::AUDIO);
    setContextStreamInfo(firebolt::rialto::MediaSourceType::VIDEO);
    setContextNeedDataPendingVideoOnly(true);
    triggerNeedDataVideo();
    checkNeedDataPendingForVideoOnly();
    checkNeedDataForVideoOnly();
}

TEST_F(NeedDataTest, shouldAttachDataWhenBuffersBuffered)
{
    setContextStreamInfo(firebolt::rialto::MediaSourceType::AUDIO);
    setContextStreamInfo(firebolt::rialto::MediaSourceType::VIDEO);
    setContextVideoBuffer();
    setContextNeedDataPendingVideoOnly(true);
    shouldAttachData(firebolt::rialto::MediaSourceType::VIDEO);
    triggerNeedDataVideo();
    checkNeedDataPendingForVideoOnly();
    checkNeedDataForVideoOnly();
}
