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

firebolt::rialto::KeyStatusesChangedEvent_KeyStatus convertKeyStatus(const firebolt::rialto::KeyStatus &keyStatus)
{
    switch (keyStatus)
    {
    case firebolt::rialto::KeyStatus::USABLE:
    {
        return firebolt::rialto::KeyStatusesChangedEvent_KeyStatus_USABLE;
    }
    case firebolt::rialto::KeyStatus::EXPIRED:
    {
        return firebolt::rialto::KeyStatusesChangedEvent_KeyStatus_EXPIRED;
    }
    case firebolt::rialto::KeyStatus::OUTPUT_RESTRICTED:
    {
        return firebolt::rialto::KeyStatusesChangedEvent_KeyStatus_OUTPUT_RESTRICTED;
    }
    case firebolt::rialto::KeyStatus::PENDING:
    {
        return firebolt::rialto::KeyStatusesChangedEvent_KeyStatus_PENDING;
    }
    case firebolt::rialto::KeyStatus::INTERNAL_ERROR:
    {
        return firebolt::rialto::KeyStatusesChangedEvent_KeyStatus_INTERNAL_ERROR;
    }
    case firebolt::rialto::KeyStatus::RELEASED:
    {
        return firebolt::rialto::KeyStatusesChangedEvent_KeyStatus_RELEASED;
    }
    }
    return firebolt::rialto::KeyStatusesChangedEvent_KeyStatus_INTERNAL_ERROR;
}

MATCHER_P(QosInfoMatcher, expectedQosInfo, "")
{
    return ((expectedQosInfo.processed == arg.processed) && (expectedQosInfo.dropped == arg.dropped));
}

class RialtoClientMediaKeysIpcCallbackTest : public MediaKeysIpcTestBase
{
protected:
    std::vector<unsigned char> m_licenseRequestMessage{'u', 'v', 'w'};
    std::string m_url{"http://"};
    KeyStatusVector m_keyStatuses;

    RialtoClientMediaKeysIpcCallbackTest() { createMediaKeysIpc(); }

    ~RialtoClientMediaKeysIpcCallbackTest() { destroyMediaKeysIpc(); }

    void addPairsToKeyStatusVector(uint32_t numPairs)
    {
        std::vector<KeyStatus> keyStatuses = {KeyStatus::USABLE,
                                              KeyStatus::EXPIRED,
                                              KeyStatus::OUTPUT_RESTRICTED,
                                              KeyStatus::PENDING,
                                              KeyStatus::INTERNAL_ERROR,
                                              KeyStatus::RELEASED};
        for (uint32_t i = 0; i < numPairs; i++)
        {
            std::vector<unsigned char> keyVector = std::vector<unsigned char>{(unsigned char)i};
            m_keyStatuses.push_back(std::make_pair(keyVector, keyStatuses[i % keyStatuses.size()]));
        }
    }

    std::shared_ptr<firebolt::rialto::LicenseRequestEvent> createLicenseRequestEvent()
    {
        auto licenseRequestEvent = std::make_shared<firebolt::rialto::LicenseRequestEvent>();
        licenseRequestEvent->set_media_keys_handle(m_mediaKeysHandle);
        licenseRequestEvent->set_url(m_url);
        licenseRequestEvent->set_key_session_id(m_kKeySessionId);

        for (auto it = m_licenseRequestMessage.begin(); it != m_licenseRequestMessage.end(); it++)
        {
            licenseRequestEvent->add_license_request_message(*it);
        }

        return licenseRequestEvent;
    }

    std::shared_ptr<firebolt::rialto::KeyStatusesChangedEvent> createKeyStatusesChangedEvent()
    {
        auto keyStatusesChangedEvent = std::make_shared<firebolt::rialto::KeyStatusesChangedEvent>();
        keyStatusesChangedEvent->set_media_keys_handle(m_mediaKeysHandle);
        keyStatusesChangedEvent->set_key_session_id(m_kKeySessionId);

        for (auto it = m_keyStatuses.begin(); it != m_keyStatuses.end(); it++)
        {
            ::firebolt::rialto::KeyStatusesChangedEvent_KeyStatusPair *keyStatusPair =
                keyStatusesChangedEvent->add_key_statuses();
            for (auto it2 = it->first.begin(); it2 != it->first.end(); it2++)
            {
                keyStatusPair->add_key_id(*it2);
            }
            keyStatusPair->set_key_status(convertKeyStatus(it->second));
        }

        return keyStatusesChangedEvent;
    }
};

/**
 * Test that a license request over IPC is forwarded to the client.
 */
