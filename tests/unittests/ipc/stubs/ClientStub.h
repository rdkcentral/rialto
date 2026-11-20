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

#ifndef CLIENT_STUB_H_
#define CLIENT_STUB_H_

#include "IIpcChannel.h"
#include "TestClientMock.h"
#include "testmodule.pb.h"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

class ClientStub
{
public:
    ClientStub(const std::shared_ptr<firebolt::rialto::ipc::TestClientMock> &clientMock, const std::string &socketName);
    ~ClientStub();

    bool connect();
    void disconnect();

    bool sendSingleVarRequest(int32_t var1);
    bool sendMultiVarRequest(int32_t var1, uint32_t var2, firebolt::rialto::TestMultiVar_TestType var3, std::string var4);
    bool sendSingleVarRequestWithNoReply(int32_t var1);
    bool sendRequestWithSingleVarResponse(int32_t &var1);
    bool sendRequestWithMultiVarResponse(int32_t &var1, uint32_t &var2, firebolt::rialto::TestMultiVar_TestType &var3,
                                         std::string &var4);

    /**
     * @brief Calls a TestModule method and processes the channel until it completes.
     *
     * @retval true if the call completed without failure.
     */
    template <typename Request, typename Response>
    bool call(void (firebolt::rialto::TestModule_Stub::*method)(google::protobuf::RpcController *, const Request *,
                                                                Response *, google::protobuf::Closure *),
              const Request &request, Response &response)
    {
        auto controller = createController();
        std::atomic_bool done{false};
        (m_testModuleStub.get()->*method)(controller.get(), &request, &response,
                                          google::protobuf::NewCallback(&ClientStub::onCallDone, &done));
        while (m_channel->process() && !done.load())
        {
            m_channel->wait(1);
        }
        return done.load() && !controller->Failed();
    }

    /**
     * @brief Subscribes to an event and processes the channel on this thread until it arrives.
     *
     * @retval the received event, or nullptr after \a timeoutMs.
     */
    template <typename Event> std::shared_ptr<Event> receiveEvent(const std::function<void()> &trigger, int timeoutMs)
    {
        std::shared_ptr<Event> received;
        const int kTag =
            m_channel->subscribe<Event>([&received](const std::shared_ptr<Event> &event) { received = event; });
        trigger();
        for (int waited = 0; !received && waited < timeoutMs && m_channel->process(); ++waited)
        {
            m_channel->wait(1);
        }
        m_channel->unsubscribe(kTag);
        return received;
    }

    void startMessageThread(bool expectMessage = true);
    void waitForSingleVarEvent(int32_t &var1);
    void waitForMultiVarEvent(int32_t &var1, uint32_t &var2, firebolt::rialto::TestEventMultiVar_TestType &var3,
                              std::string &var4);

private:
    static void onCallDone(std::atomic_bool *done);
    std::shared_ptr<google::protobuf::RpcController> createController();
    void onTestEventSingleVarReceived(const std::shared_ptr<firebolt::rialto::TestEventSingleVar> &event);
    void onTestEventMultiVarReceived(const std::shared_ptr<firebolt::rialto::TestEventMultiVar> &event);

private:
    std::string m_socketName;
    std::shared_ptr<firebolt::rialto::ipc::IChannel> m_channel;
    std::unique_ptr<firebolt::rialto::TestModule_Stub> m_testModuleStub;
    std::shared_ptr<firebolt::rialto::ipc::TestClientMock> m_clientMock;
    std::atomic<bool> m_messageReceived;
    std::thread m_eventThread;
    std::mutex m_messageMutex;
    std::condition_variable m_messageCond;
    std::mutex m_startThreadMutex;
    std::condition_variable m_startThreadCond;
    std::shared_ptr<firebolt::rialto::TestEventSingleVar> m_singleVarEvent;
    std::shared_ptr<firebolt::rialto::TestEventMultiVar> m_multiVarEvent;
    std::vector<int> m_eventTags;
};

#endif // CLIENT_STUB_H_
