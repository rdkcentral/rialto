/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Sky UK
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

#ifndef FIREBOLT_RIALTO_SERVER_CT_EVENT_RANGER_H_
#define FIREBOLT_RIALTO_SERVER_CT_EVENT_RANGER_H_

#include <IIpcChannel.h>
#include <functional>
#include <gtest/gtest.h>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

namespace firebolt::rialto::server::ct
{
class EventRanger
{
protected:
    EventRanger() = default;
    ~EventRanger() = default;

    template <typename... Message>
    void setupSubscriptions(const std::shared_ptr<::firebolt::rialto::ipc::IChannel> &channel)
    {
        [](...) {}((m_eventTags.push_back(channel->subscribe<Message>(
                        [this](const auto &msg)
                        {
                            std::unique_lock lock{m_mutex};
                            auto callbacksIt{m_callbacks.find(Message::default_instance().GetTypeName())};
                            ASSERT_NE(m_callbacks.end(), callbacksIt)
                                << "Missing ExpectMessage for: " << Message::default_instance().GetTypeName();
                            EXPECT_FALSE(callbacksIt->second.empty())
                                << "Missing ExpectMessage for: " << Message::default_instance().GetTypeName();
                            for (const auto &cb : callbacksIt->second)
                            {
                                // Callback call (under lock) is safe here, because ExpectMessage doesn't add/remove
                                // expectation during cb call
                                cb.second(msg);
                            }
                        })),
                    0)...);
    }
    void teardownSubscriptions(const std::shared_ptr<::firebolt::rialto::ipc::IChannel> &channel);

public:
    using MessageCallback = std::function<void(const std::shared_ptr<google::protobuf::Message> &)>;
    template <typename Message>
    int addExpectation(const std::function<void(const std::shared_ptr<Message> &)> &callback)
    {
        static int expectationTag{0};
        std::unique_lock lock{m_mutex};
        m_callbacks[Message::default_instance().GetTypeName()]
            .emplace(expectationTag, [cb = std::move(callback)](const std::shared_ptr<google::protobuf::Message> &msg)
                     { cb(std::dynamic_pointer_cast<Message>(msg)); });
        return expectationTag++;
    }
    void removeExpectation(int expectationTag);

private:
    std::mutex m_mutex;
    std::vector<int> m_eventTags;
    std::unordered_map<std::string, std::unordered_map<int, MessageCallback>> m_callbacks;
};
} // namespace firebolt::rialto::server::ct

#endif // FIREBOLT_RIALTO_SERVER_CT_EVENT_RANGER_H_
