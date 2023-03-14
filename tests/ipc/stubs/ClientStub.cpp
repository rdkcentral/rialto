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
#include "TestClientMock.h"
#include <IIpcChannel.h>
#include <IIpcControllerFactory.h>
#include <atomic>
#include <gtest/gtest.h>

namespace
{
void onMessageReceived(std::atomic_bool *done)
{
    done->store(true);
}
}; // namespace

ClientStub::ClientStub(const std::shared_ptr<firebolt::rialto::ipc::TestClientMock> &clientMock,
                       const std::string &socketName)
    : m_socketName{socketName}, m_clientMock{clientMock}, m_messageReceived{false}
{
}

ClientStub::~ClientStub()
{
    if (m_channel && m_channel->isConnected())
    {
        m_channel->disconnect();
    }

    if (m_eventThread.joinable())
    {
        m_eventThread.join();
    }
}

bool ClientStub::connect()
{
    auto factory = firebolt::rialto::ipc::IChannelFactory::createFactory();
    m_channel = factory->createChannel(m_socketName);
    if (!m_channel)
    {
        return false;
    }
    m_testModuleStub = std::make_unique<firebolt::rialto::TestModule::Stub>(m_channel.get());
    int eventTag = m_channel->subscribe<firebolt::rialto::TestEventSingleVar>(
        [this](const std::shared_ptr<firebolt::rialto::TestEventSingleVar> &event)
        { onTestEventSingleVarReceived(event); });
    EXPECT_GE(eventTag, 0);
    m_eventTags.push_back(eventTag);

    eventTag = m_channel->subscribe<firebolt::rialto::TestEventMultiVar>(
        [this](const std::shared_ptr<firebolt::rialto::TestEventMultiVar> &event)
        { onTestEventMultiVarReceived(event); });
    EXPECT_GE(eventTag, 0);
    m_eventTags.push_back(eventTag);

    return true;
}

void ClientStub::disconnect()
{
    EXPECT_TRUE(m_channel);
    EXPECT_TRUE(m_channel->isConnected());

    for (auto it = m_eventTags.begin(); it != m_eventTags.end(); it++)
    {
        EXPECT_TRUE(m_channel->unsubscribe(*it));
    }
    m_channel->disconnect();
}

bool ClientStub::sendSingleVarRequest(int32_t var1)
{
    firebolt::rialto::TestSingleVar request;
    firebolt::rialto::TestNoVar response;

    request.set_var1(var1);

    auto controllerFactory = firebolt::rialto::ipc::IControllerFactory::createFactory();
    auto controller = controllerFactory->create();

    std::atomic_bool done{false};
    m_testModuleStub->TestRequestSingleVar(controller.get(), &request, &response,
                                           google::protobuf::NewCallback(onMessageReceived, &done));

    while (m_channel->process() && !done.load())
    {
        m_channel->wait(1);
    }

    if (controller->Failed())
    {
        return false;
    }

    return true;
}

bool ClientStub::sendMultiVarRequest(int32_t var1, uint32_t var2, firebolt::rialto::TestMultiVar_TestType var3,
                                     std::string var4)
{
    firebolt::rialto::TestMultiVar request;
    firebolt::rialto::TestNoVar response;

    request.set_var1(var1);
    request.set_var2(var2);
    request.set_var3(var3);
    request.set_var4(var4);

    auto controllerFactory = firebolt::rialto::ipc::IControllerFactory::createFactory();
    auto controller = controllerFactory->create();

    std::atomic_bool done{false};
    m_testModuleStub->TestRequestMultiVar(controller.get(), &request, &response,
                                          google::protobuf::NewCallback(onMessageReceived, &done));

    while (m_channel->process() && !done.load())
    {
        m_channel->wait(1);
    }

    if (controller->Failed())
    {
        return false;
    }

    return true;
}

