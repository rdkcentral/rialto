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

#include "ClientStub.h"
#include "IIpcController.h"
#include "IIpcServer.h"
#include "IIpcServerFactory.h"
#include "ServerStub.h"
#include "TestClientMock.h"
#include "TestModuleMock.h"
#include "rialtoipc-transport.pb.h"
#include <gtest/gtest.h>

#include <dirent.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace firebolt::rialto;
using namespace firebolt::rialto::ipc;

using ::testing::_;
using ::testing::ByMove;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::StrictMock;
using ::testing::WithArgs;

MATCHER_P(SingleVarRequestMatcher, var1, "")
{
    return (arg->var1() == var1);
}

MATCHER_P4(MultiVarRequestMatcher, var1, var2, var3, var4, "")
{
    return ((arg->var1() == var1) && (arg->var2() == var2) && (arg->var3() == var3) && (arg->var4() == var4));
}

class RialtoIpcTest : public ::testing::Test
{
protected:
    std::shared_ptr<StrictMock<TestModuleMock>> m_testModuleMock;
    std::shared_ptr<StrictMock<TestClientMock>> m_testClientMock;
    std::shared_ptr<ServerStub> m_serverStub;
    std::shared_ptr<ClientStub> m_clientStub;
    std::string m_socketName = "/tmp/rialto-0";

    int32_t m_int = 432;
    uint32_t m_uint = 678U;
    firebolt::rialto::TestMultiVar_TestType m_enum = firebolt::rialto::TestMultiVar_TestType_ENUM1;
    std::string m_str = "test";

    virtual void SetUp()
    {
        m_testModuleMock = std::make_shared<StrictMock<TestModuleMock>>();
        m_testClientMock = std::make_shared<StrictMock<TestClientMock>>();

        m_serverStub = std::make_shared<ServerStub>(m_testModuleMock);
        m_serverStub->init();

        m_clientStub = std::make_shared<ClientStub>(m_testClientMock, m_socketName);
        m_clientStub->connect();
    }

    virtual void TearDown()
    {
        if (m_clientStub)
        {
            m_clientStub->disconnect();
            m_clientStub.reset();
        }

        m_serverStub.reset();

        m_testClientMock.reset();
        m_testModuleMock.reset();
    }
};

/**
 * Test that IPC can send a request with a single variable.
 */
TEST_F(RialtoIpcTest, SingleVarRequest)
{
    EXPECT_CALL(*m_testModuleMock, TestRequestSingleVar(_, SingleVarRequestMatcher(m_int), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_testModuleMock), &TestModuleMock::defaultReturn)));

    EXPECT_TRUE(m_clientStub->sendSingleVarRequest(m_int));
}

/**
 * Test that IPC can send a request and expect a response with a single variable.
 */
TEST_F(RialtoIpcTest, SingleVarResponse)
{
    int32_t retInt = 0;

    EXPECT_CALL(*m_testModuleMock, TestResponseSingleVar(_, _, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_testModuleMock->getSingleVarResponse(m_int)),
                        WithArgs<0, 3>(Invoke(&(*m_testModuleMock), &TestModuleMock::defaultReturn))));

    EXPECT_TRUE(m_clientStub->sendRequestWithSingleVarResponse(retInt));

    EXPECT_EQ(m_int, retInt);
}

/**
 * Test that IPC can send a request with a multiple variables.
 */
TEST_F(RialtoIpcTest, MultiVarRequest)
{
    EXPECT_CALL(*m_testModuleMock, TestRequestMultiVar(_, MultiVarRequestMatcher(m_int, m_uint, m_enum, m_str), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_testModuleMock), &TestModuleMock::defaultReturn)));

    EXPECT_TRUE(m_clientStub->sendMultiVarRequest(m_int, m_uint, m_enum, m_str));
}

/**
 * Test that IPC can send a request and expect a response with multiple variables.
 */
TEST_F(RialtoIpcTest, MultiVarResponse)
{
    int32_t retInt = 0;
    uint32_t retUint = 0;
    firebolt::rialto::TestMultiVar_TestType retEnum = firebolt::rialto::TestMultiVar_TestType_ENUM2;
    std::string retStr = "";

    EXPECT_CALL(*m_testModuleMock, TestResponseMultiVar(_, _, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_testModuleMock->getMultiVarResponse(m_int, m_uint, m_enum, m_str)),
                        WithArgs<0, 3>(Invoke(&(*m_testModuleMock), &TestModuleMock::defaultReturn))));

    EXPECT_TRUE(m_clientStub->sendRequestWithMultiVarResponse(retInt, retUint, retEnum, retStr));

    EXPECT_EQ(m_int, retInt);
    EXPECT_EQ(m_uint, retUint);
    EXPECT_EQ(m_enum, retEnum);
    EXPECT_EQ(m_str, retStr);
}

