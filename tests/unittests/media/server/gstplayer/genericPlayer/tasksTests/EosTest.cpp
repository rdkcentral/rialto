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
    shouldCancelUnderflow(firebolt::rialto::MediaSourceType::AUDIO);
    triggerEosAudio();
}

TEST_F(EosTest, shouldSetEos)
{
    setContextStreamInfo(firebolt::rialto::MediaSourceType::AUDIO);
    shouldGstAppSrcEndOfStreamSuccess();
    shouldCancelUnderflow(firebolt::rialto::MediaSourceType::AUDIO);
    triggerEosAudio();
    shouldSetEos(firebolt::rialto::MediaSourceType::AUDIO);
}

TEST_F(EosTest, shouldFailToSetEos)
{
    setContextStreamInfo(firebolt::rialto::MediaSourceType::AUDIO);
    shouldGstAppSrcEndOfStreamFailure();
    shouldCancelUnderflow(firebolt::rialto::MediaSourceType::AUDIO);
    triggerEosAudio();
}

TEST_F(EosTest, shouldSetEosForAudioAndCancelAudioUnderflow)
{
    setContextStreamInfo(firebolt::rialto::MediaSourceType::AUDIO);
    setContextAudioUnderflowOccured(true);
    shouldCancelUnderflow(firebolt::rialto::MediaSourceType::AUDIO);
    shouldGstAppSrcEndOfStreamSuccess();
    triggerEosAudio();
    shouldSetEos(firebolt::rialto::MediaSourceType::AUDIO);
}

TEST_F(EosTest, shouldSetEosForAudioAndSkipCancellingVideoUnderflow)
{
    setContextStreamInfo(firebolt::rialto::MediaSourceType::AUDIO);
    setContextStreamInfo(firebolt::rialto::MediaSourceType::VIDEO);
    shouldCancelUnderflow(firebolt::rialto::MediaSourceType::AUDIO);
    setContextVideoUnderflowOccured(true);
    shouldGstAppSrcEndOfStreamSuccess();
    triggerEosAudio();
    shouldSetEos(firebolt::rialto::MediaSourceType::AUDIO);
}

TEST_F(EosTest, shouldSetEosForVideoAndCancelVideoUnderflow)
{
    setContextStreamInfo(firebolt::rialto::MediaSourceType::VIDEO);
    setContextVideoUnderflowOccured(true);
    shouldCancelUnderflow(firebolt::rialto::MediaSourceType::VIDEO);
    shouldGstAppSrcEndOfStreamSuccess();
    triggerEosVideo();
    shouldSetEos(firebolt::rialto::MediaSourceType::VIDEO);
}

TEST_F(EosTest, shouldSetEosForVideoAndSkipCancellingAudioUnderflow)
{
    setContextStreamInfo(firebolt::rialto::MediaSourceType::AUDIO);
    setContextStreamInfo(firebolt::rialto::MediaSourceType::VIDEO);
    setContextAudioUnderflowOccured(true);
    shouldGstAppSrcEndOfStreamSuccess();
    shouldCancelUnderflow(firebolt::rialto::MediaSourceType::VIDEO);
    triggerEosVideo();
    shouldSetEos(firebolt::rialto::MediaSourceType::VIDEO);
}

TEST_F(EosTest, shouldNotEosWhenDataIsBuffered)
{
    setContextStreamInfo(firebolt::rialto::MediaSourceType::AUDIO);
    setContextAudioBuffer();
    shouldCancelUnderflow(firebolt::rialto::MediaSourceType::AUDIO);
    triggerEosAudio();
    shouldSetEosPending(firebolt::rialto::MediaSourceType::AUDIO);
}

TEST_F(EosTest, shouldSendEosWhenEosPending)
{
    setContextStreamInfo(firebolt::rialto::MediaSourceType::AUDIO);
    setContextEndOfStream(firebolt::rialto::MediaSourceType::AUDIO, false);
    shouldGstAppSrcEndOfStreamSuccess();
    shouldCancelUnderflow(firebolt::rialto::MediaSourceType::AUDIO);
    triggerEosAudio();
    shouldSetEos(firebolt::rialto::MediaSourceType::AUDIO);
}

TEST_F(EosTest, shouldNotSendEosWhenAlreadySetEos)
{
    setContextStreamInfo(firebolt::rialto::MediaSourceType::AUDIO);
    setContextEndOfStream(firebolt::rialto::MediaSourceType::AUDIO, true);
    shouldCancelUnderflow(firebolt::rialto::MediaSourceType::AUDIO);
    triggerEosAudio();
}