TEST_F(RialtoClientMediaKeysIpcCallbackTest, NotifyLicenseRequest)
{
    createKeySession();

    EXPECT_CALL(*m_eventThreadMock, addImpl(_)).WillOnce(Invoke([](std::function<void()> &&func) { func(); }));
    EXPECT_CALL(*m_mediaKeysClientMock, onLicenseRequest(m_kKeySessionId, m_licenseRequestMessage, m_url));

    m_licenseRequestCb(createLicenseRequestEvent());
}

/**
 * Test that if the media keys handle of the event is not the same as the media keys object the event will be ignored.
 */
TEST_F(RialtoClientMediaKeysIpcCallbackTest, InvalidHandleLicenseRequest)
{
    createKeySession();

    EXPECT_CALL(*m_eventThreadMock, addImpl(_)).WillOnce(Invoke([](std::function<void()> &&func) { func(); }));

    auto licenseRequestEvent = createLicenseRequestEvent();
    licenseRequestEvent->set_media_keys_handle(m_mediaKeysHandle + 1);
    m_licenseRequestCb(licenseRequestEvent);
}

/**
 * Test that if no client has been registered/key session not created on the media keys object the event will be ignored.
 */
TEST_F(RialtoClientMediaKeysIpcCallbackTest, NoClientLicenseRequest)
{
    EXPECT_CALL(*m_eventThreadMock, addImpl(_)).WillOnce(Invoke([](std::function<void()> &&func) { func(); }));
    m_licenseRequestCb(createLicenseRequestEvent());
}

/**
 * Test that a license renewal over IPC is forwarded to the client.
 */
TEST_F(RialtoClientMediaKeysIpcCallbackTest, NotifyLicenseRenewal)
{
    createKeySession();

    EXPECT_CALL(*m_eventThreadMock, addImpl(_)).WillOnce(Invoke([](std::function<void()> &&func) { func(); }));
    EXPECT_CALL(*m_mediaKeysClientMock, onLicenseRenewal(m_kKeySessionId, m_licenseRenewalMessage));

    m_licenseRenewalCb(createLicenseRenewalEvent());
}

/**
 * Test that if the session id of the event is not the same as the playback session the event will be ignored.
 */
TEST_F(RialtoClientMediaKeysIpcCallbackTest, InvalidHandleLicenseRenewal)
{
    createKeySession();

    EXPECT_CALL(*m_eventThreadMock, addImpl(_)).WillOnce(Invoke([](std::function<void()> &&func) { func(); }));

    auto licenseRenewalEvent = createLicenseRenewalEvent();
    licenseRenewalEvent->set_media_keys_handle(m_mediaKeysHandle + 1);
    m_licenseRenewalCb(licenseRenewalEvent);
}

/**
 * Test that if no client has been registered/key session not created on the media keys object the event will be ignored.
 */
TEST_F(RialtoClientMediaKeysIpcCallbackTest, NoClientLicenseRenewal)
{
    EXPECT_CALL(*m_eventThreadMock, addImpl(_)).WillOnce(Invoke([](std::function<void()> &&func) { func(); }));
    m_licenseRenewalCb(createLicenseRenewalEvent());
}

/**
 * Test that a key statuses change over IPC is forwarded to the client.
 */
TEST_F(RialtoClientMediaKeysIpcCallbackTest, NotifyKeyStatusesChanged)
{
    createKeySession();
    addPairsToKeyStatusVector(3);

    EXPECT_CALL(*m_eventThreadMock, addImpl(_)).WillOnce(Invoke([](std::function<void()> &&func) { func(); }));
    EXPECT_CALL(*m_mediaKeysClientMock, onKeyStatusesChanged(m_kKeySessionId, m_keyStatuses));

    m_KeyStatusesChangeCb(createKeyStatusesChangedEvent());
}

/**
 * Test that if the session id of the event is not the same as the playback session the event will be ignored.
 */
TEST_F(RialtoClientMediaKeysIpcCallbackTest, InvalidHandleKeyStatusesChanged)
{
    createKeySession();
    addPairsToKeyStatusVector(3);

    EXPECT_CALL(*m_eventThreadMock, addImpl(_)).WillOnce(Invoke([](std::function<void()> &&func) { func(); }));

    auto keyStatusesChangedEvent = createKeyStatusesChangedEvent();
    keyStatusesChangedEvent->set_media_keys_handle(m_mediaKeysHandle + 1);
    m_KeyStatusesChangeCb(keyStatusesChangedEvent);
}

/**
 * Test that if no client has been registered/key session not created on the media keys object the event will be ignored.
 */
TEST_F(RialtoClientMediaKeysIpcCallbackTest, NoClientKeyStatusesChanged)
{
    EXPECT_CALL(*m_eventThreadMock, addImpl(_)).WillOnce(Invoke([](std::function<void()> &&func) { func(); }));
    m_KeyStatusesChangeCb(createKeyStatusesChangedEvent());
}