/**
 * Test that IPC client returns failure if server fails.
 */
TEST_F(RialtoIpcTest, ServerFailure)
{
    EXPECT_CALL(*m_testModuleMock, TestRequestSingleVar(_, _, _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(&(*m_testModuleMock), &TestModuleMock::failureReturn)));

    EXPECT_FALSE(m_clientStub->sendSingleVarRequest(m_int));
}

/**
 * Test that IPC client can received single variable events from the server.
 */
TEST_F(RialtoIpcTest, SingleVarEvent)
{
    int32_t retInt = 0;

    m_clientStub->startMessageThread();

    m_serverStub->sendSingleVarEvent(m_int);

    m_clientStub->waitForSingleVarEvent(retInt);
}

/**
 * Test that IPC client can received multiple variable events from the server.
 */
TEST_F(RialtoIpcTest, MultiVarEvent)
{
    firebolt::rialto::TestEventMultiVar_TestType eventEnum = firebolt::rialto::TestEventMultiVar_TestType_ENUM1;
    int32_t retInt = 0;
    uint32_t retUint = 0;
    firebolt::rialto::TestEventMultiVar_TestType retEnum = firebolt::rialto::TestEventMultiVar_TestType_ENUM2;
    std::string retStr = "";

    m_clientStub->startMessageThread();

    m_serverStub->sendMultiVarEvent(m_int, m_uint, eventEnum, m_str);

    m_clientStub->waitForMultiVarEvent(retInt, retUint, retEnum, retStr);
}

/**
 * Test that IPC client returns false when message is timeouted.
 */
TEST_F(RialtoIpcTest, Timeout)
{
    constexpr bool kExpectMessage{false};
    ::google::protobuf::RpcController *controller;
    ::google::protobuf::Closure *done;
    m_clientStub->startMessageThread(kExpectMessage);

    EXPECT_CALL(*m_testModuleMock, TestRequestSingleVar(_, SingleVarRequestMatcher(m_int), _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(
            [&](auto *c, auto *d)
            {
                controller = c;
                done = d;
            })));

    EXPECT_FALSE(m_clientStub->sendSingleVarRequest(m_int));

    // Disconnect and join client stub thread before calling failureReturn
    // to fix the data race causing code coverage count issues
    m_clientStub.reset();

    // Call failureReturn at the end to prevent mock leak
    m_testModuleMock->failureReturn(controller, done);
}

/**
 * Test that IPC client can process no reply request
 */
TEST_F(RialtoIpcTest, NoReply)
{
    constexpr bool kExpectMessage{false};
    m_clientStub->startMessageThread(kExpectMessage);

    std::mutex messageReceivedMutex;
    std::condition_variable messageReceivedCond;
    bool messageReceived = false;

    EXPECT_CALL(*m_testModuleMock, TestRequestSingleVarNoReply(_, _, _, _))
        .WillOnce(WithArgs<0, 3>(Invoke(
            [&](auto *c, auto *d)
            {
                std::unique_lock<std::mutex> messageReceivedLock(messageReceivedMutex);
                m_testModuleMock->defaultReturn(c, d);
                messageReceived = true;
                messageReceivedCond.notify_one();
            })));

    EXPECT_TRUE(m_clientStub->sendSingleVarRequestWithNoReply(m_int));
    std::unique_lock<std::mutex> messageReceivedLock(messageReceivedMutex);
    bool status = messageReceivedCond.wait_for(messageReceivedLock, std::chrono::milliseconds(100),
                                               [&] { return messageReceived; });

    ASSERT_TRUE(status);
}

// -----------------------------------------------------------------------------
// File descriptor transfer at any depth (vfc-ipc-nested-fd-transfer, task 2.3)
// -----------------------------------------------------------------------------

