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

#include "RialtoControlIpcTestBase.h"
#include <gtest/gtest.h>

using ::testing::Sequence;

class RialtoClientRialtoControlIpcCreateTest : public RialtoControlIpcTestBase
{
protected:
    // Sequence var
    Sequence m_processSeq;

    virtual void SetUp() { RialtoControlIpcTestBase::SetUp(); }

    virtual void TearDown() { RialtoControlIpcTestBase::TearDown(); }

    void expectIpcThreadProcess()
    {
        EXPECT_CALL(*m_channelMock, process()).InSequence(m_processSeq).WillOnce(Return(true)).RetiresOnSaturation();
        EXPECT_CALL(*m_channelMock, wait(-1));
        EXPECT_CALL(*m_channelMock, process()).InSequence(m_processSeq).WillOnce(Return(false)).RetiresOnSaturation();
    }
};

/**
 * Test that a RialtoControlIpc object can be created successfully and connects to IPC by default.
 */
TEST_F(RialtoClientRialtoControlIpcCreateTest, CreateDestroy)
{
    // Create
    EXPECT_CALL(*m_channelFactoryMock, createChannel(m_kRialtoPath)).WillOnce(Return(m_channelMock));
    expectIpcThreadProcess();

    EXPECT_NO_THROW(m_rialtoControlIpc = std::make_shared<RialtoControlIpc>(m_channelFactoryMock, m_controllerFactoryMock,
                                                                            m_blockingClosureFactoryMock));

    // Destroy
    EXPECT_CALL(*m_channelMock, disconnect());

    m_rialtoControlIpc.reset();
}

/**
 * Test that a RialtoControlIpc object not created when ipc channel cannot be created.
 */
TEST_F(RialtoClientRialtoControlIpcCreateTest, NoIpcChannel)
{
    EXPECT_CALL(*m_channelFactoryMock, createChannel(m_kRialtoPath)).WillOnce(Return(nullptr));

    EXPECT_THROW(m_rialtoControlIpc = std::make_shared<RialtoControlIpc>(m_channelFactoryMock, m_controllerFactoryMock,
                                                                         m_blockingClosureFactoryMock),
                 std::runtime_error);
}
