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

#include "ControlIpcTestBase.h"
#include "controlmodule.pb.h"

using testing::_;
using testing::Invoke;

namespace
{
constexpr std::uint32_t kPingId{8};
} // namespace

class ControlIpcPingTest : public ControlIpcTestBase
{
protected:
    ControlIpcPingTest()
    {
        createControlIpc();
        expectIpcApiCallSuccess();
        registerClient();
        EXPECT_CALL(*m_eventThreadMock, addImpl(_)).WillOnce(Invoke([](std::function<void()> &&func) { func(); }));
    }

    ~ControlIpcPingTest() override = default;

    std::shared_ptr<firebolt::rialto::PingEvent> createEvent()
    {
        auto appStateChangeEvent = std::make_shared<firebolt::rialto::PingEvent>();
        appStateChangeEvent->set_control_handle(m_kHandleId);
        appStateChangeEvent->set_id(kPingId);
        return appStateChangeEvent;
    }
};

TEST_F(ControlIpcPingTest, shouldPingAndAck)
{
    ASSERT_TRUE(m_pingCb);

    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("ack"), m_controllerMock.get(), _, _, m_blockingClosureMock.get()));

    m_pingCb(createEvent());

    destroyControlIpc();
}

TEST_F(ControlIpcPingTest, ChannelDisconnected)
{
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();
    EXPECT_CALL(m_ipcClientMock, unregisterConnectionObserver());

    m_pingCb(createEvent());
}

TEST_F(ControlIpcPingTest, ReconnectChannel)
{
    expectIpcApiCallReconnected();
    expectUnsubscribeEvents();
    expectSubscribeEvents();

    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("ack"), m_controllerMock.get(), _, _, m_blockingClosureMock.get()));

    m_pingCb(createEvent());

    destroyControlIpc();
}

TEST_F(ControlIpcPingTest, ackFailure)
{
    expectIpcApiCallFailure();
    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("ack"), m_controllerMock.get(), _, _, m_blockingClosureMock.get()));

    m_pingCb(createEvent());

    destroyControlIpc();
}

TEST_F(ControlIpcPingTest, wrongHandleId)
{
    auto event = createEvent();
    event->set_control_handle(1234);

    m_pingCb(event);

    destroyControlIpc();
}
