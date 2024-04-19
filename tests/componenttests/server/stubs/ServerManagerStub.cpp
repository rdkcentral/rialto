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

#include "ServerManagerStub.h"
#include "servermanagermodule.pb.h"
#include <functional>
#include <gtest/gtest.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

namespace
{
constexpr std::chrono::milliseconds kChannelTimeout{200};
} // namespace

namespace firebolt::rialto::server::ct
{
ServerManagerStub::ServerManagerStub()
{
    EXPECT_GE(socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC | SOCK_NONBLOCK, 0, m_socks.data()), 0);
    m_ipcThread = std::thread(std::bind(&ServerManagerStub::ipcThread, this));
}

ServerManagerStub::~ServerManagerStub()
{
    if (m_ipcThread.joinable())
    {
        m_ipcThread.join();
    }
    if (m_ipcChannel)
    {
        teardownSubscriptions(m_ipcChannel);
    }

    // Shutdown and close the server socket
    // Other socket will be closed by the ipc channel
    shutdown(m_socks[0], SHUT_RDWR);
    close(m_socks[0]);
}

void ServerManagerStub::ipcThread()
{
    {
        std::unique_lock<std::mutex> locker(m_channelLock);
        auto factory = firebolt::rialto::ipc::IChannelFactory::createFactory();
        m_ipcChannel = factory->createChannel(m_socks[1]);
        EXPECT_TRUE(m_ipcChannel);
        m_channelCond.notify_all();
    }
    setupSubscriptions<::rialto::StateChangedEvent, ::rialto::AckEvent>(m_ipcChannel);
    while (m_ipcChannel->process())
    {
        m_ipcChannel->wait(-1);
    }
}

int ServerManagerStub::getServerSocket() const
{
    return m_socks[0];
}

std::shared_ptr<::firebolt::rialto::ipc::IChannel> ServerManagerStub::getChannel()
{
    // Wait for the channel to be created incase it hasn't yet
    std::unique_lock<std::mutex> locker(m_channelLock);
    if (!m_ipcChannel)
    {
        m_channelCond.wait_for(locker, kChannelTimeout, [this]() { return static_cast<bool>(m_ipcChannel); });
    }
    return m_ipcChannel;
}
} // namespace firebolt::rialto::server::ct