namespace
{
constexpr int kEventTimeoutMs{1000};

std::vector<int> createFiles(size_t count, const char *name = "rialto-ipc-test")
{
    std::vector<int> fds;
    for (size_t i = 0; i < count; ++i)
    {
        const int kFd = memfd_create(name, MFD_CLOEXEC);
        EXPECT_GE(kFd, 0);
        fds.push_back(kFd);
    }
    return fds;
}

void closeAll(const std::vector<int> &fds)
{
    for (int fd : fds)
    {
        if (fd >= 0)
            close(fd);
    }
}

bool refersToSameFile(int first, int second)
{
    struct stat firstStat
    {
    };
    struct stat secondStat
    {
    };
    if (fstat(first, &firstStat) != 0 || fstat(second, &secondStat) != 0)
        return false;
    return firstStat.st_dev == secondStat.st_dev && firstStat.st_ino == secondStat.st_ino;
}

// Every received descriptor must be a different descriptor number for the same file, in the same position.
void expectSameFilesInOrder(const std::vector<int> &sent, const std::vector<int> &received)
{
    ASSERT_EQ(sent.size(), received.size());
    for (size_t i = 0; i < sent.size(); ++i)
    {
        EXPECT_NE(sent[i], received[i]) << "position " << i;
        EXPECT_TRUE(refersToSameFile(sent[i], received[i])) << "position " << i;
    }
}

TestFdNestedRepeated makeNestedRepeated(const std::vector<int> &fds)
{
    TestFdNestedRepeated message;
    for (size_t i = 0; i < fds.size(); ++i)
    {
        auto *holder = message.add_holders();
        holder->set_id(static_cast<int32_t>(i));
        holder->set_fd(fds[i]);
    }
    return message;
}

std::vector<int> fdsOf(const TestFdNestedRepeated &message)
{
    std::vector<int> fds;
    for (int i = 0; i < message.holders_size(); ++i)
    {
        EXPECT_EQ(message.holders(i).id(), i);
        fds.push_back(message.holders(i).fd());
    }
    return fds;
}

TestFdRepeatedField makeRepeatedField(const std::vector<int> &fds)
{
    TestFdRepeatedField message;
    for (int fd : fds)
        message.add_fds(fd);
    return message;
}

std::vector<int> fdsOf(const TestFdRepeatedField &message)
{
    return std::vector<int>(message.fds().begin(), message.fds().end());
}

// Expects four descriptors: first_fd, two nested holders, last_fd.
TestFdMixed makeMixed(const std::vector<int> &fds)
{
    TestFdMixed message;
    message.set_first_fd(fds[0]);
    message.add_holders()->set_fd(fds[1]);
    message.add_holders()->set_fd(fds[2]);
    message.set_last_fd(fds[3]);
    return message;
}

std::vector<int> fdsOf(const TestFdMixed &message)
{
    std::vector<int> fds{message.first_fd()};
    for (const auto &holder : message.holders())
        fds.push_back(holder.fd());
    fds.push_back(message.last_fd());
    return fds;
}
} // namespace

class RialtoIpcFdTest : public RialtoIpcTest
{
protected:
    // Sends \a sentMessage as a request; the handler checks the received descriptors while they are open.
    template <typename Message>
    void testRequest(void (TestModule_Stub::*method)(google::protobuf::RpcController *, const Message *, TestNoVar *,
                                                     google::protobuf::Closure *),
                     const std::vector<int> &sent, const Message &sentMessage)
    {
        bool handled{false};
        auto handler =
            [&](google::protobuf::RpcController *, const Message *request, TestNoVar *, google::protobuf::Closure *done)
        {
            expectSameFilesInOrder(sent, fdsOf(*request));
            handled = true;
            done->Run();
        };
        expectRequest(handler, sentMessage);

        TestNoVar response;
        EXPECT_TRUE(m_clientStub->call(method, sentMessage, response));
        EXPECT_TRUE(handled);
    }

    // Sends \a sentMessage as a reply and returns the received descriptors, which the caller owns.
    template <typename Message>
    std::vector<int> testReply(void (TestModule_Stub::*method)(google::protobuf::RpcController *, const TestNoVar *,
                                                               Message *, google::protobuf::Closure *),
                               const Message &sentMessage)
    {
        expectResponse(sentMessage);

        TestNoVar request;
        Message response;
        EXPECT_TRUE(m_clientStub->call(method, request, response));
        return fdsOf(response);
    }

