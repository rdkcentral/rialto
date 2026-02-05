/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Sky UK
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

#include <vector>

#include "ActionTraits.h"
#include "ConfigureAction.h"
#include "ExpectMessage.h"
#include "Matchers.h"
#include "MediaKeysTestMethods.h"

using testing::_;
using testing::ByMove;
using testing::Return;
using testing::StrictMock;

namespace firebolt::rialto::server::ct
{
class LicenseRenewalTest : public virtual MediaKeysTestMethods
{
public:
    LicenseRenewalTest() {}
    virtual ~LicenseRenewalTest() {}

    void licenseRenew();

    void updateAllKeys();
    void updateOneKey();

    const std::vector<unsigned char> kOneKeyId{'a', 'z', 'q', 'l', 'D'};
};

void LicenseRenewalTest::licenseRenew()
{
    ExpectMessage<::firebolt::rialto::LicenseRenewalEvent> expectedMessage(m_clientStub);

    const std::string kUrl{"NOT PASSED TO CALLBACK"};
    const std::vector<unsigned char> kLicenseRenewalMessage{'x', 'u', 'A'};

    m_ocdmSessionClient->onProcessChallenge(kUrl.c_str(), &kLicenseRenewalMessage[0], kLicenseRenewalMessage.size());

    auto message = expectedMessage.getMessage();
    ASSERT_TRUE(message);
    ASSERT_EQ(message->media_keys_handle(), m_mediaKeysHandle);
    ASSERT_EQ(message->key_session_id(), m_mediaKeySessionId);
    const unsigned int kMax = message->license_renewal_message_size();
    ASSERT_EQ(kMax, kLicenseRenewalMessage.size());
    for (unsigned int i = 0; i < kMax; ++i)
    {
        ASSERT_EQ(message->license_renewal_message(i), kLicenseRenewalMessage[i]);
    }
}

void LicenseRenewalTest::updateAllKeys()
{
    ExpectMessage<::firebolt::rialto::KeyStatusesChangedEvent> expectedMessage(m_clientStub);

    m_ocdmSessionClient->onAllKeysUpdated();

    auto message = expectedMessage.getMessage();
    ASSERT_TRUE(message);
    ASSERT_EQ(message->media_keys_handle(), m_mediaKeysHandle);
    ASSERT_EQ(message->key_session_id(), m_mediaKeySessionId);

    const ::google::protobuf::RepeatedPtrField<::firebolt::rialto::KeyStatusesChangedEvent_KeyStatusPair> &key_statuses =
        message->key_statuses();

    EXPECT_FALSE(key_statuses.empty());
    for (const ::firebolt::rialto::KeyStatusesChangedEvent_KeyStatusPair &i : key_statuses)
    {
        ASSERT_EQ(i.key_status(), KeyStatusesChangedEvent_KeyStatus_USABLE);

        const unsigned int kMax = i.key_id_size();
        ASSERT_EQ(kMax, kOneKeyId.size());
        for (unsigned int j = 0; j < kMax; ++j)
        {
            ASSERT_EQ(i.key_id(j), kOneKeyId[j]);
        }
    }
}

void LicenseRenewalTest::updateOneKey()
{
    EXPECT_CALL(m_ocdmSessionMock, getStatus(::arrayMatcher(kOneKeyId), kOneKeyId.size()))
        .WillOnce(Return(KeyStatus::USABLE));

    m_ocdmSessionClient->onKeyUpdated(&kOneKeyId[0], kOneKeyId.size());
}

/*
 * Component Test: License renewal sequence.
 * Test Objective:
 *  Test the notification of license renewal and updating of the new license.
 *
 * Sequence Diagrams:
 *  License Renewal - Cobalt/OCDM, Update MKS - Cobalt/OCDM, "Destroy" MKS - Cobalt/OCDM
 *   - https://wiki.rdkcentral.com/display/ASP/Rialto+Media+Key+Session+Management+Design
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: MediaKeys
 *
 * Test Initialize:
 *   RialtoServerComponentTest::RialtoServerComponentTest() will set up wrappers and
 *      starts rialtoServer running in its own thread
 *   send a CreateMediaKeys message to rialtoServer
 *   expect a "createSession" call (to OCDM mock)
 *   send a CreateKeySession message to rialtoServer
 *
 *
 * Test Steps:
 *  Step 1: Notify license renewal
 *   Server notifies the client that of license renewal.
 *   Expect that the license renewal notification is processed by the client.
 *
 *  Step 2: Notify key statuses changed
 *   Server notifies the client of key statuses changed.
 *   Expect that the key statuses changed notification is processed by the client.
 *
 * Test Tear-down:
 *  Server is terminated.
 *
 * Expected Results:
 *  Client can be notified of license renewal and update the key session successfully.
 *
 * Code:
 */
TEST_F(LicenseRenewalTest, licenseRenewal)
{
    createMediaKeysWidevine();
    ocdmSessionWillBeCreated();
    createKeySession();

    // Step 1: Notify license renewal
    licenseRenew();

    // Step 2: Notify key statuses changed
    updateOneKey();
    updateAllKeys();
}

/*
 * Component Test: License renewal sequence for netflix playready.
 * Test Objective:
 *  Test the notification of license renewal and updating of the new license.
 *
 * Sequence Diagrams:
 *  License Renewal - Cobalt/OCDM, Update MKS - Cobalt/OCDM, "Destroy" MKS - Cobalt/OCDM
 *   - https://wiki.rdkcentral.com/display/ASP/Rialto+Media+Key+Session+Management+Design
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: MediaKeys
 *
 * Test Initialize:
 *   RialtoServerComponentTest::RialtoServerComponentTest() will set up wrappers and
 *      starts rialtoServer running in its own thread
 *   send a CreateMediaKeys message to rialtoServer
 *   expect a "createSession" call (to OCDM mock)
 *   send a CreateKeySession message to rialtoServer
 *   generate request message for playready and send it to the client
 *   expect the client to process the generate request message
 *
 *
 * Test Steps:
 *  Step 1: Update session
 *   updateSession with the updated license.
 *   Expect that updateSession is processed by the server.
 *   Api call returns with success.
 *
 *  Step 2: Close session
 *   closeSession.
 *   Expect that closeSession is processed by the server.
 *   Api call returns with success.
 *
 * Test Tear-down:
 *  Server is terminated.
 *
 * Expected Results:
 *  Client can be notified of license renewal and update the key session successfully.
 *
 * Code:
 */
TEST_F(LicenseRenewalTest, licenseRenewalNetflix)
{
    createMediaKeysNetflix();
    ocdmSessionWillBeCreated();
    createKeySession();
    willGenerateRequestPlayready();
    generateRequestPlayready();

    // Step 1: Update session
    willUpdateSessionNetflix();
    updateSessionNetflix();

    // Step 2: Close session
    willCloseKeySessionPlayready();
    closeKeySessionPlayready();
    willRelease();
}

} // namespace firebolt::rialto::server::ct
