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

#include "ClientComponentTest.h"
#include <cstring>
#include <fcntl.h>
#include <linux/memfd.h>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <sys/mman.h>
#include <syscall.h>
#include <unistd.h>
#include <utility>

#if !defined(SYS_memfd_create)
#if defined(__NR_memfd_create)
#define SYS_memfd_create __NR_memfd_create
#elif defined(__arm__)
#define SYS_memfd_create 385
#endif
#endif

#if !defined(MFD_CLOEXEC)
#define MFD_CLOEXEC 0x0001U
#endif

#if !defined(MFD_ALLOW_SEALING)
#define MFD_ALLOW_SEALING 0x0002U
#endif

#if !defined(F_ADD_SEALS)
#if !defined(F_LINUX_SPECIFIC_BASE)
#define F_LINUX_SPECIFIC_BASE 1024
#endif
#define F_ADD_SEALS (F_LINUX_SPECIFIC_BASE + 9)
#define F_GET_SEALS (F_LINUX_SPECIFIC_BASE + 10)

#define F_SEAL_SEAL 0x0001
#define F_SEAL_SHRINK 0x0002
#define F_SEAL_GROW 0x0004
#define F_SEAL_WRITE 0x0008
#endif

namespace
{
constexpr std::chrono::milliseconds kEventTimeout{200};
constexpr uint32_t kSharedMemorySize{123000};
constexpr firebolt::rialto::MediaPlayerShmInfo kShmInfoVideo{1000, 0, 1000, 100000};
constexpr firebolt::rialto::MediaPlayerShmInfo kShmInfoAudio{1000, 101000, 102000, 10000};
constexpr firebolt::rialto::MediaPlayerShmInfo kShmInfoWebAudio{1000, 112000, 113000, 10000};
} // namespace

ClientComponentTest::ClientComponentTest()
    : MediaPipelineTestMethods(kShmInfoAudio, kShmInfoVideo),
      m_serverStub{std::make_shared<ServerStub>(m_controlModuleMock, m_mediaPipelineModuleMock)}
{
    initRealShm();
}

ClientComponentTest::~ClientComponentTest()
{
    termRealShm();
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

std::shared_ptr<ServerStub> &ClientComponentTest::getServerStub()
{
    return m_serverStub;
}

void ClientComponentTest::disconnectServer()
{
    m_serverStub.reset();
}

void ClientComponentTest::initRealShm()
{
    int fd = syscall(SYS_memfd_create, "rialto_avbuf", MFD_CLOEXEC | MFD_ALLOW_SEALING);
    ASSERT_GT(fd, 0);

    ASSERT_NE(ftruncate(fd, static_cast<off_t>(kSharedMemorySize)), -1);
    ASSERT_NE(fcntl(fd, F_ADD_SEALS, (F_SEAL_SEAL | F_SEAL_GROW | F_SEAL_SHRINK)), -1);

    void *addr = mmap(nullptr, kSharedMemorySize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ASSERT_NE(addr, MAP_FAILED);

    m_fd = fd;
    m_address = addr;
}

void ClientComponentTest::termRealShm()
{
    ASSERT_EQ(munmap(m_address, kSharedMemorySize), 0);
    close(m_fd);
}

int32_t ClientComponentTest::getShmFd()
{
    return m_fd;
}

void *ClientComponentTest::getShmAddress()
{
    return m_address;
}

uint32_t ClientComponentTest::getShmSize()
{
    return kSharedMemorySize;
}

void ClientComponentTest::startApplicationRunning()
{
    createControl();
    shouldRegisterClient();
    registerClient();
    shouldNotifyApplicationStateInactive();
    sendNotifyApplicationStateInactive();
    shouldNotifyApplicationStateRunning();
    sendNotifyApplicationStateRunning();
}

void ClientComponentTest::stopApplication()
{
    shouldNotifyApplicationStateUnknown();
    disconnectServer();
    waitEvent();
}
