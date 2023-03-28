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

#include "IpcTestsFixture.h"
#include "Controller.h"
#include <gtest/gtest.h>
#include <string>

using testing::Invoke;

namespace
{
constexpr int kAppId{3};
} // namespace

IpcTests::IpcTests()
    : m_expectationsFlag{false},
      m_sessionServerAppManager{
          std::make_unique<StrictMock<rialto::servermanager::common::SessionServerAppManagerMock>>()},
      m_sessionServerAppManagerMock{dynamic_cast<StrictMock<rialto::servermanager::common::SessionServerAppManagerMock> &>(
          *m_sessionServerAppManager)},
      m_sut{std::make_unique<rialto::servermanager::ipc::Controller>(m_sessionServerAppManager)}
{
}

void IpcTests::configureServerToSendOkResponses()
{
    m_appStub.start(StubResponse::OK);
}

void IpcTests::configureServerToSendFailResponses()
{
    m_appStub.start(StubResponse::Fail);
}

void IpcTests::simulateStateChangedEventInactive()
{
    m_appStub.sendStateChangedEvent();
}

void IpcTests::simulateClientDisconnection()
{
    m_appStub.disconnectClient();
}

void IpcTests::sessionServerAppManagerWillBeNotifiedAboutSessionServerStateChange(
    const firebolt::rialto::common::SessionServerState &newState)
{
    EXPECT_CALL(m_sessionServerAppManagerMock, onSessionServerStateChanged(kAppId, newState))
        .WillOnce(Invoke(
            [this](const auto &, const auto &)
            {
                std::unique_lock<std::mutex> lock{m_expectationsMetMutex};
                m_expectationsFlag = true;
                m_expectationsCv.notify_one();
            }));
}

void IpcTests::waitForExpectationsMet()
{
    std::unique_lock<std::mutex> lock{m_expectationsMetMutex};
    m_expectationsCv.wait(lock, [this] { return m_expectationsFlag; });
}

bool IpcTests::triggerCreateClientConnectToFakeSocket()
{
    EXPECT_TRUE(m_sut);
    return m_sut->createClient(kAppId, 12345);
}

bool IpcTests::triggerCreateClient()
{
    EXPECT_TRUE(m_sut);
    return m_sut->createClient(kAppId, m_appStub.getClientSocket());
}

void IpcTests::triggerRemoveClient()
{
    EXPECT_TRUE(m_sut);
    return m_sut->removeClient(kAppId);
}

bool IpcTests::triggerPerformSetConfiguration()
{
    EXPECT_TRUE(m_sut);
    const auto initialState{firebolt::rialto::common::SessionServerState::INACTIVE};
    const std::string socketName{getenv("RIALTO_SOCKET_PATH")};
    constexpr firebolt::rialto::common::MaxResourceCapabilitites maxResource{2, 1};
    return m_sut->performSetConfiguration(kAppId, initialState, socketName, maxResource);
}

bool IpcTests::triggerPerformSetState(const firebolt::rialto::common::SessionServerState &state)
{
    EXPECT_TRUE(m_sut);
    return m_sut->performSetState(kAppId, state);
}

bool IpcTests::triggerSetLogLevels()
{
    EXPECT_TRUE(m_sut);
    rialto::servermanager::service::LoggingLevels logLevels;
    return m_sut->setLogLevels(logLevels);
}
