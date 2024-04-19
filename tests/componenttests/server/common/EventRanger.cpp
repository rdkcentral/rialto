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

#include "EventRanger.h"
#include <gtest/gtest.h>
#include <memory>

namespace firebolt::rialto::server::ct
{
void EventRanger::teardownSubscriptions(const std::shared_ptr<::firebolt::rialto::ipc::IChannel> &channel)
{
    for (int tag : m_eventTags)
    {
        EXPECT_TRUE(channel->unsubscribe(tag));
    }
    m_eventTags.clear();
}

void EventRanger::removeExpectation(int expectationTag)
{
    std::unique_lock lock{m_mutex};
    for (auto &callbacks : m_callbacks)
    {
        if (0 != callbacks.second.erase(expectationTag))
        {
            return;
        }
    }
}

void EventRanger::removeSuppression(int expectationTag)
{
    removeExpectation(expectationTag);
}
} // namespace firebolt::rialto::server::ct
