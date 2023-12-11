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

#include "MediaPipelineTest.h"

namespace firebolt::rialto::server::ct
{
TEST_F(MediaPipelineTest, shouldAttachAudioSourceOnly)
{
    createSession();
    gstPlayerWillBeCreated();
    load();
    audioSourceWillBeAttached();
    attachAudioSource();
    sourceWillBeSetup();
    setupSource();
    willSetupAndAddSource(&m_audioAppSrc);
    willFinishSetupAndAddSource();
    indicateAllSourcesAttached();
    willRemoveAudioSource();
    removeSource(m_audioSourceId);
    willStop();
    stop();
    gstPlayerWillBeDestructed();
}

TEST_F(MediaPipelineTest, shouldAttachBothSources)
{
    createSession();
    gstPlayerWillBeCreated();
    load();
    audioSourceWillBeAttached();
    attachAudioSource();
    videoSourceWillBeAttached();
    attachVideoSource();
    sourceWillBeSetup();
    setupSource();
    willSetupAndAddSource(&m_audioAppSrc);
    willSetupAndAddSource(&m_videoAppSrc);
    willFinishSetupAndAddSource();
    indicateAllSourcesAttached();
    willRemoveAudioSource();
    removeSource(m_audioSourceId);
    removeSource(m_videoSourceId);
    willStop();
    stop();
    gstPlayerWillBeDestructed();
}
} // namespace firebolt::rialto::server::ct
