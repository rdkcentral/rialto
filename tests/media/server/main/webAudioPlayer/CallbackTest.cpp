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

#include "IGstWebAudioPlayerClient.h"
#include "WebAudioPlayerTestBase.h"

using ::testing::ByMove;
using ::testing::DoAll;
using ::testing::SaveArg;

class RialtoServerWebAudioPlayerCallbackTest : public WebAudioPlayerTestBase
{
protected:
    RialtoServerWebAudioPlayerCallbackTest()
    {
        createWebAudioPlayer();
    }

    ~RialtoServerWebAudioPlayerCallbackTest() { destroyWebAudioPlayer(); }
};

/**
 * Test a notification of the state is forwarded to the registered client.
 */
TEST_F(RialtoServerWebAudioPlayerCallbackTest, notifyState)
{
    WebAudioPlayerState state = WebAudioPlayerState::IDLE;
    mainThreadWillEnqueueTask();
    EXPECT_CALL(*m_webAudioPlayerClientMock, notifyState(state));

    m_webAudioPlayer->notifyState(state);
}
