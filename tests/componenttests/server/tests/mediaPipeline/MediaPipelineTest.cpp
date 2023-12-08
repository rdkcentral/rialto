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

#include "ActionTraits.h"
#include "ConfigureAction.h"
#include "ExpectMessage.h"
#include "MediaPipelineTestFixture.h"
#include "MessageBuilders.h"

namespace
{
constexpr int kFrameCountInPausedState{3};
} // namespace

namespace firebolt::rialto::server::ct
{
TEST_F(MediaPipelineTest, shouldCreatePipeline)
{
    createSession();
}

TEST_F(MediaPipelineTest, shouldFailToLoadWhenSessionIdIsWrong)
{
    createSession();
    auto request = createLoadRequest(m_sessionId + 1);
    ConfigureAction<Load>(m_clientStub).send(request).expectFailure();
}

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
    gstPlayerWillBeDestructed();
}

TEST_F(MediaPipelineTest, playback)
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
    willPause();
    pause();
    gstNeedData(&m_audioAppSrc, kFrameCountInPausedState);
    gstNeedData(&m_videoAppSrc, kFrameCountInPausedState);
    {
        ExpectMessage<firebolt::rialto::NetworkStateChangeEvent> expectedNetworkStateChange{m_clientStub};

        pushAudioData(3);
        pushVideoData(3);

        auto receivedNetworkStateChange{expectedNetworkStateChange.getMessage()};
        ASSERT_TRUE(receivedNetworkStateChange);
        EXPECT_EQ(receivedNetworkStateChange->session_id(), m_sessionId);
        EXPECT_EQ(receivedNetworkStateChange->state(), ::firebolt::rialto::NetworkStateChangeEvent_NetworkState_BUFFERED);
    }
    gstPlayerWillBeDestructed();
}
} // namespace firebolt::rialto::server::ct
