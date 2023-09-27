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

using testing::Invoke;
using testing::WithArgs;

namespace
{
int32_t kFd{123};
uint32_t kSize{456};
} // namespace

class ControlIpcGetSharedMemoryTest : public ControlIpcTestBase
{
protected:
    ControlIpcGetSharedMemoryTest() = default;
    ~ControlIpcGetSharedMemoryTest() override = default;

    int32_t m_fd;
    uint32_t m_size;
};

/**
 * Test that getSharedMemory can be called successfully.
 */
TEST_F(ControlIpcGetSharedMemoryTest, Success)
{
    createControlIpc();
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("getSharedMemory"), m_controllerMock.get(), _, _, m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(
            [](google::protobuf::Message *response)
            {
                firebolt::rialto::GetSharedMemoryResponse *getShmResp =
                    dynamic_cast<firebolt::rialto::GetSharedMemoryResponse *>(response);
                getShmResp->set_fd(kFd);
                getShmResp->set_size(kSize);
            })));

    EXPECT_EQ(m_controlIpc->getSharedMemory(m_fd, m_size), true);
    EXPECT_EQ(kFd, m_fd);
    EXPECT_EQ(kSize, m_size);

    destroyControlIpc();
}

/**
 * Test that getSharedMemory fails if the ipc channel disconnected.
 */
TEST_F(ControlIpcGetSharedMemoryTest, ChannelDisconnected)
{
    createControlIpc();
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();
    EXPECT_CALL(m_ipcClientMock, unregisterConnectionObserver());

    EXPECT_EQ(m_controlIpc->getSharedMemory(m_fd, m_size), false);
}

/**
 * Test that getSharedMemory fails if the ipc channel disconnected and succeeds if the channel is reconnected.
 */
TEST_F(ControlIpcGetSharedMemoryTest, ReconnectChannel)
{
    createControlIpc();
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getSharedMemory"), _, _, _, _));

    EXPECT_EQ(m_controlIpc->getSharedMemory(m_fd, m_size), true);

    destroyControlIpc();
}

/**
 * Test that getSharedMemory fails when ipc fails.
 */
TEST_F(ControlIpcGetSharedMemoryTest, getSharedMemoryFailure)
{
    createControlIpc();
    expectIpcApiCallFailure();
    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("getSharedMemory"), _, _, _, _));

    EXPECT_EQ(m_controlIpc->getSharedMemory(m_fd, m_size), false);

    destroyControlIpc();
}
