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

#include "EventRanger.h"
#include "IIpcChannel.h"
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
    explicit ExpectMessage(EventRanger &eventRanger) : m_eventRanger{eventRanger}
    {
        m_subscriptionTag =
            m_eventRanger.addExpectation<MessageType>(std::bind(&ExpectMessage::onEvent, this, std::placeholders::_1));
    }

    ~ExpectMessage() { m_eventRanger.removeExpectation(m_subscriptionTag); }

    std::shared_ptr<MessageType> getMessage()
    {
        std::unique_lock lock{m_messageMutex};
        m_messageCv.wait_for(lock, m_timeout, [&]() { return static_cast<bool>(m_message); });
        return m_message;
    }

    void setFilter(const std::function<bool(const MessageType &)> &filter) { m_filter = filter; }

    void setTimeout(const std::chrono::milliseconds &timeout) { m_timeout = timeout; }

private:
    void onEvent(const std::shared_ptr<MessageType> &event)
    {
        if (m_filter(*event))
        {
            std::unique_lock lock{m_messageMutex};
            m_message = event;
            m_messageCv.notify_one();
        }
    }

private:
    std::mutex m_messageMutex;
    std::condition_variable m_messageCv;
    int m_subscriptionTag{-1};
    EventRanger &m_eventRanger;
    std::shared_ptr<MessageType> m_message{nullptr};
    std::function<bool(const MessageType &)> m_filter{[](const MessageType &) { return true; }};
    std::chrono::milliseconds m_timeout{400};
};
} // namespace firebolt::rialto::server::ct

#endif // FIREBOLT_RIALTO_SERVER_CT_EXPECT_MESSAGE_H_
