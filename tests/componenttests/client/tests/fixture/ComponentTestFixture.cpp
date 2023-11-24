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

#include "ComponentTestFixture.h"
#include <memory>
#include <utility>
#include <linux/memfd.h>
#include <sys/mman.h>
#include <sys/syscall.h>

namespace
{
constexpr std::chrono::milliseconds kEventTimeout{200};
} // namespace

ComponentTestFixture::ComponentTestFixture()
    : m_serverStub{std::make_shared<ServerStub>(m_controlModuleMock)}
{
    // Create a valid file descriptor
    // Create a proper shared memory region if we are writing to this buffer
    m_fd = memfd_create("memfdfile", 0);
}

ComponentTestFixture::~ComponentTestFixture()
{
    close(m_fd);
}

void ComponentTestFixture::notifyEvent()
{
    std::unique_lock<std::mutex> locker(m_eventsLock);
    m_eventReceived = true;
    m_eventsCond.notify_all();
}

void ComponentTestFixture::waitEvent()
{
    std::unique_lock<std::mutex> locker(m_eventsLock);
    if (!m_eventReceived)
    {
        bool status = m_eventsCond.wait_for(locker, kEventTimeout, [this]() { return m_eventReceived; });
        ASSERT_TRUE(status);
    }
    m_eventReceived = false;
}

std::shared_ptr<ServerStub>& ComponentTestFixture::getServerStub()
{
    return m_serverStub;
}