    // Sends \a sentMessage as an event and returns the received descriptors, which the caller owns.
    template <typename Message> std::vector<int> testEvent(const Message &sentMessage)
    {
        auto event =
            m_clientStub
                ->receiveEvent<Message>([&]()
                                        { EXPECT_TRUE(m_serverStub->sendEvent(std::make_shared<Message>(sentMessage))); },
                                        kEventTimeoutMs);
        EXPECT_NE(event, nullptr);
        return event ? fdsOf(*event) : std::vector<int>{};
    }

private:
    template <typename Handler> void expectRequest(Handler handler, const TestFdNestedRepeated &)
    {
        EXPECT_CALL(*m_testModuleMock, TestFdNestedRepeatedRequest(_, _, _, _)).WillOnce(Invoke(handler));
    }
    template <typename Handler> void expectRequest(Handler handler, const TestFdRepeatedField &)
    {
        EXPECT_CALL(*m_testModuleMock, TestFdRepeatedFieldRequest(_, _, _, _)).WillOnce(Invoke(handler));
    }
    template <typename Handler> void expectRequest(Handler handler, const TestFdMixed &)
    {
        EXPECT_CALL(*m_testModuleMock, TestFdMixedRequest(_, _, _, _)).WillOnce(Invoke(handler));
    }

    void expectResponse(const TestFdNestedRepeated &message)
    {
        EXPECT_CALL(*m_testModuleMock, TestFdNestedRepeatedResponse(_, _, _, _))
            .WillOnce(DoAll(SetArgPointee<2>(message),
                            WithArgs<0, 3>(Invoke(&(*m_testModuleMock), &TestModuleMock::defaultReturn))));
    }
    void expectResponse(const TestFdRepeatedField &message)
    {
        EXPECT_CALL(*m_testModuleMock, TestFdRepeatedFieldResponse(_, _, _, _))
            .WillOnce(DoAll(SetArgPointee<2>(message),
                            WithArgs<0, 3>(Invoke(&(*m_testModuleMock), &TestModuleMock::defaultReturn))));
    }
    void expectResponse(const TestFdMixed &message)
    {
        EXPECT_CALL(*m_testModuleMock, TestFdMixedResponse(_, _, _, _))
            .WillOnce(DoAll(SetArgPointee<2>(message),
                            WithArgs<0, 3>(Invoke(&(*m_testModuleMock), &TestModuleMock::defaultReturn))));
    }
};

/**
 * Requests carry nested, repeated and mixed FD fields at the right positions.
 */
TEST_F(RialtoIpcFdTest, ShouldTransferNestedRepeatedFdsInRequests)
{
    for (size_t count : {0U, 1U, 3U})
    {
        const auto kSent = createFiles(count);
        testRequest(&TestModule_Stub::TestFdNestedRepeatedRequest, kSent, makeNestedRepeated(kSent));
        closeAll(kSent);
    }
}

TEST_F(RialtoIpcFdTest, ShouldTransferRepeatedFdFieldInRequest)
{
    const auto kSent = createFiles(2);
    testRequest(&TestModule_Stub::TestFdRepeatedFieldRequest, kSent, makeRepeatedField(kSent));
    closeAll(kSent);
}

TEST_F(RialtoIpcFdTest, ShouldTransferMixedFdsInRequestInOrder)
{
    const auto kSent = createFiles(4);
    testRequest(&TestModule_Stub::TestFdMixedRequest, kSent, makeMixed(kSent));
    closeAll(kSent);
}

/**
 * Replies carry nested, repeated and mixed FD fields at the right positions.
 */
TEST_F(RialtoIpcFdTest, ShouldTransferNestedRepeatedFdsInReplies)
{
    for (size_t count : {0U, 1U, 3U})
    {
        const auto kSent = createFiles(count);
        const auto kReceived = testReply(&TestModule_Stub::TestFdNestedRepeatedResponse, makeNestedRepeated(kSent));
        expectSameFilesInOrder(kSent, kReceived);
        closeAll(kReceived);
        closeAll(kSent);
    }
}

TEST_F(RialtoIpcFdTest, ShouldTransferRepeatedFdFieldInReply)
{
    const auto kSent = createFiles(2);
    const auto kReceived = testReply(&TestModule_Stub::TestFdRepeatedFieldResponse, makeRepeatedField(kSent));
    expectSameFilesInOrder(kSent, kReceived);
    closeAll(kReceived);
    closeAll(kSent);
}

