/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Sky UK
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

class FlushTest : public GenericTasksTestsBase
{
protected:
    FlushTest()
    {
        setContextStreamInfo(firebolt::rialto::MediaSourceType::AUDIO);
        setContextStreamInfo(firebolt::rialto::MediaSourceType::VIDEO);
        setContextNeedData(true);
        setContextNeedDataPending(true);
        setContextAudioBuffer();
        setContextVideoBuffer();
        setContextEndOfStream(firebolt::rialto::MediaSourceType::AUDIO);
        setContextEndOfStream(firebolt::rialto::MediaSourceType::VIDEO);
        setContextEndOfStreamNotified();
    }
};

TEST_F(FlushTest, ShouldNotFlushUnknownSource)
{
    // Hack for coverage :)
    setContextStreamInfo(firebolt::rialto::MediaSourceType::UNKNOWN);
    triggerFlush(firebolt::rialto::MediaSourceType::UNKNOWN);
}

TEST_F(FlushTest, ShouldNotFlushWhenSourceIsNotAccessible)
{
    setContextStreamInfoEmpty();
    triggerFlush(firebolt::rialto::MediaSourceType::AUDIO);
}

TEST_F(FlushTest, ShouldFlushAudio)
{
    shouldFlushAudio();
    shouldFlushAudioSrcSuccess();
    setPipelinePlaying();
    triggerFlush(firebolt::rialto::MediaSourceType::AUDIO);
    checkAudioFlushed();
}

TEST_F(FlushTest, ShouldFlushAudioWithoutSendingEventBelowPaused)
{
    shouldFlushAudio();
    triggerFlush(firebolt::rialto::MediaSourceType::AUDIO);
    checkAudioFlushed();
}

TEST_F(FlushTest, ShouldFlushAudioEvenIfEventSendingFails)
{
    shouldFlushAudio();
    shouldFlushAudioSrcFailure();
    setPipelinePlaying();
    triggerFlush(firebolt::rialto::MediaSourceType::AUDIO);
    checkAudioFlushed();
}

TEST_F(FlushTest, ShouldFlushAudioWithNeedData)
{
    setContextEndOfStream(firebolt::rialto::MediaSourceType::AUDIO);
    setContextEndOfStreamNotified();
    setContextSetupSourceFinished();
    shouldFlushAudio();
    shouldFlushAudioSrcSuccess();
    shouldNotifyNeedAudioDataSuccess();
    setPipelinePlaying();
    triggerFlush(firebolt::rialto::MediaSourceType::AUDIO);
    checkAudioFlushed();
}

TEST_F(FlushTest, ShouldFlushVideo)
{
    shouldFlushVideo();
    shouldFlushVideoSrcSuccess();
    setPipelinePlaying();
    triggerFlush(firebolt::rialto::MediaSourceType::VIDEO);
    checkVideoFlushed();
}

TEST_F(FlushTest, ShouldFlushVideoWithNeedData)
{
    setContextEndOfStream(firebolt::rialto::MediaSourceType::VIDEO);
    setContextEndOfStreamNotified();
    setContextSetupSourceFinished();
    shouldFlushVideo();
    shouldFlushVideoSrcSuccess();
    shouldNotifyNeedVideoDataSuccess();
    setPipelinePlaying();
    triggerFlush(firebolt::rialto::MediaSourceType::VIDEO);
    checkVideoFlushed();
}
