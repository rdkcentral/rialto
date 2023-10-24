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
#include <memory>
#include <string>
#include <utility>

MediaKeysIpcTestBase::MediaKeysIpcTestBase()
    : m_eventThreadFactoryMock{std::make_shared<StrictMock<EventThreadFactoryMock>>()},
      m_eventThread{std::make_unique<StrictMock<EventThreadMock>>()}, m_eventThreadMock{m_eventThread.get()},
      m_mediaKeysClientMock{std::make_shared<StrictMock<MediaKeysClientMock>>()}
{
}

MediaKeysIpcTestBase::~MediaKeysIpcTestBase() {}

void MediaKeysIpcTestBase::createMediaKeysIpc()
{
    expectInitIpc();
    expectSubscribeEvents();
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_eventThreadFactoryMock, createEventThread(_)).WillOnce(Return(ByMove(std::move(m_eventThread))));
    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("createMediaKeys"), _, _, _, _))
        .WillOnce(WithArgs<3>(Invoke(this, &MediaKeysIpcTestBase::setCreateMediaKeysResponse)));

    EXPECT_NO_THROW(
        m_mediaKeysIpc = std::make_unique<MediaKeysIpc>(m_keySystem, m_ipcClientMock, m_eventThreadFactoryMock));
    EXPECT_NE(m_mediaKeysIpc, nullptr);
}

void MediaKeysIpcTestBase::destroyMediaKeysIpc()
{
    expectIpcApiCallSuccess();
    expectUnsubscribeEvents();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("destroyMediaKeys"), _, _, _, _));

    m_mediaKeysIpc.reset();
    EXPECT_EQ(m_mediaKeysIpc, nullptr);
}

void MediaKeysIpcTestBase::expectSubscribeEvents()
{
    EXPECT_CALL(*m_channelMock, subscribeImpl("firebolt.rialto.LicenseRequestEvent", _, _))
        .WillOnce(Invoke(
            [this](const std::string &eventName, const google::protobuf::Descriptor *descriptor,
                   std::function<void(const std::shared_ptr<google::protobuf::Message> &msg)> &&handler)
            {
                m_licenseRequestCb = std::move(handler);
                return static_cast<int>(EventTags::LicenseRequestEvent);
            }))
        .RetiresOnSaturation();
    EXPECT_CALL(*m_channelMock, subscribeImpl("firebolt.rialto.LicenseRenewalEvent", _, _))
        .WillOnce(Invoke(
            [this](const std::string &eventName, const google::protobuf::Descriptor *descriptor,
                   std::function<void(const std::shared_ptr<google::protobuf::Message> &msg)> &&handler)
            {
                m_licenseRenewalCb = std::move(handler);
                return static_cast<int>(EventTags::LicenseRenewalEvent);
            }))
        .RetiresOnSaturation();
    EXPECT_CALL(*m_channelMock, subscribeImpl("firebolt.rialto.KeyStatusesChangedEvent", _, _))
        .WillOnce(Invoke(
            [this](const std::string &eventName, const google::protobuf::Descriptor *descriptor,
                   std::function<void(const std::shared_ptr<google::protobuf::Message> &msg)> &&handler)
            {
                m_KeyStatusesChangeCb = std::move(handler);
                return static_cast<int>(EventTags::KeyStatusesChangedEvent);
            }))
        .RetiresOnSaturation();
}

void MediaKeysIpcTestBase::expectUnsubscribeEvents()
{
    EXPECT_CALL(*m_channelMock, unsubscribe(static_cast<int>(EventTags::LicenseRequestEvent))).WillOnce(Return(true));
    EXPECT_CALL(*m_channelMock, unsubscribe(static_cast<int>(EventTags::LicenseRenewalEvent))).WillOnce(Return(true));
    EXPECT_CALL(*m_channelMock, unsubscribe(static_cast<int>(EventTags::KeyStatusesChangedEvent))).WillOnce(Return(true));
}

void MediaKeysIpcTestBase::createKeySession()
{
    int32_t returnKeySessionid;
    expectIpcApiCallSuccess();

    EXPECT_CALL(*m_channelMock, CallMethod(methodMatcher("createKeySession"), _, _, _, _))
        .WillOnce(WithArgs<3>(Invoke(this, &MediaKeysIpcTestBase::setCreateKeySessionResponseSuccess)));

    EXPECT_EQ(m_mediaKeysIpc->createKeySession(KeySessionType::PERSISTENT_LICENCE, m_mediaKeysClientMock, false,
                                               returnKeySessionid),
              MediaKeyErrorStatus::OK);
}

std::shared_ptr<firebolt::rialto::LicenseRenewalEvent> MediaKeysIpcTestBase::createLicenseRenewalEvent()
{
    auto licenseRenewalEvent = std::make_shared<firebolt::rialto::LicenseRenewalEvent>();
    licenseRenewalEvent->set_media_keys_handle(m_mediaKeysHandle);
    licenseRenewalEvent->set_key_session_id(m_kKeySessionId);

    for (auto it = m_licenseRenewalMessage.begin(); it != m_licenseRenewalMessage.end(); it++)
    {
        licenseRenewalEvent->add_license_renewal_message(*it);
    }

    return licenseRenewalEvent;
}

void MediaKeysIpcTestBase::setCreateMediaKeysResponse(google::protobuf::Message *response)
{
    firebolt::rialto::CreateMediaKeysResponse *createMediaKeysResponse =
        dynamic_cast<firebolt::rialto::CreateMediaKeysResponse *>(response);
    createMediaKeysResponse->set_media_keys_handle(m_mediaKeysHandle);
}

void MediaKeysIpcTestBase::setCreateKeySessionResponseSuccess(google::protobuf::Message *response)
{
    firebolt::rialto::CreateKeySessionResponse *createKeySessionResponse =
        dynamic_cast<firebolt::rialto::CreateKeySessionResponse *>(response);
    createKeySessionResponse->set_key_session_id(m_kKeySessionId);
    createKeySessionResponse->set_error_status(MediaKeysIpcTestBase::convertMediaKeyErrorStatus(MediaKeyErrorStatus::OK));
}

ProtoMediaKeyErrorStatus MediaKeysIpcTestBase::convertMediaKeyErrorStatus(firebolt::rialto::MediaKeyErrorStatus errorStatus)
{
    switch (errorStatus)
    {
    case firebolt::rialto::MediaKeyErrorStatus::OK:
    {
        return firebolt::rialto::ProtoMediaKeyErrorStatus::OK;
    }
    case firebolt::rialto::MediaKeyErrorStatus::BAD_SESSION_ID:
    {
        return firebolt::rialto::ProtoMediaKeyErrorStatus::BAD_SESSION_ID;
    }
    case firebolt::rialto::MediaKeyErrorStatus::NOT_SUPPORTED:
    {
        return firebolt::rialto::ProtoMediaKeyErrorStatus::NOT_SUPPORTED;
    }
    case firebolt::rialto::MediaKeyErrorStatus::INVALID_STATE:
    {
        return firebolt::rialto::ProtoMediaKeyErrorStatus::INVALID_STATE;
    }
    case firebolt::rialto::MediaKeyErrorStatus::FAIL:
    {
        return firebolt::rialto::ProtoMediaKeyErrorStatus::FAIL;
    }
    }
    return firebolt::rialto::ProtoMediaKeyErrorStatus::FAIL;
}
