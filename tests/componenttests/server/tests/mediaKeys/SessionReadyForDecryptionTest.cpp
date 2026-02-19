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

class SessionReadyForDecryptionTest : public virtual MediaKeysTestMethods
{
public:
    SessionReadyForDecryptionTest() {}
    virtual ~SessionReadyForDecryptionTest() {}

    void willGenerateRequestWidevine();
    void generateRequestWidevine();
    void processChallengeWidevine();

    void willGenerateRequestNetflix();
    void generateRequestNetflix();

    void willUpdateSessionWidevine();
    void updateSessionWidevine();
    void closeKeySessionWidevine();

    void willCloseKeySessionNetflix();
    void closeKeySessionNetflix();

    void releaseKeySession();

    void destroyMediaKeysRequest();

    const std::vector<unsigned char> kResponse{4, 1, 3};
};

void SessionReadyForDecryptionTest::willGenerateRequestWidevine()
{
    EXPECT_CALL(m_ocdmSessionMock, constructSession(KeySessionType::TEMPORARY, InitDataType::CENC, _, m_kInitData.size()))
        .WillOnce(testing::Invoke(
            [&](KeySessionType sessionType, InitDataType initDataType, const uint8_t initData[],
                uint32_t initDataSize) -> MediaKeyErrorStatus
            {
                for (uint32_t i = 0; i < initDataSize; ++i)
                {
                    EXPECT_EQ(initData[i], m_kInitData[i]);
                }

                return MediaKeyErrorStatus::OK;
            }));
}

void SessionReadyForDecryptionTest::generateRequestWidevine()
{
    auto request{createGenerateRequestRequest(m_mediaKeysHandle, m_mediaKeySessionId, m_kInitData)};

    ConfigureAction<GenerateRequest>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse([&](const firebolt::rialto::GenerateRequestResponse &resp)
                       { EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::OK); });
}

void SessionReadyForDecryptionTest::processChallengeWidevine()
{
    const std::string kUrl{"http://fictionalUrlForTest"};
    const std::vector<unsigned char> kLicenseRequestMessage{'d', 'z', 'f'};

    ExpectMessage<::firebolt::rialto::LicenseRequestEvent> expectedMessage(m_clientStub);
    m_ocdmSessionClient->onProcessChallenge(kUrl.c_str(), &kLicenseRequestMessage[0], kLicenseRequestMessage.size());

    auto message = expectedMessage.getMessage();
    ASSERT_TRUE(message);
    ASSERT_EQ(message->media_keys_handle(), m_mediaKeysHandle);
    ASSERT_EQ(message->key_session_id(), m_mediaKeySessionId);
    EXPECT_TRUE(message->url() == kUrl);
    const unsigned int kMax = message->license_request_message_size();
    ASSERT_EQ(kMax, kLicenseRequestMessage.size());
    for (unsigned int i = 0; i < kMax; ++i)
    {
        ASSERT_EQ(message->license_request_message(i), kLicenseRequestMessage[i]);
    }
}

void SessionReadyForDecryptionTest::willGenerateRequestNetflix()
{
    EXPECT_CALL(m_ocdmSessionMock, constructSession(KeySessionType::TEMPORARY, InitDataType::CENC, _, m_kInitData.size()))
        .WillOnce(testing::Invoke(
            [&](KeySessionType sessionType, InitDataType initDataType, const uint8_t initData[],
                uint32_t initDataSize) -> MediaKeyErrorStatus
            {
                for (uint32_t i = 0; i < initDataSize; ++i)
                {
                    EXPECT_EQ(initData[i], m_kInitData[i]);
                }

                return MediaKeyErrorStatus::OK;
            }));

    EXPECT_CALL(m_ocdmSessionMock, getChallengeData(false, _, _))
        .WillOnce(testing::Invoke(
            [&](bool isLDL, const uint8_t *challenge, uint32_t *challengeSize) -> MediaKeyErrorStatus
            {
                // This first call asks for the size of the data
                EXPECT_EQ(challenge, nullptr);
                *challengeSize = m_kLicenseRequestMessage.size();
                return MediaKeyErrorStatus::OK;
            }))
        .WillOnce(testing::Invoke(
            [&](bool isLDL, uint8_t *challenge, const uint32_t *challengeSize) -> MediaKeyErrorStatus
            {
                // This second call asks for the data
                EXPECT_EQ(*challengeSize, m_kLicenseRequestMessage.size());
                for (size_t i = 0; i < m_kLicenseRequestMessage.size(); ++i)
                {
                    challenge[i] = m_kLicenseRequestMessage[i];
                }
                return MediaKeyErrorStatus::OK;
            }));
}

