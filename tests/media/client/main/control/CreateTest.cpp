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

#include "Control.h"
#include "ControlIpcFactoryMock.h"
#include "ControlIpcMock.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::client;

using ::testing::_;
using ::testing::ByMove;
using ::testing::Return;
using ::testing::StrictMock;

class RialtoClientControlCreateTest : public ::testing::Test
{
protected:
    std::shared_ptr<StrictMock<ControlIpcFactoryMock>> m_controlIpcFactoryMock;
    std::shared_ptr<StrictMock<ControlIpcMock>> m_controlIpcMock;

    virtual void SetUp()
    {
        m_controlIpcFactoryMock = std::make_shared<StrictMock<ControlIpcFactoryMock>>();
        m_controlIpcMock = std::make_shared<StrictMock<ControlIpcMock>>();
    }

    virtual void TearDown()
    {
        m_controlIpcMock.reset();
        m_controlIpcFactoryMock.reset();
    }
};

/**
 * Test that creating and destroying a Control object connects and disconnects the Ipc .
 */
TEST_F(RialtoClientControlCreateTest, CreateDestroy)
{
    std::unique_ptr<IControl> control;

    // Create
    EXPECT_CALL(*m_controlIpcFactoryMock, getControlIpc()).WillOnce(Return(m_controlIpcMock));

    EXPECT_NO_THROW(control = std::make_unique<Control>(m_controlIpcFactoryMock));

    // Destroy
    control.reset();
}

/**
 * Test that create ControlIpc failure throws an exception when creating the Control object.
 */
TEST_F(RialtoClientControlCreateTest, CreateControlIpcFailure)
{
    std::unique_ptr<IControl> control;

    EXPECT_CALL(*m_controlIpcFactoryMock, getControlIpc()).WillOnce(Return(nullptr));

    EXPECT_THROW(control = std::make_unique<Control>(m_controlIpcFactoryMock), std::runtime_error);
    EXPECT_EQ(control, nullptr);
}

/**
 * Test that Control object is still destroyed if term fails.
 */
TEST_F(RialtoClientControlCreateTest, TermFailure)
{
    std::unique_ptr<IControl> control;

    // Create
    EXPECT_CALL(*m_controlIpcFactoryMock, getControlIpc()).WillOnce(Return(m_controlIpcMock));

    EXPECT_NO_THROW(control = std::make_unique<Control>(m_controlIpcFactoryMock));

    // Destroy
    control.reset();
}
