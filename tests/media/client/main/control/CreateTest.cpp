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
#include "ControlClientMock.h"
#include "SharedMemoryManagerMock.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::client;

using ::testing::Return;
using ::testing::StrictMock;

class RialtoClientControlCreateTest : public ::testing::Test
{
protected:
    std::shared_ptr<StrictMock<ControlClientMock>> m_controlClientMock;
    StrictMock<SharedMemoryManagerMock> m_sharedMemoryManagerMock;
};

TEST_F(RialtoClientControlCreateTest, CreateDestroy)
{
    m_controlClientMock = std::make_shared<StrictMock<ControlClientMock>>();
    std::unique_ptr<IControl> control;

    // Create
    EXPECT_CALL(m_sharedMemoryManagerMock, registerClient(m_controlClientMock.get())).WillOnce(Return(true));

    EXPECT_NO_THROW(control = std::make_unique<Control>(m_controlClientMock, m_sharedMemoryManagerMock));

    EXPECT_CALL(m_sharedMemoryManagerMock, unregisterClient(m_controlClientMock.get())).WillOnce(Return(true));

    // Destroy
    control.reset();
}

TEST_F(RialtoClientControlCreateTest, CreateControlWithoutRegisteringClient)
{
    std::unique_ptr<IControl> control;

    // Create
    EXPECT_NO_THROW(control = std::make_unique<Control>(m_controlClientMock, m_sharedMemoryManagerMock));

    // Destroy
    control.reset();
}
