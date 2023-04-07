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

#include "ControlClientMock.h"
#include "ControlServerInternal.h"
#include <gtest/gtest.h>

using testing::StrictMock;
using testing::Test;

namespace
{
constexpr int kId{3};
constexpr firebolt::rialto::ApplicationState kAppState{firebolt::rialto::ApplicationState::INACTIVE};
} // namespace

class ControlServerInternalTests : public Test
{
public:
    ControlServerInternalTests()
        : m_controlClientMock{std::make_shared<StrictMock<firebolt::rialto::ControlClientMock>>()}, m_sut{m_controlClientMock}
    {
    }

    std::shared_ptr<StrictMock<firebolt::rialto::ControlClientMock>> m_controlClientMock;
    firebolt::rialto::server::ControlServerInternal m_sut;
};

TEST_F(ControlServerInternalTests, shouldAck)
{
    // Currently not implemented, just for coverage
    m_sut.ack(kId);
}

TEST_F(ControlServerInternalTests, shouldSetApplicationState)
{
    EXPECT_CALL(*m_controlClientMock, notifyApplicationState(kAppState));
    m_sut.setApplicationState(kAppState);
}
