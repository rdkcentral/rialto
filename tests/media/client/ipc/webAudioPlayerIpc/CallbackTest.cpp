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

#include "WebAudioPlayerIpcTestBase.h"
#include "webaudioplayermodule.pb.h"

class RialtoClientWebAudioPlayerIpcCallbackTest : public WebAudioPlayerIpcTestBase
{
protected:

    QosInfo m_qosInfo = {5U, 2U};

    virtual void SetUp()
    {
        WebAudioPlayerIpcTestBase::SetUp();

        createWebAudioPlayerIpc();
    }

    virtual void TearDown()
    {
        destroyWebAudioPlayerIpc();

        WebAudioPlayerIpcTestBase::TearDown();
    }
};

/**
 * Test that a playback state update over IPC is forwarded to the client.
 */
TEST_F(RialtoClientWebAudioPlayerIpcCallbackTest, NotifyPlaybackState)
{
    auto updatePlaybackStateEvent = std::make_shared<firebolt::rialto::WebAudioPlayerStateEvent>();
    updatePlaybackStateEvent->set_web_audio_player_handle(m_webAaudioPlayerHandle);
    updatePlaybackStateEvent->set_state(firebolt::rialto::WebAudioPlayerStateEvent_WebAudioPlayerState_PLAYING);

    EXPECT_CALL(*m_eventThreadMock, addImpl(_)).WillOnce(Invoke([](std::function<void()> &&func) { func(); }));
    EXPECT_CALL(*m_clientMock, notifyState(WebAudioPlayerState::PLAYING));

    m_notifyStateCb(updatePlaybackStateEvent);
}

/**
 * Test that if the session id of the event is not the same as the playback session the event will be ignored.
 */
/*
TEST_F(RialtoClientWebAudioPlayerIpcCallbackTest, InvalidSessionIdPlaybackState)
{
    auto updatePlaybackStateEvent = std::make_shared<firebolt::rialto::WebAudioPlayerStateEvent>();
    updatePlaybackStateEvent->set_web_audio_player_handle(-1);
    updatePlaybackStateEvent->set_state(firebolt::rialto::WebAudioPlayerStateEvent_WebAudioPlayerState_PLAYING);

    EXPECT_CALL(*m_eventThreadMock, addImpl(_)).WillOnce(Invoke([](std::function<void()> &&func) { func(); }));

    m_notifyStateCb(updatePlaybackStateEvent);
}
*/