TEST_F(RialtoIpcFdTest, ShouldTransferMixedFdsInReplyInOrder)
{
    const auto kSent = createFiles(4);
    const auto kReceived = testReply(&TestModule_Stub::TestFdMixedResponse, makeMixed(kSent));
    expectSameFilesInOrder(kSent, kReceived);
    closeAll(kReceived);
    closeAll(kSent);
}

/**
 * Events carry nested, repeated and mixed FD fields at the right positions.
 */
TEST_F(RialtoIpcFdTest, ShouldTransferNestedRepeatedFdsInEvents)
{
    for (size_t count : {0U, 1U, 3U})
    {
        const auto kSent = createFiles(count);
        const auto kReceived = testEvent(makeNestedRepeated(kSent));
        expectSameFilesInOrder(kSent, kReceived);
        closeAll(kReceived);
        closeAll(kSent);
    }
}

TEST_F(RialtoIpcFdTest, ShouldTransferRepeatedFdFieldInEvent)
{
    const auto kSent = createFiles(2);
    const auto kReceived = testEvent(makeRepeatedField(kSent));
    expectSameFilesInOrder(kSent, kReceived);
    closeAll(kReceived);
    closeAll(kSent);
}

TEST_F(RialtoIpcFdTest, ShouldTransferMixedFdsInEventInOrder)
{
    const auto kSent = createFiles(4);
    const auto kReceived = testEvent(makeMixed(kSent));
    expectSameFilesInOrder(kSent, kReceived);
    closeAll(kReceived);
    closeAll(kSent);
}

// -----------------------------------------------------------------------------
// Per-message FD limits on the sending side (vfc-ipc-nested-fd-transfer, task 3.2)
// -----------------------------------------------------------------------------

/**
 * A request with more FDs than the server accepts fails locally and is not sent.
 */
TEST_F(RialtoIpcFdTest, ShouldFailRequestOverFdLimitWithoutSending)
{
    const auto kSent = createFiles(17); // server accepts 16
    TestNoVar response;

    // StrictMock: any call to the handler fails the test
    EXPECT_FALSE(m_clientStub->call(&TestModule_Stub::TestFdRepeatedFieldRequest, makeRepeatedField(kSent), response));
    closeAll(kSent);
}

/**
 * A reply with more FDs than the client accepts is replaced by an error reply.
 */
TEST_F(RialtoIpcFdTest, ShouldReplaceReplyOverFdLimitWithError)
{
    const auto kSent = createFiles(33); // client accepts 32
    EXPECT_CALL(*m_testModuleMock, TestFdRepeatedFieldResponse(_, _, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(makeRepeatedField(kSent)),
                        WithArgs<0, 3>(Invoke(&(*m_testModuleMock), &TestModuleMock::defaultReturn))));

    TestNoVar request;
    TestFdRepeatedField response;
    EXPECT_FALSE(m_clientStub->call(&TestModule_Stub::TestFdRepeatedFieldResponse, request, response));
    EXPECT_EQ(response.fds_size(), 0);
    closeAll(kSent);
}

/**
 * An event with more FDs than the client accepts is not sent, and sending reports failure.
 */
TEST_F(RialtoIpcFdTest, ShouldNotSendEventOverFdLimit)
{
    constexpr int kShortTimeoutMs{100};
    const auto kSent = createFiles(33); // client accepts 32
    bool sent{true};

    auto event = m_clientStub->receiveEvent<TestFdRepeatedField>(
        [&]() { sent = m_serverStub->sendEvent(std::make_shared<TestFdRepeatedField>(makeRepeatedField(kSent))); },
        kShortTimeoutMs);

    EXPECT_FALSE(sent);
    EXPECT_EQ(event, nullptr);
    closeAll(kSent);
}

// -----------------------------------------------------------------------------
// Owned reply FDs (vfc-ipc-nested-fd-transfer, task 4.4)
// -----------------------------------------------------------------------------

