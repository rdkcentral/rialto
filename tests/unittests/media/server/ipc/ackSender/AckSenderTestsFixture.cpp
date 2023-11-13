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

#include "AckSenderTestsFixture.h"

namespace
{
constexpr int kId{22};
constexpr bool kSuccess{true};
} // namespace

MATCHER(AckEventMatcher, "")
{
    std::shared_ptr<rialto::AckEvent> event = std::dynamic_pointer_cast<rialto::AckEvent>(arg);
    return (kSuccess == event->success() && kId == event->id());
}

AckSenderTests::AckSenderTests()
    : m_clientMock{std::make_shared<StrictMock<firebolt::rialto::ipc::ClientMock>>()}, m_sut{m_clientMock}
{
}

void AckSenderTests::clientWillSendAckEvent()
{
    EXPECT_CALL(*m_clientMock, sendEvent(AckEventMatcher()));
}

void AckSenderTests::triggerSend()
{
    m_sut.send(kId, kSuccess);
}
