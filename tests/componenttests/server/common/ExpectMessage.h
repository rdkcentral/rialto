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

#ifndef FIREBOLT_RIALTO_SERVER_CT_EXPECT_MESSAGE_H_
#define FIREBOLT_RIALTO_SERVER_CT_EXPECT_MESSAGE_H_

#include "IIpcChannel.h"
#include "IStub.h"
#include <condition_variable>
#include <functional>
#include <gtest/gtest.h>
#include <memory>
#include <mutex>

namespace firebolt::rialto::server::ct
{
template <typename MessageType> class ExpectMessage
{
public:
    ExpectMessage(IStub &stub) : m_channel{stub.getChannel()}
    {
        if (!m_channel)
        {
            EXPECT_TRUE(m_channel); // assert not possible in constructor, just to fail test and not crash
            return;
        }
        m_subscriptionTag =
            m_channel->subscribe<MessageType>(std::bind(&ExpectMessage::onEvent, this, std::placeholders::_1));
    }

    ~ExpectMessage()
    {
        if (!m_channel)
        {
            EXPECT_TRUE(m_channel); // assert not possible in destructor, just to fail test and not crash
            return;
        }
        m_channel->unsubscribe(m_subscriptionTag);
    }

    std::shared_ptr<MessageType> getMessage()
    {
        std::unique_lock lock{m_messageMutex};
        m_messageCv.wait_for(lock, std::chrono::milliseconds(200), [&]() { return static_cast<bool>(m_message); });
        return m_message;
    }

private:
    void onEvent(const std::shared_ptr<MessageType> &event)
    {
        std::unique_lock lock{m_messageMutex};
        m_message = event;
        m_messageCv.notify_one();
    }

private:
    std::mutex m_messageMutex;
    std::condition_variable m_messageCv;
    int m_subscriptionTag{-1};
    std::shared_ptr<::firebolt::rialto::ipc::IChannel> m_channel{nullptr};
    std::shared_ptr<MessageType> m_message{nullptr};
};
} // namespace firebolt::rialto::server::ct

#endif // FIREBOLT_RIALTO_SERVER_CT_EXPECT_MESSAGE_H_