namespace
{
constexpr char kOwnedFileName[]{"rialto-owned-reply"};

// Number of descriptors in this process that refer to a memfd created with \a name.
size_t countFilesNamed(const char *name)
{
    const std::string kTarget = std::string("/memfd:") + name;
    size_t count{0};
    DIR *dir = opendir("/proc/self/fd");
    if (!dir)
        return 0;
    while (struct dirent *entry = readdir(dir))
    {
        char link[256]{};
        const std::string kPath = std::string("/proc/self/fd/") + entry->d_name;
        if (readlink(kPath.c_str(), link, sizeof(link) - 1) > 0 && std::string(link).rfind(kTarget, 0) == 0)
            ++count;
    }
    closedir(dir);
    return count;
}

// The server closes owned descriptors just after the reply is sent, so allow it a moment.
bool waitForFilesNamed(const char *name, size_t expected)
{
    for (int i = 0; i < 500 && countFilesNamed(name) != expected; ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    return countFilesNamed(name) == expected;
}

struct FileId
{
    dev_t dev;
    ino_t ino;
};

std::vector<FileId> fileIdsOf(const std::vector<int> &fds)
{
    std::vector<FileId> ids;
    for (int fd : fds)
    {
        struct stat st
        {
        };
        EXPECT_EQ(fstat(fd, &st), 0);
        ids.push_back({st.st_dev, st.st_ino});
    }
    return ids;
}

void expectFileIds(const std::vector<FileId> &expected, const std::vector<int> &fds)
{
    const auto kActual = fileIdsOf(fds);
    ASSERT_EQ(expected.size(), kActual.size());
    for (size_t i = 0; i < expected.size(); ++i)
    {
        EXPECT_EQ(expected[i].dev, kActual[i].dev) << "position " << i;
        EXPECT_EQ(expected[i].ino, kActual[i].ino) << "position " << i;
    }
}

IController &ipcController(google::protobuf::RpcController *controller)
{
    return *dynamic_cast<IController *>(controller);
}
} // namespace

class RialtoIpcOwnedFdTest : public RialtoIpcFdTest
{
protected:
    // Replies with \a sent in a repeated FD field; \a configure runs in the handler before done->Run().
    bool replyWith(const std::vector<int> &sent, const std::function<void(IController &)> &configure,
                   std::vector<int> &received)
    {
        EXPECT_CALL(*m_testModuleMock, TestFdRepeatedFieldResponse(_, _, _, _))
            .WillOnce(Invoke(
                [&](google::protobuf::RpcController *controller, const TestNoVar *, TestFdRepeatedField *response,
                    google::protobuf::Closure *done)
                {
                    *response = makeRepeatedField(sent);
                    configure(ipcController(controller));
                    done->Run();
                }));

        TestNoVar request;
        TestFdRepeatedField response;
        const bool kResult = m_clientStub->call(&TestModule_Stub::TestFdRepeatedFieldResponse, request, response);
        received = fdsOf(response);
        return kResult;
    }
};

TEST_F(RialtoIpcOwnedFdTest, ShouldCloseOwnedFdsAfterSuccessfulReply)
{
    const auto kSent = createFiles(2, kOwnedFileName);
    const auto kSentIds = fileIdsOf(kSent);
    std::vector<int> received;

    EXPECT_TRUE(replyWith(kSent, [](IController &controller) { controller.setReplyFdsOwned(true); }, received));
    expectFileIds(kSentIds, received);

    EXPECT_TRUE(waitForFilesNamed(kOwnedFileName, received.size())); // only the client's copies remain
    closeAll(received);
    EXPECT_EQ(countFilesNamed(kOwnedFileName), 0U);
}

TEST_F(RialtoIpcOwnedFdTest, ShouldCloseOwnedFdsWhenCallFailsAfterFillingThem)
{
    const auto kSent = createFiles(2, kOwnedFileName);
    std::vector<int> received;

    EXPECT_FALSE(replyWith(
        kSent,
        [](IController &controller)
        {
            controller.setReplyFdsOwned(true);
            controller.SetFailed("failed after filling the reply");
        },
        received));
    EXPECT_TRUE(received.empty());

    EXPECT_TRUE(waitForFilesNamed(kOwnedFileName, 0));
}

TEST_F(RialtoIpcOwnedFdTest, ShouldCloseOwnedFdsWhenReplyIsOverTheLimit)
{
    const auto kSent = createFiles(33, kOwnedFileName); // client accepts 32
    std::vector<int> received;

    EXPECT_FALSE(replyWith(kSent, [](IController &controller) { controller.setReplyFdsOwned(true); }, received));
    EXPECT_TRUE(received.empty());

    EXPECT_TRUE(waitForFilesNamed(kOwnedFileName, 0));
}

TEST_F(RialtoIpcOwnedFdTest, ShouldLeaveUnownedFdsOpen)
{
    const auto kSent = createFiles(1, kOwnedFileName);
    std::vector<int> received;

    EXPECT_TRUE(replyWith(kSent, [](IController &) {}, received));
    expectSameFilesInOrder(kSent, received); // kSent is still open, so this can fstat it

    closeAll(received);
    EXPECT_EQ(countFilesNamed(kOwnedFileName), kSent.size());
    closeAll(kSent);
}

/**
 * A real server driven from the test thread, with the client end of a socket pair written directly,
 * so a test can complete a call after the client has gone.
 */
class RialtoIpcRawServerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ASSERT_EQ(socketpair(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0, m_sockets), 0);
        m_server = IServerFactory::createFactory()->create();
        ASSERT_TRUE(m_server);
        m_client = m_server->addClient(m_sockets[0]);
        ASSERT_TRUE(m_client);
        m_client->exportService(m_moduleMock);
    }

    void TearDown() override
    {
        for (int sock : m_sockets)
        {
            if (sock >= 0)
                close(sock);
        }
    }

    // Sends a TestFdRepeatedFieldResponse call from the raw client end.
    void sendCall()
    {
        transport::MessageToServer message;
        auto *call = message.mutable_call();
        call->set_serial_id(1);
        call->set_service_name(TestModule::descriptor()->full_name());
        call->set_method_name("TestFdRepeatedFieldResponse");
        call->set_request_message(TestNoVar().SerializeAsString());
        const std::string kData = message.SerializeAsString();
        ASSERT_EQ(send(m_sockets[1], kData.data(), kData.size(), MSG_NOSIGNAL), static_cast<ssize_t>(kData.size()));
    }

    void processServerUntil(const std::function<bool()> &condition)
    {
        for (int i = 0; i < 500 && !condition(); ++i)
        {
            m_server->process();
            m_server->wait(1);
        }
        EXPECT_TRUE(condition());
    }

    // Closes the client end and lets the server notice.
    void disconnectClient()
    {
        close(m_sockets[1]);
        m_sockets[1] = -1;
        processServerUntil([this]() { return !m_client->isConnected(); });
    }

    std::shared_ptr<StrictMock<TestModuleMock>> m_moduleMock{std::make_shared<StrictMock<TestModuleMock>>()};
    std::shared_ptr<IServer> m_server;
    std::shared_ptr<IClient> m_client;
    int m_sockets[2]{-1, -1};
};

