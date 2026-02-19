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

#include "MediaKeySessionTestBase.h"

MATCHER_P2(keyIdMatcher, keyId, keyIdLength, "")
{
    if (0 == memcmp(arg, keyId, keyIdLength))
        return true;

    return false;
}

class RialtoServerMediaKeySessionCallbacksTest : public MediaKeySessionTestBase
{
protected:
    const std::string m_kUrl{"http://"};
    const std::vector<unsigned char> m_kLicenseRequestMessage{'d', 'e', 'f'};
    const std::vector<unsigned char> m_kLicenseRenewalMessage{'a', 'b', 'c'};
    KeyStatusVector m_keyStatusVec;

    RialtoServerMediaKeySessionCallbacksTest() { createKeySession(kWidevineKeySystem); }
    ~RialtoServerMediaKeySessionCallbacksTest() { destroyKeySession(); }

    void createKeyStatusVec()
    {
        m_keyStatusVec.push_back(std::make_pair(std::vector<unsigned char>{'q', 'w', 'e'}, KeyStatus::EXPIRED));
        m_keyStatusVec.push_back(std::make_pair(std::vector<unsigned char>{'y', 'u', 'i'}, KeyStatus::OUTPUT_RESTRICTED));
        m_keyStatusVec.push_back(std::make_pair(std::vector<unsigned char>{'k', 'j', 'l'}, KeyStatus::INTERNAL_ERROR));
    }
};

/**
 * Test that onProcessChallenge that is not linked to a generateRequest notifies licenseRenewal.
 */
TEST_F(RialtoServerMediaKeySessionCallbacksTest, ProcessChallengeNoGenerateRequest)
{
    mainThreadWillEnqueueTask();
    EXPECT_CALL(*m_mediaKeysClientMock, onLicenseRenewal(m_kKeySessionId, m_kLicenseRenewalMessage));

    m_mediaKeySession->onProcessChallenge(m_kUrl.c_str(), &m_kLicenseRenewalMessage[0], m_kLicenseRenewalMessage.size());
}

/**
 * Test that onProcessChallenge after a generateRequest for none Netflix key system notifies licenseRequest.
 */
TEST_F(RialtoServerMediaKeySessionCallbacksTest, ProcessChallengeGenerateRequest)
{
    generateRequest();
    mainThreadWillEnqueueTask();
    EXPECT_CALL(*m_mediaKeysClientMock, onLicenseRequest(m_kKeySessionId, m_kLicenseRequestMessage, m_kUrl));

    m_mediaKeySession->onProcessChallenge(m_kUrl.c_str(), &m_kLicenseRequestMessage[0], m_kLicenseRequestMessage.size());

    // OcdmSession will be closed on destruction
    expectCloseKeySession(kWidevineKeySystem);
}

/**
 * Test that onKeyUpdate stores the key status and onAllKeysUpdated notifies the statuses stored.
 */
TEST_F(RialtoServerMediaKeySessionCallbacksTest, KeyStatusUpdate)
{
    createKeyStatusVec();

    for (auto it = m_keyStatusVec.begin(); it != m_keyStatusVec.end(); it++)
    {
        mainThreadWillEnqueueTask();
        EXPECT_CALL(*m_ocdmSessionMock, getStatus(keyIdMatcher(&it->first[0], it->first.size()), it->first.size()))
            .WillOnce(Return(it->second));
        m_mediaKeySession->onKeyUpdated(&it->first[0], it->first.size());
    }

    mainThreadWillEnqueueTask();
    EXPECT_CALL(*m_mediaKeysClientMock, onKeyStatusesChanged(m_kKeySessionId, m_keyStatusVec));
    m_mediaKeySession->onAllKeysUpdated();
}
