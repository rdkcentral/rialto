/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
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

#include "IIpcChannel.h"
#include "IIpcServer.h"
#include "IIpcServerFactory.h"
#include "RialtoLogging.h"
#include "TestModuleMock.h"
#include "rialtoipc-transport.pb.h"
#include "testmodule.pb.h"

#include <gtest/gtest.h>

#include <dirent.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <mutex>
#include <string>
#include <vector>

using namespace firebolt::rialto;
using namespace firebolt::rialto::ipc;
using ::testing::StrictMock;

namespace
{
// Number of entries in /proc/self/fd, including the one used to read it.
size_t openFdCount()
{
    size_t count{0};
    DIR *dir = opendir("/proc/self/fd");
    if (!dir)
        return 0;
    while (readdir(dir) != nullptr)
        ++count;
    closedir(dir);
    return count;
}

std::vector<int> createFiles(size_t count)
{
    std::vector<int> fds;
    for (size_t i = 0; i < count; ++i)
        fds.push_back(memfd_create("rialto-ipc-mismatch", MFD_CLOEXEC));
    return fds;
}

void closeAll(const std::vector<int> &fds)
{
    for (int fd : fds)
        close(fd);
}

// A message with four present FD fields.
TestFdMixed makeFourFdMessage()
{
    TestFdMixed message;
    message.set_first_fd(-1);
    message.add_holders()->set_fd(-1);
    message.add_holders()->set_fd(-1);
    message.set_last_fd(-1);
    return message;
}

// Writes one SOCK_SEQPACKET message with \a fds attached as SCM_RIGHTS.
bool sendRaw(int sock, const std::string &data, const std::vector<int> &fds)
{
    struct iovec iov{const_cast<char *>(data.data()), data.size()};
    std::vector<uint8_t> control(CMSG_SPACE(sizeof(int) * fds.size()));

    struct msghdr header{};
    header.msg_iov = &iov;
    header.msg_iovlen = 1;
    if (!fds.empty())
    {
        header.msg_control = control.data();
        header.msg_controllen = control.size();
        struct cmsghdr *cmsg = CMSG_FIRSTHDR(&header);
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;
        cmsg->cmsg_len = CMSG_LEN(sizeof(int) * fds.size());
        memcpy(CMSG_DATA(cmsg), fds.data(), sizeof(int) * fds.size());
    }
    return sendmsg(sock, &header, MSG_NOSIGNAL) == static_cast<ssize_t>(data.size());
}

// Collects IPC error log lines while it exists.
class IpcErrorLog
{
public:
    IpcErrorLog()
    {
        logging::setLogHandler(
            RIALTO_COMPONENT_IPC,
            [this](RIALTO_DEBUG_LEVEL level, const char *, int, const char *, const char *message, std::size_t length)
            {
                if (level == RIALTO_DEBUG_LEVEL_ERROR)
                {
                    std::lock_guard<std::mutex> lock{m_mutex};
                    m_errors.emplace_back(message, length);
                }
            },
            true);
    }
    ~IpcErrorLog() { logging::setLogHandler(RIALTO_COMPONENT_IPC, nullptr, false); }

    bool contains(const std::string &text)
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        for (const auto &error : m_errors)
        {
            if (error.find(text) != std::string::npos)
                return true;
        }
        return false;
    }

private:
    std::mutex m_mutex;
    std::vector<std::string> m_errors;
};
} // namespace

class FdMismatchTest : public ::testing::Test
{
protected:
    void SetUp() override { ASSERT_EQ(socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0, m_sockets), 0); }

    void TearDown() override
    {
        for (int sock : m_sockets)
        {
            if (sock >= 0)
                close(sock);
        }
    }

    // A real server on m_sockets[0]; the test writes requests to m_sockets[1].
    void testRequestWithFds(size_t attachedFds)
    {
        auto moduleMock = std::make_shared<StrictMock<TestModuleMock>>(); // any handler call fails the test
        auto server = IServerFactory::createFactory()->create();
        ASSERT_TRUE(server);
        auto client = server->addClient(m_sockets[0]);
        ASSERT_TRUE(client);
        client->exportService(moduleMock);

        transport::MessageToServer message;
        auto *call = message.mutable_call();
        call->set_serial_id(1);
        call->set_service_name(TestModule::descriptor()->full_name());
        call->set_method_name("TestFdMixedRequest");
        call->set_request_message(makeFourFdMessage().SerializeAsString());

        const auto kFiles = createFiles(attachedFds);
        const size_t kBaseline = openFdCount();
        IpcErrorLog log;

        ASSERT_TRUE(sendRaw(m_sockets[1], message.SerializeAsString(), kFiles));
        for (int i = 0; i < 20; ++i)
        {
            server->process();
            server->wait(1);
        }

        EXPECT_TRUE(log.contains("mismatch of file descriptors to the request"));
        EXPECT_EQ(openFdCount(), kBaseline);

        client->disconnect();
        closeAll(kFiles);
    }

    // A real client channel on m_sockets[0]; the test writes events to m_sockets[1].
    void testEventWithFds(size_t attachedFds)
    {
        auto channel = IChannelFactory::createFactory()->createChannel(m_sockets[0]);
        ASSERT_TRUE(channel);
        m_sockets[0] = -1; // owned by the channel now
        bool handled{false};
        const int kTag = channel->subscribe<TestFdMixed>([&handled](const std::shared_ptr<TestFdMixed> &)
                                                         { handled = true; });

        transport::MessageFromServer message;
        auto *event = message.mutable_event();
        event->set_event_name(TestFdMixed::descriptor()->full_name());
        event->set_message(makeFourFdMessage().SerializeAsString());

        const auto kFiles = createFiles(attachedFds);
        const size_t kBaseline = openFdCount();
        IpcErrorLog log;

        ASSERT_TRUE(sendRaw(m_sockets[1], message.SerializeAsString(), kFiles));
        for (int i = 0; i < 20; ++i)
        {
            channel->process();
            channel->wait(1);
        }

        EXPECT_FALSE(handled);
        EXPECT_TRUE(log.contains("mismatch of file descriptors to the reply"));
        EXPECT_EQ(openFdCount(), kBaseline);

        channel->unsubscribe(kTag);
        channel->disconnect();
        closeAll(kFiles);
    }

    int m_sockets[2]{-1, -1};
};

TEST_F(FdMismatchTest, ShouldRejectRequestWithTooFewFds)
{
    testRequestWithFds(3);
}

TEST_F(FdMismatchTest, ShouldRejectRequestWithTooManyFds)
{
    testRequestWithFds(5);
}

TEST_F(FdMismatchTest, ShouldRejectEventWithTooFewFds)
{
    testEventWithFds(3);
}

TEST_F(FdMismatchTest, ShouldRejectEventWithTooManyFds)
{
    testEventWithFds(5);
}
