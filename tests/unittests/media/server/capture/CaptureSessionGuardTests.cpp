/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
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

#include "CaptureSessionGuard.h"
#include "CaptureSessionMock.h"
#include "RialtoLogging.h"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <vector>

using namespace firebolt::rialto;
using namespace firebolt::rialto::capture;
using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::StrictMock;
using ::testing::Throw;

namespace
{
// Collects server error log lines while it exists.
class ServerErrorLog
{
public:
    ServerErrorLog()
    {
        logging::setLogHandler(
            RIALTO_COMPONENT_SERVER,
            [this](RIALTO_DEBUG_LEVEL level, const char *, int, const char *, const char *message, std::size_t length)
            {
                if (level == RIALTO_DEBUG_LEVEL_ERROR)
                    m_errors.emplace_back(message, length);
            },
            true);
    }
    ~ServerErrorLog() { logging::setLogHandler(RIALTO_COMPONENT_SERVER, nullptr, false); }

    bool contains(const std::string &text) const
    {
        for (const auto &error : m_errors)
        {
            if (error.find(text) != std::string::npos)
                return true;
        }
        return false;
    }

private:
    std::vector<std::string> m_errors;
};
} // namespace

class CaptureSessionGuardTest : public ::testing::Test
{
protected:
    StrictMock<CaptureSessionMock> m_session;
    CaptureSessionGuard m_guard{m_session};
};

TEST_F(CaptureSessionGuardTest, ShouldPassResultsAndArgumentsThrough)
{
    CaptureCapabilities capabilities;
    CapturePool pool;
    CapturedFrame frame;

    EXPECT_CALL(m_session, getCapabilities(_)).WillOnce(Return(CaptureStatus::CAPABILITIES_NOT_AVAILABLE_YET));
    EXPECT_CALL(m_session, createPool(3, _)).WillOnce(Return(CaptureStatus::OK));
    EXPECT_CALL(m_session, acquireCurrentFrame(_))
        .WillOnce(Invoke(
            [](CapturedFrame &outFrame)
            {
                outFrame.slotIndex = 1;
                return CaptureStatus::OK;
            }));
    EXPECT_CALL(m_session, releaseFrame(_)).WillOnce(Return(CaptureStatus::INVALID_ARGUMENT));

    EXPECT_EQ(m_guard.getCapabilities(capabilities), CaptureStatus::CAPABILITIES_NOT_AVAILABLE_YET);
    EXPECT_EQ(m_guard.createPool(3, pool), CaptureStatus::OK);
    EXPECT_EQ(m_guard.acquireCurrentFrame(frame), CaptureStatus::OK);
    EXPECT_EQ(frame.slotIndex, 1U);
    EXPECT_EQ(m_guard.releaseFrame(frame), CaptureStatus::INVALID_ARGUMENT);
}

TEST_F(CaptureSessionGuardTest, ShouldReturnFatalErrorAndLogWhenAnyCallThrows)
{
    ServerErrorLog log;
    CaptureCapabilities capabilities;
    CapturePool pool;
    CapturedFrame frame;

    EXPECT_CALL(m_session, getCapabilities(_)).WillOnce(Throw(std::runtime_error("vendor bug")));
    EXPECT_CALL(m_session, createPool(_, _)).WillOnce(Throw(std::bad_alloc()));
    EXPECT_CALL(m_session, acquireCurrentFrame(_)).WillOnce(Throw(42));
    EXPECT_CALL(m_session, releaseFrame(_)).WillOnce(Throw(std::logic_error("bad release")));

    EXPECT_EQ(m_guard.getCapabilities(capabilities), CaptureStatus::FATAL_ERROR);
    EXPECT_EQ(m_guard.createPool(2, pool), CaptureStatus::FATAL_ERROR);
    EXPECT_EQ(m_guard.acquireCurrentFrame(frame), CaptureStatus::FATAL_ERROR);
    EXPECT_EQ(m_guard.releaseFrame(frame), CaptureStatus::FATAL_ERROR);

    EXPECT_TRUE(log.contains("getCapabilities threw an exception: vendor bug"));
    EXPECT_TRUE(log.contains("createPool threw an exception"));
    EXPECT_TRUE(log.contains("acquireCurrentFrame threw an exception"));
    EXPECT_TRUE(log.contains("releaseFrame threw an exception: bad release"));
}
