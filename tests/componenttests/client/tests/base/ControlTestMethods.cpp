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
#include <memory>
#include <cstring>
#include <fcntl.h>
#include <numeric>
#include <stdexcept>
#include <sys/mman.h>
#include <syscall.h>
#include <unistd.h>

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
constexpr int32_t kControlId = 1;
} // namespace

ControlTestMethods::ControlTestMethods()
    : m_controlClientMock{std::make_shared<StrictMock<ControlClientMock>>()},
      m_controlModuleMock{std::make_shared<StrictMock<ControlModuleMock>>()}
{
    initRealShm();
}

ControlTestMethods::~ControlTestMethods()
{
    termRealShm();
}

void ControlTestMethods::createControl()
{
    std::cout<<"createControl 1"<<std::endl;
    m_controlFactory = firebolt::rialto::IControlFactory::createFactory();
    m_control = m_controlFactory->createControl();
    EXPECT_NE(m_control, nullptr);
    std::cout<<"createControl 2"<<std::endl;
}

void ControlTestMethods::shouldRegisterClient()
{
    const firebolt::rialto::common::SchemaVersion kSchemaVersion = firebolt::rialto::common::getCurrentSchemaVersion();
    EXPECT_CALL(*m_controlModuleMock, registerClient(_, _, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_controlModuleMock->getRegisterClientResponse(kControlId, kSchemaVersion)),
                        WithArgs<0, 3>(Invoke(&(*m_controlModuleMock), &ControlModuleMock::defaultReturn))));
}

void ControlTestMethods::registerClient()
{
    std::cout<<"registerClient 1"<<std::endl;
    ApplicationState appState;
    EXPECT_TRUE(m_control->registerClient(m_controlClientMock, appState));
    EXPECT_EQ(ApplicationState::UNKNOWN, appState);
    std::cout<<"registerClient 2"<<std::endl;
}

void ControlTestMethods::shouldNotifyApplicationStateInactive()
{
    EXPECT_CALL(*m_controlClientMock, notifyApplicationState(ApplicationState::INACTIVE))
        .WillOnce(Invoke(this, &ControlTestMethods::notifyEvent));
}

void ControlTestMethods::sendNotifyApplicationStateInactive()
{
    getServerStub()->notifyApplicationStateEvent(kControlId, ApplicationState::INACTIVE);
    waitEvent();
}

void ControlTestMethods::shouldNotifyApplicationStateUnknown()
{
    EXPECT_CALL(*m_controlClientMock, notifyApplicationState(ApplicationState::UNKNOWN))
        .WillOnce(Invoke(this, &ControlTestMethods::notifyEvent));
}

void ControlTestMethods::shouldNotifyApplicationStateRunning()
{
    EXPECT_CALL(*m_controlClientMock, notifyApplicationState(ApplicationState::RUNNING))
        .WillOnce(Invoke(this, &ControlTestMethods::notifyEvent));
    EXPECT_CALL(*m_controlModuleMock, getSharedMemory(_, _, _, _))
        .WillOnce(DoAll(SetArgPointee<2>(m_controlModuleMock->getSharedMemoryResponse(m_fd, m_size)),
                        WithArgs<0, 3>(Invoke(&(*m_controlModuleMock), &ControlModuleMock::defaultReturn))));
}

void ControlTestMethods::sendNotifyApplicationStateRunning()
{
    getServerStub()->notifyApplicationStateEvent(kControlId, ApplicationState::RUNNING);
    waitEvent();
}

void ControlTestMethods::initRealShm()
{
    std::cout<<m_fd<<std::endl;
    int fd = syscall(SYS_memfd_create, "rialto_avbuf", MFD_CLOEXEC | MFD_ALLOW_SEALING);
    ASSERT_GT(fd, 0);
    
    ASSERT_NE(ftruncate(fd, static_cast<off_t>(m_size)), -1);
    ASSERT_NE(fcntl(fd, F_ADD_SEALS, (F_SEAL_SEAL | F_SEAL_GROW | F_SEAL_SHRINK)), -1);

    void *addr = mmap(nullptr, m_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    ASSERT_NE(addr, MAP_FAILED);

    m_fd = fd;
    m_address = addr;
    std::cout<<m_fd<<std::endl;
}

void ControlTestMethods::termRealShm()
{
    ASSERT_EQ(munmap(m_address, m_size), 0);
    std::cout<<m_fd<<std::endl;
    close(m_fd);
    //ASSERT_EQ(close(m_fd), 0);
    std::cout<<strerror(errno)<<std::endl;
}
