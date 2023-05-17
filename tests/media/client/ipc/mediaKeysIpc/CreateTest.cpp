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

#include "MediaKeysIpcTestBase.h"

class RialtoClientCreateMediaKeysIpcTest : public MediaKeysIpcTestBase
{
};

/**
 * Test that a MediaKeysIpc object can be created successfully.
 */
TEST_F(RialtoClientCreateMediaKeysIpcTest, Create)
{
    /* create media keys */
    expectInitIpc();
    expectSubscribeEvents();
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_eventThreadFactoryMock, createEventThread(_)).WillOnce(Return(ByMove(std::move(m_eventThread))));
    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("createMediaKeys"), m_controllerMock.get(),
                                           createMediaKeysRequestMatcher(m_keySystem), _, m_blockingClosureMock.get()))
        .WillOnce(WithArgs<3>(Invoke(this, &RialtoClientCreateMediaKeysIpcTest::setCreateMediaKeysResponse)));

    EXPECT_NO_THROW(
        m_mediaKeysIpc = std::make_unique<MediaKeysIpc>(m_keySystem, m_ipcClientMock, m_eventThreadFactoryMock));
    EXPECT_NE(m_mediaKeysIpc, nullptr);

    /* destroy media keys */
    expectIpcApiCallSuccess();
    expectUnsubscribeEvents();

    EXPECT_CALL(*m_channelMock,
                CallMethod(methodMatcher("destroyMediaKeys"), m_controllerMock.get(),
                           destroyMediaKeysRequestMatcher(m_mediaKeysHandle), _, m_blockingClosureMock.get()));

    m_mediaKeysIpc.reset();
    EXPECT_EQ(m_mediaKeysIpc, nullptr);
}

/**
 * Test that a MediaKeysIpc object not created when the ipc channel has not been created.
 */
TEST_F(RialtoClientCreateMediaKeysIpcTest, CreateNoIpcChannel)
{
    EXPECT_CALL(*m_eventThreadFactoryMock, createEventThread(_)).WillOnce(Return(ByMove(std::move(m_eventThread))));
    EXPECT_CALL(m_ipcClientMock, getChannel()).WillOnce(Return(nullptr));

    EXPECT_THROW(m_mediaKeysIpc = std::make_unique<MediaKeysIpc>(m_keySystem, m_ipcClientMock, m_eventThreadFactoryMock),
                 std::runtime_error);
}

/**
 * Test that a MediaKeysIpc object not created when the ipc channel is not connected.
 */
TEST_F(RialtoClientCreateMediaKeysIpcTest, CreateIpcChannelDisconnected)
{
    EXPECT_CALL(*m_eventThreadFactoryMock, createEventThread(_)).WillOnce(Return(ByMove(std::move(m_eventThread))));
    EXPECT_CALL(m_ipcClientMock, getChannel()).WillOnce(Return(m_channelMock));
    EXPECT_CALL(*m_channelMock, isConnected()).WillOnce(Return(false));

    EXPECT_THROW(m_mediaKeysIpc = std::make_unique<MediaKeysIpc>(m_keySystem, m_ipcClientMock, m_eventThreadFactoryMock),
                 std::runtime_error);
}

/**
 * Test that a MediaKeysIpc object not created when subscribing to events fails, and any event subscribed
 * are unsubscribed.
 */
TEST_F(RialtoClientCreateMediaKeysIpcTest, SubscribeEventFailure)
{
    expectInitIpc();
    EXPECT_CALL(*m_eventThreadFactoryMock, createEventThread(_)).WillOnce(Return(ByMove(std::move(m_eventThread))));

    EXPECT_CALL(*m_channelMock, subscribeImpl("firebolt.rialto.LicenseRequestEvent", _, _))
        .WillOnce(Invoke(
            [this](const std::string &eventName, const google::protobuf::Descriptor *descriptor,
                   std::function<void(const std::shared_ptr<google::protobuf::Message> &msg)> &&handler)
            {
                m_licenseRequestCb = std::move(handler);
                return static_cast<int>(EventTags::LicenseRequestEvent);
            }));
    EXPECT_CALL(*m_channelMock, subscribeImpl("firebolt.rialto.LicenseRenewalEvent", _, _)).WillOnce(Return(-1));
    EXPECT_CALL(*m_channelMock, unsubscribe(static_cast<int>(EventTags::LicenseRequestEvent)));

    EXPECT_THROW(m_mediaKeysIpc = std::make_unique<MediaKeysIpc>(m_keySystem, m_ipcClientMock, m_eventThreadFactoryMock),
                 std::runtime_error);
}

/**
 * Test that a MediaKeysIpc object not created when create media keys fails.
 */
TEST_F(RialtoClientCreateMediaKeysIpcTest, CreateMediaKeysFailure)
{
    expectInitIpc();
    expectSubscribeEvents();
    expectIpcApiCallFailure();
    expectUnsubscribeEvents();

    EXPECT_CALL(*m_eventThreadFactoryMock, createEventThread(_)).WillOnce(Return(ByMove(std::move(m_eventThread))));
    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("createMediaKeys"), _, _, _, _));

    EXPECT_THROW(m_mediaKeysIpc = std::make_unique<MediaKeysIpc>(m_keySystem, m_ipcClientMock, m_eventThreadFactoryMock),
                 std::runtime_error);
}

/**
 * Test that when destroy media keys fails, the MediaKeysIpc object is still destroyed.
 */
TEST_F(RialtoClientCreateMediaKeysIpcTest, DestroyMediaKeysFailure)
{
    /* create media keys */
    createMediaKeysIpc();

    /* destroy media keys */
    expectUnsubscribeEvents();
    expectIpcApiCallFailure();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("destroyMediaKeys"), _, _, _, _));

    m_mediaKeysIpc.reset();
    EXPECT_EQ(m_mediaKeysIpc, nullptr);
}

/**
 * Test that when the ipc channel is disconnected, the MediaKeysIpc object is still destroyed.
 */
TEST_F(RialtoClientCreateMediaKeysIpcTest, DestructorChannelDisconnected)
{
    /* create media keys */
    createMediaKeysIpc();

    /* destroy media keys */
    expectIpcApiCallDisconnected();
    expectUnsubscribeEvents();

    m_mediaKeysIpc.reset();
    EXPECT_EQ(m_mediaKeysIpc, nullptr);
}
