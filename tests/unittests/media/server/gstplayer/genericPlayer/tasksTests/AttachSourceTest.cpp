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

class AttachSourceTest : public GenericTasksTestsBase
{
};

TEST_F(AttachSourceTest, shouldAttachAudioSource)
{
    shouldAttachAudioSource();
    triggerAttachAudioSource();
    checkAudioSourceAttached();
}

TEST_F(AttachSourceTest, shouldAttachAudioSourceWithChannelsAndRateAndDrm)
{
    shouldAttachAudioSourceWithChannelsAndRate();
    triggerAttachAudioSourceWithChannelsAndRateAndDrm();
    checkAudioSourceAttachedWithDrm();
}

TEST_F(AttachSourceTest, shouldAttachOpusWithAudioSpecificConf)
{
    shouldAttachAudioSourceWithAudioSpecificConf();
    triggerAttachOpusAudioSourceWithAudioSpecificConf();
    checkAudioSourceAttached();
}

TEST_F(AttachSourceTest, shouldAttachVideoSource)
{
    shouldAttachVideoSource();
    triggerAttachVideoSource();
    checkVideoSourceAttached();
}

TEST_F(AttachSourceTest, shouldAttachSubtitleSource)
{
    triggerAttachSubtitleSource();
    checkSubtitleSourceAttached();
}

TEST_F(AttachSourceTest, shouldAttachVideoSourceWithStringCodecData)
{
    shouldAttachVideoSourceWithStringCodecData();
    triggerAttachVideoSourceWithStringCodecData();
    checkVideoSourceAttachedWithDrm();
}

TEST_F(AttachSourceTest, shouldAttachVideoSourceEmptyCodecData)
{
    shouldAttachVideoSourceWithEmptyCodecData();
    triggerAttachVideoSourceWithEmptyCodecData();
    checkVideoSourceAttachedWithDrm();
}

TEST_F(AttachSourceTest, shouldAttachVideoDolbyVisionSource)
{
    shouldAttachVideoSourceWithDolbyVisionSource();
    triggerAttachVideoSourceWithDolbyVisionSource();
    checkVideoSourceAttachedWithDrm();
}

TEST_F(AttachSourceTest, shouldSwitchAudioSource)
{
    shouldSwitchAudioSource();
    triggerReattachAudioSource();
    checkNewAudioSourceAttached();
}

TEST_F(AttachSourceTest, shouldReattachAudioSource)
{
    shouldReattachAudioSource();
    triggerReattachAudioSource();
    checkNewAudioSourceAttached();
}

TEST_F(AttachSourceTest, shouldNotReattachAudioSourceWhenMimeTypeIsEmpty)
{
    shouldNotSwitchAudioSourceWhenMimeTypeIsEmpty();
    triggerReattachAudioSourceWithEmptyMimeType();
}
