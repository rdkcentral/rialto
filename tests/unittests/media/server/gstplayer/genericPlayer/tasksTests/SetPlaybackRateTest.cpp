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

#if GST_CHECK_VERSION(1, 18, 0)
namespace
{
constexpr bool kEnableInstantRateChangeSeek{true};
} // namespace
#endif

class SetPlaybackRateTest : public GenericTasksTestsBase
{
};

TEST_F(SetPlaybackRateTest, shouldNotChangePlaybackRateIfItsAlreadySet)
{
    setContextPlaybackRate();
    triggerSetPlaybackRate();
    checkNoPendingPlaybackRate();
    checkPlaybackRateSet();
}

TEST_F(SetPlaybackRateTest, shouldNotChangePlaybackRateIfPipelineIsNull)
{
    setContextPipelineNull();
    triggerSetPlaybackRate();
    checkPendingPlaybackRate();
    checkPlaybackRateDefault();
}

TEST_F(SetPlaybackRateTest, shouldNotChangePlaybackRateIfPipelineStateIsBelowPlaying)
{
    triggerSetPlaybackRate();
    checkPendingPlaybackRate();
    checkPlaybackRateDefault();
}

TEST_F(SetPlaybackRateTest, shouldSetPlaybackRateAudioSinkNull)
{
    setPipelinePlaying();
    shouldSetPlaybackRateAudioSinkNullSuccess();
    triggerSetPlaybackRate();
    checkNoPendingPlaybackRate();
    checkPlaybackRateSet();
}

TEST_F(SetPlaybackRateTest, shouldFailToSetPlaybackRateAudioSinkNull)
{
    setPipelinePlaying();
    shouldSetPlaybackRateAudioSinkNullFailure();
    triggerSetPlaybackRate();
    checkNoPendingPlaybackRate();
    checkPlaybackRateDefault();
}

TEST_F(SetPlaybackRateTest, shouldSetPlaybackRateAudioSinkOtherThanAmlhala)
{
    setPipelinePlaying();
    shouldSetPlaybackRateAudioSinkOtherThanAmlhala();
    triggerSetPlaybackRate();
    checkNoPendingPlaybackRate();
    checkPlaybackRateSet();
}

TEST_F(SetPlaybackRateTest, shouldFailToSetPlaybackRateAudioSinkOtherThanAmlhala)
{
    setPipelinePlaying();
    shouldFailToSetPlaybackRateAudioSinkOtherThanAmlhala();
    triggerSetPlaybackRate();
    checkNoPendingPlaybackRate();
    checkPlaybackRateDefault();
}

TEST_F(SetPlaybackRateTest, shouldSetPlaybackRateAmlhalaAudioSink)
{
    setPipelinePlaying();
    shouldSetPlaybackRateAmlhalaAudioSink();
    triggerSetPlaybackRate();
    checkNoPendingPlaybackRate();
    checkPlaybackRateSet();
    checkSegmentInfo();
}

TEST_F(SetPlaybackRateTest, shouldFailToSetPlaybackRateAmlhalaAudioSink)
{
    setPipelinePlaying();
    shouldFailToSetPlaybackRateAmlhalaAudioSink();
    triggerSetPlaybackRate();
    checkNoPendingPlaybackRate();
    checkPlaybackRateDefault();
    checkSegmentInfo();
}

#if GST_CHECK_VERSION(1, 18, 0)
TEST_F(SetPlaybackRateTest, shouldSetPlaybackRateUsingSeek)
{
    setPipelinePlaying();
    shouldSetPlaybackRateUsingSeek();
    triggerSetPlaybackRate(kEnableInstantRateChangeSeek);
    checkNoPendingPlaybackRate();
    checkPlaybackRateSet();
}

TEST_F(SetPlaybackRateTest, shouldFailToSetPlaybackRateUsingSeek)
{
    setPipelinePlaying();
    shouldFailToSetPlaybackRateUsingSeek();
    triggerSetPlaybackRate(kEnableInstantRateChangeSeek);
    checkNoPendingPlaybackRate();
    checkPlaybackRateDefault();
}
#endif
