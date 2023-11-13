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
#include <memory>
#include <string>
#include <utility>

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::WithArgs;

namespace
{
constexpr int kNotifyAppStateSubscriptionId{1};
constexpr int kPingSubscriptionId{2};
} // namespace

ControlIpcTestBase::ControlIpcTestBase()
    : m_eventThreadFactoryMock{std::make_shared<StrictMock<EventThreadFactoryMock>>()},
      m_eventThread{std::make_unique<StrictMock<EventThreadMock>>()}, m_eventThreadMock{m_eventThread.get()}
{
}

ControlIpcTestBase::~ControlIpcTestBase()
{
    m_eventThreadFactoryMock.reset();
    m_eventThreadMock = nullptr;
}

void ControlIpcTestBase::createControlIpc()
{
    expectInitIpc();
    expectSubscribeEvents();
    EXPECT_CALL(*m_eventThreadFactoryMock, createEventThread(_)).WillOnce(Return(ByMove(std::move(m_eventThread))));

    EXPECT_NO_THROW(
        m_controlIpc = std::make_shared<ControlIpc>(&m_controlClientMock, *m_ipcClientMock, m_eventThreadFactoryMock));
}

void ControlIpcTestBase::destroyControlIpc()
{
    expectUnsubscribeEvents();

    m_controlIpc.reset();
}

bool ControlIpcTestBase::registerClient(const std::optional<firebolt::rialto::common::SchemaVersion> &schemaVersion)
{
    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("registerClient"), m_controllerMock.get(), _, _, m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(
            [&](google::protobuf::Message *response)
            {
                firebolt::rialto::RegisterClientResponse *registerClientResponse =
                    dynamic_cast<firebolt::rialto::RegisterClientResponse *>(response);
                registerClientResponse->set_control_handle(m_kHandleId);
                if (schemaVersion)
                {
                    registerClientResponse->mutable_server_schema_version()->set_major(schemaVersion->major());
                    registerClientResponse->mutable_server_schema_version()->set_minor(schemaVersion->minor());
                    registerClientResponse->mutable_server_schema_version()->set_patch(schemaVersion->patch());
                }
            })));

    return m_controlIpc->registerClient();
}

void ControlIpcTestBase::expectSubscribeEvents()
{
    EXPECT_CALL(*m_channelMock, subscribeImpl("firebolt.rialto.ApplicationStateChangeEvent", _, _))
        .WillOnce(Invoke(
            [this](const std::string &eventName, const google::protobuf::Descriptor *descriptor,
                   std::function<void(const std::shared_ptr<google::protobuf::Message> &msg)> &&handler)
            {
                m_notifyApplicationStateCb = std::move(handler);
                return kNotifyAppStateSubscriptionId;
            }))
        .RetiresOnSaturation();
    EXPECT_CALL(*m_channelMock, subscribeImpl("firebolt.rialto.PingEvent", _, _))
        .WillOnce(Invoke(
            [this](const std::string &eventName, const google::protobuf::Descriptor *descriptor,
                   std::function<void(const std::shared_ptr<google::protobuf::Message> &msg)> &&handler)
            {
                m_pingCb = std::move(handler);
                return kPingSubscriptionId;
            }))
        .RetiresOnSaturation();
}

void ControlIpcTestBase::expectUnsubscribeEvents()
{
    EXPECT_CALL(*m_channelMock, unsubscribe(kNotifyAppStateSubscriptionId)).WillOnce(Return(true));
    EXPECT_CALL(*m_channelMock, unsubscribe(kPingSubscriptionId)).WillOnce(Return(true));
}
