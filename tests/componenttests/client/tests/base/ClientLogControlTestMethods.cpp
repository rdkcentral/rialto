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

#include "ClientLogControlTestMethods.h"
#include "RialtoClientLogging.h"
#include <memory>

using ::testing::_;
using ::testing::Ne;
using ::testing::StrEq;
using ::testing::StrNe;

namespace firebolt::rialto::client::ct
{
ClientLogControlTestMethods::ClientLogControlTestMethods()
    : m_clientLogHandlerMock{std::make_shared<StrictMock<ClientLogHandlerMock>>()}
{
}

ClientLogControlTestMethods::~ClientLogControlTestMethods() {}

void ClientLogControlTestMethods::createClientLogControl()
{
    m_clientLogControlFactory = firebolt::rialto::IClientLogControlFactory::createFactory();
    m_clientLogControl = &m_clientLogControlFactory->createClientLogControl();
    EXPECT_NE(m_clientLogControl, nullptr);
}

void ClientLogControlTestMethods::registerLogHandler()
{
    EXPECT_EQ(m_clientLogControl->registerLogHandler(m_clientLogHandlerMock, false), true);
}

void ClientLogControlTestMethods::registerLogHandlerWithIgnoreLevels()
{
    EXPECT_EQ(m_clientLogControl->registerLogHandler(m_clientLogHandlerMock, true), true);
}

void ClientLogControlTestMethods::unregisterLogHandler()
{
    EXPECT_EQ(m_clientLogControl->registerLogHandler(nullptr, false), true);
}

void ClientLogControlTestMethods::setLogLevel(RIALTO_DEBUG_LEVEL level)
{
    EXPECT_EQ(firebolt::rialto::logging::setLogLevels(RIALTO_COMPONENT_CLIENT, level),
              firebolt::rialto::logging::RIALTO_LOGGING_STATUS_OK);
}

void ClientLogControlTestMethods::shouldLog(IClientLogHandler::Level level)
{
    EXPECT_CALL(*m_clientLogHandlerMock,
                log(level, StrEq("ClientLogControlTestMethods.cpp"), Ne(0), StrNe(""), StrNe("")));
}

void ClientLogControlTestMethods::shouldReplaceLogHandler()
{
    EXPECT_CALL(*m_clientLogHandlerMock, log(_, StrEq("ClientLogControl.cpp"), _, _, _)).Times(2);
}

void ClientLogControlTestMethods::log(IClientLogHandler::Level level)
{
    switch (level)
    {
    case IClientLogHandler::Level::Fatal:
        RIALTO_CLIENT_LOG_FATAL("Test Fatal");
        break;
    case IClientLogHandler::Level::Error:
        RIALTO_CLIENT_LOG_ERROR("Test Error");
        break;
    case IClientLogHandler::Level::Warning:
        RIALTO_CLIENT_LOG_WARN("Test Warning");
        break;
    case IClientLogHandler::Level::Milestone:
        RIALTO_CLIENT_LOG_MIL("Test Milestone");
        break;
    case IClientLogHandler::Level::Info:
        RIALTO_CLIENT_LOG_INFO("Test Info");
        break;
    case IClientLogHandler::Level::Debug:
        RIALTO_CLIENT_LOG_DEBUG("Test Debug");
        break;
    case IClientLogHandler::Level::External:
    default:
        break;
    }
}

} // namespace firebolt::rialto::client::ct
