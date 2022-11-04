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

#include "RialtoControl.h"
#include "RialtoControlIpcFactoryMock.h"
#include "RialtoControlIpcMock.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::client;
using namespace firebolt::rialto::client;

using ::testing::_;
using ::testing::ByMove;
using ::testing::Return;
using ::testing::StrictMock;

class RialtoClientRialtoControlCreateTest : public ::testing::Test
{
protected:
    std::shared_ptr<StrictMock<RialtoControlIpcFactoryMock>> m_rialtoControlIpcFactoryMock;
    std::shared_ptr<StrictMock<RialtoControlIpcMock>> m_rialtoControlIpcMock;

    virtual void SetUp()
    {
        m_rialtoControlIpcFactoryMock = std::make_shared<StrictMock<RialtoControlIpcFactoryMock>>();
        m_rialtoControlIpcMock = std::make_shared<StrictMock<RialtoControlIpcMock>>();
    }

    virtual void TearDown()
    {
        m_rialtoControlIpcMock.reset();
        m_rialtoControlIpcFactoryMock.reset();
    }
};

/**
 * Test that creating and destroying a RialtoControl object connects and disconnects the Ipc .
 */
TEST_F(RialtoClientRialtoControlCreateTest, CreateDestroy)
{
    std::unique_ptr<IRialtoControl> rialtoControl;

    // Create
    EXPECT_CALL(*m_rialtoControlIpcFactoryMock, getRialtoControlIpc()).WillOnce(Return(m_rialtoControlIpcMock));

    EXPECT_NO_THROW(rialtoControl = std::make_unique<RialtoControl>(m_rialtoControlIpcFactoryMock));

    // Destroy
    rialtoControl.reset();
}

/**
 * Test that create RialtoControlIpc failure throws an exception when creating the RialtoControl object.
 */
TEST_F(RialtoClientRialtoControlCreateTest, CreateRialtoControlIpcFailure)
{
    std::unique_ptr<IRialtoControl> rialtoControl;

    EXPECT_CALL(*m_rialtoControlIpcFactoryMock, getRialtoControlIpc()).WillOnce(Return(nullptr));

    EXPECT_THROW(rialtoControl = std::make_unique<RialtoControl>(m_rialtoControlIpcFactoryMock), std::runtime_error);
    EXPECT_EQ(rialtoControl, nullptr);
}

/**
 * Test that RialtoControl object is still destroyed if term fails.
 */
TEST_F(RialtoClientRialtoControlCreateTest, TermFailure)
{
    std::unique_ptr<IRialtoControl> rialtoControl;

    // Create
    EXPECT_CALL(*m_rialtoControlIpcFactoryMock, getRialtoControlIpc()).WillOnce(Return(m_rialtoControlIpcMock));

    EXPECT_NO_THROW(rialtoControl = std::make_unique<RialtoControl>(m_rialtoControlIpcFactoryMock));

    // Destroy
    rialtoControl.reset();
}
