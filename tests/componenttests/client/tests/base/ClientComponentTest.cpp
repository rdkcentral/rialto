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

#include "ClientComponentTest.h"
#include <memory>
#include <utility>
#include <linux/memfd.h>
#include <sys/mman.h>
#include <sys/syscall.h>

namespace
{
constexpr std::chrono::milliseconds kEventTimeout{200};
} // namespace

ClientComponentTest::ClientComponentTest()
    : m_serverStub{std::make_shared<ServerStub>(m_controlModuleMock)}
{
}

ClientComponentTest::~ClientComponentTest()
{
}

void ClientComponentTest::notifyEvent()
{
    std::unique_lock<std::mutex> locker(m_eventsLock);
    m_eventReceived = true;
    m_eventsCond.notify_all();
}

void ClientComponentTest::waitEvent()
{
    std::unique_lock<std::mutex> locker(m_eventsLock);
    if (!m_eventReceived)
    {
        bool status = m_eventsCond.wait_for(locker, kEventTimeout, [this]() { return m_eventReceived; });
        ASSERT_TRUE(status);
    }
    m_eventReceived = false;
}

std::shared_ptr<ServerStub>& ClientComponentTest::getServerStub()
{
    return m_serverStub;
}

void ClientComponentTest::disconnectServer()
{
    m_serverStub.reset();
}
