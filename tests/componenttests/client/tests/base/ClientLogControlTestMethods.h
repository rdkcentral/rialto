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

#ifndef FIREBOLT_RIALTO_CLIENT_CT_CLIENT_LOG_CONTROL_TEST_METHODS_H_
#define FIREBOLT_RIALTO_CLIENT_CT_CLIENT_LOG_CONTROL_TEST_METHODS_H_

#include "ClientLogHandlerMock.h"
#include "IClientLogControl.h"
#include "ServerStub.h"
#include <gtest/gtest.h>
#include <memory>

using ::testing::StrictMock;

using namespace firebolt::rialto;

namespace firebolt::rialto::client::ct
{
class ClientLogControlTestMethods
{
public:
    ClientLogControlTestMethods();
    virtual ~ClientLogControlTestMethods();

protected:
    // Strict Mocks
    std::shared_ptr<StrictMock<ClientLogHandlerMock>> m_clientLogHandlerMock;

    // Objects
    std::shared_ptr<IClientLogControlFactory> m_clientLogControlFactory;
    IClientLogControl *m_clientLogControl = nullptr;

    // Test methods
    void createClientLogControl();
    void registerLogHandler();
    void registerLogHandlerWithIgnoreLevels();
    void unregisterLogHandler();
    void setLogLevel(RIALTO_DEBUG_LEVEL level);
    void shouldLog(IClientLogHandler::Level level);
    void shouldReplaceLogHandler();
    void log(IClientLogHandler::Level level);

    // Component test helpers
    virtual void notifyEvent() = 0;
    virtual void waitEvent() = 0;
    virtual std::shared_ptr<ServerStub> &getServerStub() = 0;
    virtual int32_t getShmFd() = 0;
    virtual void *getShmAddress() = 0;
    virtual uint32_t getShmSize() = 0;
};
} // namespace firebolt::rialto::client::ct

#endif // FIREBOLT_RIALTO_CLIENT_CT_CLIENT_LOG_CONTROL_TEST_METHODS_H_