TEST_F(RialtoIpcRawServerTest, ShouldCloseOwnedFdsWhenClientIsGone)
{
    const auto kSent = createFiles(2, kOwnedFileName);
    google::protobuf::Closure *pendingDone{nullptr};
    EXPECT_CALL(*m_moduleMock, TestFdRepeatedFieldResponse(_, _, _, _))
        .WillOnce(Invoke(
            [&](google::protobuf::RpcController *controller, const TestNoVar *, TestFdRepeatedField *response,
                google::protobuf::Closure *done)
            {
                *response = makeRepeatedField(kSent);
                ipcController(controller).setReplyFdsOwned(true);
                pendingDone = done; // completed after the client has gone
            }));

    sendCall();
    processServerUntil([&]() { return pendingDone != nullptr; });
    disconnectClient();

    ASSERT_NE(pendingDone, nullptr);
    pendingDone->Run();
    EXPECT_EQ(countFilesNamed(kOwnedFileName), 0U);
}

// -----------------------------------------------------------------------------
// Reply delivery callback (vfc-ipc-nested-fd-transfer, task 4.5)
// -----------------------------------------------------------------------------

namespace
{
// Records every delivery callback invocation; the server may invoke it just after the client sees the reply.
class DeliveryRecorder
{
public:
    std::function<void(bool)> callback()
    {
        return [this](bool delivered)
        {
            std::lock_guard<std::mutex> lock{m_mutex};
            m_results.push_back(delivered);
        };
    }

    std::vector<bool> waitForResults(size_t count)
    {
        for (int i = 0; i < 500 && results().size() < count; ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return results();
    }

    std::vector<bool> results()
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        return m_results;
    }

private:
    std::mutex m_mutex;
    std::vector<bool> m_results;
};
} // namespace