void SessionReadyForDecryptionTest::generateRequestNetflix()
{
    constexpr bool kUseExtendedInterface{true};
    auto request{createGenerateRequestRequest(m_mediaKeysHandle, m_mediaKeySessionId, m_kInitData, kUseExtendedInterface)};

    ExpectMessage<::firebolt::rialto::LicenseRequestEvent> expectedMessage(m_clientStub);

    ConfigureAction<GenerateRequest>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse([&](const firebolt::rialto::GenerateRequestResponse &resp)
                       { EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::OK); });

    auto message = expectedMessage.getMessage();
    ASSERT_TRUE(message);
    ASSERT_EQ(message->media_keys_handle(), m_mediaKeysHandle);
    ASSERT_EQ(message->key_session_id(), m_mediaKeySessionId);
    EXPECT_EQ(message->url(), "");
    const unsigned int kMax = message->license_request_message_size();
    ASSERT_EQ(kMax, m_kLicenseRequestMessage.size());
    for (unsigned int i = 0; i < kMax; ++i)
    {
        ASSERT_EQ(message->license_request_message(i), m_kLicenseRequestMessage[i]);
    }
}

void SessionReadyForDecryptionTest::willUpdateSessionWidevine()
{
    // The following should match the details within the message "request"
    EXPECT_CALL(m_ocdmSessionMock, update(_, kResponse.size()))
        .WillOnce(testing::Invoke(
            [&](const uint8_t response[], uint32_t responseSize) -> MediaKeyErrorStatus
            {
                for (uint32_t i = 0; i < responseSize; ++i)
                {
                    EXPECT_EQ(response[i], kResponse[i]);
                }
                return MediaKeyErrorStatus::OK;
            }));
}

void SessionReadyForDecryptionTest::updateSessionWidevine()
{
    auto request{createUpdateSessionRequest(m_mediaKeysHandle, m_mediaKeySessionId, kResponse)};

    ConfigureAction<UpdateSession>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse([&](const firebolt::rialto::UpdateSessionResponse &resp)
                       { EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::OK); });
}

void SessionReadyForDecryptionTest::closeKeySessionWidevine()
{
    auto request{createCloseKeySessionRequest(m_mediaKeysHandle, m_mediaKeySessionId)};

    ConfigureAction<CloseKeySession>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse([&](const firebolt::rialto::CloseKeySessionResponse &resp)
                       { EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::OK); });
}

void SessionReadyForDecryptionTest::willCloseKeySessionNetflix()
{
    EXPECT_CALL(m_ocdmSessionMock, cancelChallengeData()).WillOnce(Return(MediaKeyErrorStatus::OK));
    EXPECT_CALL(m_ocdmSessionMock, cleanDecryptContext()).WillOnce(Return(MediaKeyErrorStatus::OK));
}
void SessionReadyForDecryptionTest::closeKeySessionNetflix()
{
    auto request{createCloseKeySessionRequest(m_mediaKeysHandle, m_mediaKeySessionId)};

    ConfigureAction<CloseKeySession>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse([&](const firebolt::rialto::CloseKeySessionResponse &resp)
                       { EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::OK); });
}

void SessionReadyForDecryptionTest::releaseKeySession()
{
    auto request{createReleaseKeySessionRequest(m_mediaKeysHandle, m_mediaKeySessionId)};

    ConfigureAction<ReleaseKeySession>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse([&](const firebolt::rialto::ReleaseKeySessionResponse &resp)
                       { EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::OK); });
}

void SessionReadyForDecryptionTest::destroyMediaKeysRequest()
{
    auto request{createDestroyMediaKeysRequest(m_mediaKeysHandle)};

    ConfigureAction<DestroyMediaKeys>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse([&](const firebolt::rialto::DestroyMediaKeysResponse &resp) {});
}

/*
 * Component Test: Session initalised and ready for decryption for widevine
 * Test Objective:
 *  Test the license generation and updating of a key session for widevine media
 *  key system.
 *
 * Sequence Diagrams:
 *  Create MKS - Cobalt/OCDM, Update MKS - Cobalt/OCDM, "Destroy" MKS - Cobalt/OCDM
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
 *  Step 1: Generate license request
 *   Server notifies the client of license request.
 *   Expect that the license request notification is propagated to the client.
 *
 *  Step 3: Update session
 *   updateSession with a key.
 *   Expect that updateSession is propagated to the server.
 *   Api call returns with success.
 *   Server notifies the client of key statuses changed.
 *   Expect that the key statuses changed notification is propagated to the client.
 *
 *  Step 4: Close session
 *   closeSession.
 *   Expect that closeSession is propagated to the server.
 *   Api call returns with success.
 *
 *  Step 5: Destroy media keys
 *   Destroy instance of MediaKeys.
 *   Expect that session is released automatically, if releaseKeySession is not called earlier
 *   Expect that media keys is destroyed on the server.
 *
 * Test Tear-down:
 *  Server is terminated.
 *
 * Expected Results:
 *  For a widevine key system, the application can create new sessions, generate license
 *  requests and update keys ready for decryption of content.
 *
 * Code:
 */
