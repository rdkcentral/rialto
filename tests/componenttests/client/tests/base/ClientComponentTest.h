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

#ifndef CLIENT_COMPONENT_TEST_H_
#define CLIENT_COMPONENT_TEST_H_

#include "ControlTestMethods.h"
#include "MediaPipelineTestMethods.h"
#include "ServerStub.h"
#include <gtest/gtest.h>
#include <memory>

using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::StrictMock;
using ::testing::WithArgs;

using namespace firebolt::rialto;
using namespace firebolt::rialto::ct::stub;

class ClientComponentTest : public ::testing::Test, public ControlTestMethods, public MediaPipelineTestMethods
{
public:
    ClientComponentTest();
    virtual ~ClientComponentTest();

protected:
    // Test Methods
    void disconnectServer();
    void startApplicationRunning();
    void stopApplication();

private:
    // Derived class helpers
    void notifyEvent() override;
    void waitEvent() override;
    std::shared_ptr<ServerStub> &getServerStub() override;
    int32_t getShmFd() override;
    void *getShmAddress() override;
    uint32_t getShmSize() override;

    // Server stub
    std::shared_ptr<ServerStub> m_serverStub;

    // Event variables
    std::mutex m_eventsLock;
    std::condition_variable m_eventsCond;
    bool m_eventReceived{false};

    // Shared Memory
    int32_t m_fd{-1};
    void *m_address{nullptr};
    void initRealShm();
    void termRealShm();
};

#endif // CLIENT_COMPONENT_TEST_H_