TEST_F(RialtoIpcOwnedFdTest, ShouldReportDeliveredOnceOnSuccess)
{
    const auto kSent = createFiles(1);
    DeliveryRecorder recorder;
    std::vector<int> received;

    EXPECT_TRUE(
        replyWith(kSent, [&](IController &controller) { controller.setDeliveryCallback(recorder.callback()); }, received));

    EXPECT_EQ(recorder.waitForResults(1), std::vector<bool>{true});
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_EQ(recorder.results().size(), 1U); // exactly once
    closeAll(received);
    closeAll(kSent);
}

TEST_F(RialtoIpcOwnedFdTest, ShouldReportNotDeliveredForErrorReply)
{
    const auto kSent = createFiles(1);
    DeliveryRecorder recorder;
    std::vector<int> received;

    EXPECT_FALSE(replyWith(
        kSent,
        [&](IController &controller)
        {
            controller.setDeliveryCallback(recorder.callback());
            controller.SetFailed("failed");
        },
        received));

    EXPECT_EQ(recorder.waitForResults(1), std::vector<bool>{false});
    closeAll(kSent);
}

TEST_F(RialtoIpcOwnedFdTest, ShouldSendReplyUnchangedWithoutCallback)
{
    const auto kSent = createFiles(2);
    std::vector<int> received;

    EXPECT_TRUE(replyWith(kSent, [](IController &) {}, received));
    expectSameFilesInOrder(kSent, received);
    closeAll(received);
    closeAll(kSent);
}

TEST_F(RialtoIpcOwnedFdTest, ShouldAllowCallbackToSendToClientAgain)
{
    const auto kSent = createFiles(1);
    std::vector<int> received;
    std::atomic<bool> callbackSent{false};

    auto event = m_clientStub->receiveEvent<TestEventSingleVar>(
        [&]()
        {
            EXPECT_TRUE(replyWith(
                kSent,
                [&](IController &controller)
                {
                    std::shared_ptr<IClient> client = controller.getClient();
                    controller.setDeliveryCallback(
                        [client, &callbackSent](bool delivered)
                        {
                            // would deadlock if the client lock were still held
                            auto followUp = std::make_shared<TestEventSingleVar>();
                            followUp->set_var1(delivered ? 1 : 0);
                            callbackSent = client->sendEvent(followUp);
                        });
                },
                received));
        },
        kEventTimeoutMs);

    ASSERT_NE(event, nullptr);
    EXPECT_EQ(event->var1(), 1);
    EXPECT_TRUE(callbackSent);
    closeAll(received);
    closeAll(kSent);
}

TEST_F(RialtoIpcRawServerTest, ShouldReportNotDeliveredWhenClientIsGone)
{
    DeliveryRecorder recorder;
    google::protobuf::Closure *pendingDone{nullptr};
    EXPECT_CALL(*m_moduleMock, TestFdRepeatedFieldResponse(_, _, _, _))
        .WillOnce(Invoke(
            [&](google::protobuf::RpcController *controller, const TestNoVar *, TestFdRepeatedField *,
                google::protobuf::Closure *done)
            {
                ipcController(controller).setDeliveryCallback(recorder.callback());
                pendingDone = done;
            }));

    sendCall();
    processServerUntil([&]() { return pendingDone != nullptr; });
    disconnectClient();

    ASSERT_NE(pendingDone, nullptr);
    pendingDone->Run();
    EXPECT_EQ(recorder.results(), std::vector<bool>{false});
}

TEST_F(RialtoIpcRawServerTest, ShouldReportNotDeliveredWhenSendFails)
{
    DeliveryRecorder recorder;
    google::protobuf::Closure *pendingDone{nullptr};
    EXPECT_CALL(*m_moduleMock, TestFdRepeatedFieldResponse(_, _, _, _))
        .WillOnce(Invoke(
            [&](google::protobuf::RpcController *controller, const TestNoVar *, TestFdRepeatedField *,
                google::protobuf::Closure *done)
            {
                ipcController(controller).setDeliveryCallback(recorder.callback());
                pendingDone = done;
            }));

    sendCall();
    processServerUntil([&]() { return pendingDone != nullptr; });

    // close the peer without letting the server notice: the client is still registered, but sendmsg fails
    close(m_sockets[1]);
    m_sockets[1] = -1;

    ASSERT_NE(pendingDone, nullptr);
    pendingDone->Run();
    EXPECT_EQ(recorder.results(), std::vector<bool>{false});
}
