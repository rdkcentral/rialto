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

#include "ControlIpcTestBase.h"
#include <gtest/gtest.h>

using testing::Invoke;

class RialtoClientControlIpcCreateTest : public ControlIpcTestBase
{
protected:
    RialtoClientControlIpcCreateTest() = default;
    ~RialtoClientControlIpcCreateTest() override = default;
};

/**
 * Test that a ControlIpc object can be created successfully and connects to IPC by default.
 */
TEST_F(RialtoClientControlIpcCreateTest, CreateDestroy)
{
    createControlIpc();
    destroyControlIpc();
}

/**
 * Test that a ControlIpc object not created when ipc channel cannot be created.
 */
TEST_F(RialtoClientControlIpcCreateTest, CreateNoIpcChannel)
{
    expectInitIpcButAttachChannelFailure();
    EXPECT_CALL(*m_eventThreadFactoryMock, createEventThread(_)).WillOnce(Return(ByMove(std::move(m_eventThread))));

    EXPECT_THROW(m_controlIpc =
                     std::make_shared<ControlIpc>(&m_controlClientMock, *m_ipcClientMock, m_eventThreadFactoryMock),
                 std::runtime_error);
}

/**
 * Test that a ControlIpc object not created when the ipc channel is not connected.
 */
TEST_F(RialtoClientControlIpcCreateTest, CreateIpcChannelDisconnected)
{
    EXPECT_CALL(*m_eventThreadFactoryMock, createEventThread(_)).WillOnce(Return(ByMove(std::move(m_eventThread))));
    EXPECT_CALL(*m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock));
    EXPECT_CALL(*m_channelMock, isConnected()).WillOnce(Return(false));
    EXPECT_CALL(*m_ipcClientMock, reconnect()).WillOnce(Return(false));

    EXPECT_THROW(m_controlIpc =
                     std::make_shared<ControlIpc>(&m_controlClientMock, *m_ipcClientMock, m_eventThreadFactoryMock),
                 std::runtime_error);
}

/**
 * Test that a ControlIpc object not created when subscribing to events fails, and any event subscribed
 * are unsubscribed.
 */
TEST_F(RialtoClientControlIpcCreateTest, SubscribeEventFailure)
{
    constexpr int kEventId{0};
    expectInitIpc();
    EXPECT_CALL(*m_eventThreadFactoryMock, createEventThread(_)).WillOnce(Return(ByMove(std::move(m_eventThread))));

    EXPECT_CALL(*m_channelMock, subscribeImpl("firebolt.rialto.ApplicationStateChangeEvent", _, _))
        .WillOnce(Invoke(
            [this](const std::string &eventName, const google::protobuf::Descriptor *descriptor,
                   std::function<void(const std::shared_ptr<google::protobuf::Message> &msg)> &&handler)
            {
                m_notifyApplicationStateCb = std::move(handler);
                return kEventId;
            }));
    EXPECT_CALL(*m_channelMock, subscribeImpl("firebolt.rialto.PingEvent", _, _)).WillOnce(Return(-1));
    EXPECT_CALL(*m_channelMock, unsubscribe(kEventId));

    EXPECT_THROW(m_controlIpc =
                     std::make_shared<ControlIpc>(&m_controlClientMock, *m_ipcClientMock, m_eventThreadFactoryMock),
                 std::runtime_error);
}
