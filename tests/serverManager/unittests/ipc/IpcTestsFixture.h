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

#ifndef IPC_TESTS_FIXTURE_H_
#define IPC_TESTS_FIXTURE_H_

#include "IController.h"
#include "RialtoSessionServerStub.h"
#include "SessionServerAppManagerMock.h"
#include <condition_variable>
#include <gtest/gtest.h>
#include <memory>
#include <mutex>

using testing::StrictMock;

class IpcTests : public testing::Test
{
public:
    IpcTests();
    virtual ~IpcTests() = default;

    void configureServerToSendOkResponses();
    void configureServerToSendFailResponses();

    void simulateStateChangedEventInactive();
    void simulateAckEvent();
    void simulateClientDisconnection();

    void sessionServerAppManagerWillBeNotifiedAboutSessionServerStateChange(
        const firebolt::rialto::common::SessionServerState &newState);
    void sessionServerAppManagerWillBeNotifiedAboutCompletedHealthcheck();
    void waitForExpectationsMet();

    bool triggerCreateClientConnectToFakeSocket();
    bool triggerCreateClient();
    void triggerRemoveClient();
    bool triggerPerformSetConfiguration();
    bool triggerPerformSetState(const firebolt::rialto::common::SessionServerState &state);
    bool triggerSetLogLevels();

private:
    std::mutex m_expectationsMetMutex;
    std::condition_variable m_expectationsCv;
    bool m_expectationsFlag;
    RialtoSessionServerStub m_appStub;
    std::unique_ptr<rialto::servermanager::common::ISessionServerAppManager> m_sessionServerAppManager;
    StrictMock<rialto::servermanager::common::SessionServerAppManagerMock> &m_sessionServerAppManagerMock;
    std::unique_ptr<rialto::servermanager::ipc::IController> m_sut;
};

#endif // IPC_TESTS_FIXTURE_H_
