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

#include "ClientController.h"
#include "ControlIpcFactoryMock.h"
#include "ControlIpcMock.h"
#include "PrivateMetricsIpcFactoryMock.h"
#include "PrivateMetricsIpcMock.h"
#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::client;

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::StrictMock;

class ClientControllerCreateTest : public ::testing::Test
{
protected:
    std::shared_ptr<StrictMock<ControlIpcFactoryMock>> m_controlIpcFactoryMock;
    std::shared_ptr<StrictMock<ControlIpcMock>> m_controlIpcMock;
    std::shared_ptr<StrictMock<PrivateMetricsIpcFactoryMock>> m_privateMetricsIpcFactoryMock;
    std::shared_ptr<NiceMock<PrivateMetricsIpcMock>> m_privateMetricsIpcMock;

    ClientControllerCreateTest()
        : m_controlIpcFactoryMock{std::make_shared<StrictMock<ControlIpcFactoryMock>>()},
          m_controlIpcMock{std::make_shared<StrictMock<ControlIpcMock>>()},
          m_privateMetricsIpcFactoryMock{std::make_shared<StrictMock<PrivateMetricsIpcFactoryMock>>()},
          m_privateMetricsIpcMock{std::make_shared<NiceMock<PrivateMetricsIpcMock>>()}
    {
    }

    ~ClientControllerCreateTest()
    {
        m_controlIpcMock.reset();
        m_controlIpcFactoryMock.reset();
        m_privateMetricsIpcMock.reset();
        m_privateMetricsIpcFactoryMock.reset();
    }
};

TEST_F(ClientControllerCreateTest, CreateDestroy)
{
    std::unique_ptr<IClientController> controller;

    // Create
    EXPECT_CALL(*m_controlIpcFactoryMock, createControlIpc(_)).WillOnce(Return(m_controlIpcMock));
    EXPECT_CALL(*m_privateMetricsIpcFactoryMock, createPrivateMetricsIpc(_)).WillOnce(Return(m_privateMetricsIpcMock));

    EXPECT_NO_THROW(controller =
                        std::make_unique<ClientController>(m_controlIpcFactoryMock, m_privateMetricsIpcFactoryMock));

    // Destroy
    controller.reset();
}

TEST_F(ClientControllerCreateTest, CreateControlIpcFailure)
{
    std::unique_ptr<IClientController> controller;

    EXPECT_CALL(*m_controlIpcFactoryMock, createControlIpc(_)).WillOnce(Return(nullptr));

    EXPECT_THROW(controller = std::make_unique<ClientController>(m_controlIpcFactoryMock, m_privateMetricsIpcFactoryMock),
                 std::runtime_error);
    EXPECT_EQ(controller, nullptr);
}