TEST_F(SessionReadyForDecryptionTest, shouldUpdateWidevine)
{
    createMediaKeysWidevine();
    ocdmSessionWillBeCreated();
    createKeySession();

    // Step 1: Generate license request
    willGenerateRequestWidevine();
    generateRequestWidevine();

    // Step 2: process challenge from OCDM
    processChallengeWidevine();

    // Step 3: Update session
    willUpdateSessionWidevine();
    updateSessionWidevine();

    // Step 4: Close session
    willTeardown();
    closeKeySessionWidevine();

    // Step 5: Destroy media keys
    willRelease();
    destroyMediaKeysRequest();
}

/*
 * Component Test: Session initalised and ready for decryption for playready
 * Test Objective:
 *  Test the license generation and updating of a key session for playready media
 *  key system.
 *
 * Sequence Diagrams:
 *  Create MKS - Netflix/OCDM, Update MKS - Netflix/native Rialto, Close - Netflix/native Rialto
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
 *  Step 1: generateRequest
 *      client sends generateRequest message to rialtoServer
 *      rialtoServer passes request to OCDM library
 *      ocdm lib returns success
 *      rialtoServer returns a success message to the client
 *
 *      rialtoServer calls OCDM library get_challenge_data()
 *      rialtoServer should forward this request, via an onLicenceRequest
 *      message, to the client. The content of this message should match
 *      the details from the ocdm library
 *
 *  Step 2: Update session
 *   updateSession with a key.
 *   Expect that updateSession is processed by the server.
 *   Api call returns with success.
 *   Server notifies the client of key statuses changed.
 *   Expect that the key statuses changed notification is processed by the client.
 *
 *  Step 3: Close session
 *   closeSession.
 *   Expect that closeSession is processed by the server.
 *   Api call returns with success.
 *
 *  Step 4: Destroy media keys
 *   Destroy instance of MediaKeys.
 *   Expect that session is released automatically, if releaseKeySession is not called earlier
 *   Expect that media keys is destroyed on the server.
 *
 * Test Tear-down:
 *  Server is terminated.
 *
 * Expected Results:
 *  For a playready key system, the application can create new sessions, generate license
 *  requests and update keys ready for decryption of content.
 *
 * Code:
 */
TEST_F(SessionReadyForDecryptionTest, shouldUpdatNetflix)
{
    createMediaKeysNetflix();
    ocdmSessionWillBeCreated();
    createKeySession();

    // Step 1: generateRequest
    willGenerateRequestNetflix();
    generateRequestNetflix();

    // Step 2: Update session
    willUpdateSessionNetflix();
    updateSessionNetflix();

    // Step 3: Close session
    willCloseKeySessionNetflix();
    closeKeySessionNetflix();

    // Step 4: Destroy media keys
    willRelease();
    destroyMediaKeysRequest();
}

/*
 * Component Test: Session initalised and released correctly
 * Test Objective:
 *  Test if session is released correctly after sending ReleaseKeySession
 *
 * Sequence Diagrams:
 *  Create MKS - Cobalt/OCDM, Update MKS - Cobalt/OCDM, "Destroy" MKS - Cobalt/OCDM
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
 *  Step 1: Generate license request
 *   Server notifies the client of license request.
 *   Expect that the license request notification is propagated to the client.
 *
 *  Step 3: Update session
 *   updateSession with a key.
 *   Expect that updateSession is propagated to the server.
 *   Api call returns with success.
 *   Server notifies the client of key statuses changed.
 *   Expect that the key statuses changed notification is propagated to the client.
 *
 *  Step 4: Close session
 *   closeSession.
 *   Expect that closeSession is propagated to the server.
 *   Api call returns with success.
 *
 *  Step 5: Release session
 *   releaseKeySession
 *   Expect that session is released
 *   Api call returns with success.
 *
 *  Step 6: Destroy media keys
 *   Destroy instance of MediaKeys.
 *   Expect that media keys is destroyed on the server.
 *
 * Test Tear-down:
 *  Server is terminated.
 *
 * Expected Results:
 *  For a widevine key system, the application can create new sessions, generate license
 *  requests and update keys ready for decryption of content.
 *
 * Code:
 */
TEST_F(SessionReadyForDecryptionTest, shouldReleaseKeySession)
{
    createMediaKeysWidevine();
    ocdmSessionWillBeCreated();
    createKeySession();

    // Step 1: Generate license request
    willGenerateRequestWidevine();
    generateRequestWidevine();

    // Step 2: process challenge from OCDM
    processChallengeWidevine();

    // Step 3: Update session
    willUpdateSessionWidevine();
    updateSessionWidevine();

    // Step 4: Close session
    willTeardown();
    closeKeySessionWidevine();

    // Step 5: Release session
    willRelease();
    releaseKeySession();

    // Step 6: Destroy media keys
    destroyMediaKeysRequest();
}
} // namespace firebolt::rialto::server::ct
