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

class SetSourcePositionTest : public GenericTasksTestsBase
{
protected:
    SetSourcePositionTest()
    {
        setContextPlaybackRate();
        setContextStreamInfo(firebolt::rialto::MediaSourceType::AUDIO);
        setContextStreamInfo(firebolt::rialto::MediaSourceType::VIDEO);
    }
};

TEST_F(SetSourcePositionTest, ShouldNotSetSourcePositionForUnknownSource)
{
    triggerSetSourcePosition(firebolt::rialto::MediaSourceType::UNKNOWN);
    checkInitialPositionNotSet(firebolt::rialto::MediaSourceType::AUDIO);
    checkInitialPositionNotSet(firebolt::rialto::MediaSourceType::VIDEO);
}

TEST_F(SetSourcePositionTest, ShouldNotSetSourcePositionWhenSourceIsNotAccessible)
{
    setContextStreamInfoEmpty();
    triggerSetSourcePosition(firebolt::rialto::MediaSourceType::AUDIO);
    checkInitialPositionNotSet(firebolt::rialto::MediaSourceType::AUDIO);
    checkInitialPositionNotSet(firebolt::rialto::MediaSourceType::VIDEO);
}

TEST_F(SetSourcePositionTest, ShouldSetAudioSourcePosition)
{
    triggerSetSourcePosition(firebolt::rialto::MediaSourceType::AUDIO);
    checkInitialPositionSet(firebolt::rialto::MediaSourceType::AUDIO);
    checkInitialPositionNotSet(firebolt::rialto::MediaSourceType::VIDEO);
}

TEST_F(SetSourcePositionTest, ShouldSetVideoSourcePosition)
{
    triggerSetSourcePosition(firebolt::rialto::MediaSourceType::VIDEO);
    checkInitialPositionSet(firebolt::rialto::MediaSourceType::VIDEO);
    checkInitialPositionNotSet(firebolt::rialto::MediaSourceType::AUDIO);
}

TEST_F(SetSourcePositionTest, ShouldSetSubtitleSourcePosition)
{
    setContextStreamInfo(firebolt::rialto::MediaSourceType::SUBTITLE);
    setContextSetupSourceFinished();
    setContextSubtitleSink();
    shouldSetSubtitleSourcePosition();
    triggerSetSourcePosition(firebolt::rialto::MediaSourceType::SUBTITLE);
    checkInitialPositionNotSet(firebolt::rialto::MediaSourceType::SUBTITLE);
}
