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

class EosTest : public GenericTasksTestsBase
{
};

TEST_F(EosTest, shouldFailWhenStreamIsNotFound)
{
    triggerEosAudio();
}

TEST_F(EosTest, shouldSetEos)
{
    setContextStreamInfo(firebolt::rialto::MediaSourceType::AUDIO);
    shouldGstAppSrcEndOfStreamSuccess();
    triggerEosAudio();
}

TEST_F(EosTest, shouldFailToSetEos)
{
    setContextStreamInfo(firebolt::rialto::MediaSourceType::AUDIO);
    shouldGstAppSrcEndOfStreamFailure();
    triggerEosAudio();
}

TEST_F(EosTest, shouldSetEosForAudioAndCancelAudioUnderflow)
{
    setContextStreamInfo(firebolt::rialto::MediaSourceType::AUDIO);
    setContextAudioUnderflowOccured(true);
    shouldCancelUnderflow();
    shouldGstAppSrcEndOfStreamSuccess();
    triggerEosAudio();
}

TEST_F(EosTest, shouldSetEosForAudioAndSkipCancellingVideoUnderflow)
{
    setContextStreamInfo(firebolt::rialto::MediaSourceType::AUDIO);
    setContextVideoUnderflowOccured(true);
    shouldGstAppSrcEndOfStreamSuccess();
    triggerEosAudio();
}

TEST_F(EosTest, shouldSetEosForVideoAndCancelVideoUnderflow)
{
    setContextStreamInfo(firebolt::rialto::MediaSourceType::VIDEO);
    setContextVideoUnderflowOccured(true);
    shouldCancelUnderflow();
    shouldGstAppSrcEndOfStreamSuccess();
    triggerEosVideo();
}

TEST_F(EosTest, shouldSetEosForVideoAndSkipCancellingAudioUnderflow)
{
    setContextStreamInfo(firebolt::rialto::MediaSourceType::VIDEO);
    setContextAudioUnderflowOccured(true);
    shouldGstAppSrcEndOfStreamSuccess();
    triggerEosVideo();
}
