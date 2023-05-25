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

#include "ControlIpcTestBase.h"

class ControlIpcRegisterClientTest : public ControlIpcTestBase
{
protected:
    ControlIpcRegisterClientTest() = default;
    ~ControlIpcRegisterClientTest() override = default;
};

/**
 * Test that registerClient can be called successfully.
 */
TEST_F(ControlIpcRegisterClientTest, Success)
{
    createControlIpc();
    expectIpcApiCallSuccess();
    registerClient();
    destroyControlIpc();
}

/**
 * Test that registerClient fails if the ipc channel disconnected.
 */
TEST_F(ControlIpcRegisterClientTest, ChannelDisconnected)
{
    createControlIpc();
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    EXPECT_EQ(m_controlIpc->registerClient(), false);
}

/**
 * Test that registerClient fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(ControlIpcRegisterClientTest, ReconnectChannel)
{
    createControlIpc();
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("registerClient"), _, _, _, _));

    EXPECT_EQ(m_controlIpc->registerClient(), true);

    destroyControlIpc();
}

/**
 * Test that registerClient fails when ipc fails.
 */
TEST_F(ControlIpcRegisterClientTest, registerClientFailure)
{
    createControlIpc();
    expectIpcApiCallFailure();
    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("registerClient"), _, _, _, _));

    EXPECT_EQ(m_controlIpc->registerClient(), false);

    destroyControlIpc();
}