bool ClientStub::sendRequestWithSingleVarResponse(int32_t &var1)
{
    firebolt::rialto::TestNoVar request;
    firebolt::rialto::TestSingleVar response;

    auto controllerFactory = firebolt::rialto::ipc::IControllerFactory::createFactory();
    auto controller = controllerFactory->create();

    std::atomic_bool done{false};
    m_testModuleStub->TestResponseSingleVar(controller.get(), &request, &response,
                                            google::protobuf::NewCallback(onMessageReceived, &done));

    while (m_channel->process() && !done.load())
    {
        m_channel->wait(1);
    }

    if (controller->Failed())
    {
        return false;
    }

    var1 = response.var1();

    return true;
}

bool ClientStub::sendRequestWithMultiVarResponse(int32_t &var1, uint32_t &var2,
                                                 firebolt::rialto::TestMultiVar_TestType &var3, std::string &var4)
{
    firebolt::rialto::TestNoVar request;
    firebolt::rialto::TestMultiVar response;

    auto controllerFactory = firebolt::rialto::ipc::IControllerFactory::createFactory();
    auto controller = controllerFactory->create();

    std::atomic_bool done{false};
    m_testModuleStub->TestResponseMultiVar(controller.get(), &request, &response,
                                           google::protobuf::NewCallback(onMessageReceived, &done));

    while (m_channel->process() && !done.load())
    {
        m_channel->wait(1);
    }

    if (controller->Failed())
    {
        return false;
    }

    var1 = response.var1();
    var2 = response.var2();
    var3 = response.var3();
    var4 = response.var4();

    return true;
}

void ClientStub::startMessageThread(bool expectMessage)
{
    m_eventThread = std::thread{[this, expectMessage]()
                                {
                                    while (m_channel->process() && !m_messageReceived.load())
                                    {
                                        m_startThreadCond.notify_one();
                                        m_channel->wait(10);
                                    }
                                    EXPECT_EQ(m_messageReceived.load(), expectMessage);
                                    m_messageCond.notify_one();
                                }};

    std::unique_lock<std::mutex> startThreadLock(m_startThreadMutex);
    std::cv_status status = m_startThreadCond.wait_for(startThreadLock, std::chrono::milliseconds(100));
    ASSERT_NE(std::cv_status::timeout, status);
}

void ClientStub::waitForSingleVarEvent(int32_t &var1)
{
    if (!m_messageReceived.load())
    {
        std::unique_lock<std::mutex> messageLock(m_messageMutex);
        std::cv_status status = m_messageCond.wait_for(messageLock, std::chrono::milliseconds(100));
        EXPECT_NE(std::cv_status::timeout, status);
    }
    ASSERT_NE(m_singleVarEvent, nullptr);

    var1 = m_singleVarEvent->var1();
}

void ClientStub::waitForMultiVarEvent(int32_t &var1, uint32_t &var2, firebolt::rialto::TestEventMultiVar_TestType &var3,
                                      std::string &var4)
{
    if (!m_messageReceived.load())
    {
        std::unique_lock<std::mutex> messageLock(m_messageMutex);
        std::cv_status status = m_messageCond.wait_for(messageLock, std::chrono::milliseconds(100));
        EXPECT_NE(std::cv_status::timeout, status);
    }
    ASSERT_NE(m_multiVarEvent, nullptr);

    var1 = m_multiVarEvent->var1();
    var2 = m_multiVarEvent->var2();
    var3 = m_multiVarEvent->var3();
    var4 = m_multiVarEvent->var4();
}

void ClientStub::onTestEventSingleVarReceived(const std::shared_ptr<firebolt::rialto::TestEventSingleVar> &event)
{
    m_singleVarEvent = event;
    m_messageReceived.store(true);
}

void ClientStub::onTestEventMultiVarReceived(const std::shared_ptr<firebolt::rialto::TestEventMultiVar> &event)
{
    m_multiVarEvent = event;
    m_messageReceived.store(true);
}
