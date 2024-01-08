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
class MediaKeysTest : public MediaKeysTestMethods
{
public:
    MediaKeysTest() {}
    virtual ~MediaKeysTest() {}

    void willGenerateRequest();
    void generateRequest();

    void willProcessChallenge();
    void processChallenge();

    void willGenerateRequestFail();
    void generateRequestFail();

    void shouldFailToCreateKeySessionWhenMksIdIsWrong();

    const std::vector<unsigned char> m_kInitData{1, 2, 7};
};

void MediaKeysTest::willGenerateRequest()
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

void MediaKeysTest::generateRequest()
{
    auto request{createGenerateRequestRequest(m_mediaKeysHandle, m_mediaKeySessionId, m_kInitData)};

    ConfigureAction<GenerateRequest>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse([&](const firebolt::rialto::GenerateRequestResponse &resp)
                       { EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::OK); });
}

void MediaKeysTest::willGenerateRequestFail()
{
    EXPECT_CALL(m_ocdmSessionMock, constructSession(KeySessionType::TEMPORARY, InitDataType::CENC, _, m_kInitData.size()))
        .WillOnce(Return(MediaKeyErrorStatus::FAIL));
}

void MediaKeysTest::generateRequestFail()
{
    auto request{createGenerateRequestRequest(m_mediaKeysHandle, m_mediaKeySessionId, m_kInitData)};

    ConfigureAction<GenerateRequest>(m_clientStub)
        .send(request)
        .expectSuccess()
        .matchResponse([&](const firebolt::rialto::GenerateRequestResponse &resp)
                       { EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::FAIL); });
}

void MediaKeysTest::processChallenge()
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

void MediaKeysTest::shouldFailToCreateKeySessionWhenMksIdIsWrong()
{
    auto request{createCreateKeySessionRequest(m_mediaKeysHandle + 1)};
    ConfigureAction<CreateKeySession>(m_clientStub)
        .send(request)
        .expectSuccess() // sick!
        .matchResponse([&](const firebolt::rialto::CreateKeySessionResponse &resp)
                       { EXPECT_EQ(resp.error_status(), ProtoMediaKeyErrorStatus::FAIL); });
}

/*
 * Component Test: Key APIs.
 * Test Objective:
 *    test the validation of the mediaKeysHandle
 *
 * Sequence Diagrams:
 *  Create
 *  https://wiki.rdkcentral.com/pages/viewpage.action?spaceKey=ASP&title=Rialto+Media+Key+Session+Management+Design
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: RialtoApplicationSessionServer with stubs for RialtoClient and RialtoServerManager
 *
 * Test Initialize:
 *   RialtoServerComponentTest::RialtoServerComponentTest() will set up wrappers and
 *      starts rialtoServer running in its own thread
 *   send a createMediaKeys message to rialtoServer
 *
 *
 * Test Steps:
 *  Step 1: Create Media keys
 *          m_mediaKeysHandle becomes a valid handle
 *
 *  Step 2: Attempt to create a Key Session should fail
 *          this should fail because the request doesn't use
 *          a valid mediaKeyHandle
 *
 * Test Teardown:
 *  Server is terminated.
 *
 * Expected Results:
 *  All API calls are handled by the server.
 *
 * Code:
 */
TEST_F(MediaKeysTest, shouldFailToCreateKeySessionWhenMksIdIsWrong)
{
    // Step 1: Create Media keys
    createMediaKeysWidevine();

    // Step 2: Attempt to create a Key Session should fail
    shouldFailToCreateKeySessionWhenMksIdIsWrong();
}

/*
 * Component Test: Key APIs.
 * Test Objective:
 *
 *
 * Sequence Diagrams:
 *   Create MKS OCDM
 *   https://wiki.rdkcentral.com/pages/viewpage.action?spaceKey=ASP&title=Rialto+Media+Key+Session+Management+Design
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: RialtoApplicationSessionServer with stubs for RialtoClient and RialtoServerManager
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
 *  Step 2: process challenge from OCDM
 *       Ocdm library generates a process challenge callback to rialtoServer
 *       rialtoServer should forward this request, via message, to the client.
 *         The content of this message should match the details from the ocdm library
 *
 * Test Teardown:
 *  Server is terminated.
 *
 * Expected Results:
 *  All API calls are handled by the server.
 *
 * Code:
 */
TEST_F(MediaKeysTest, shouldGenerate)
{
    createMediaKeysWidevine();
    ocdmSessionWillBeCreated();
    createKeySession();

    // Step 1: generateRequest
    willGenerateRequest();
    generateRequest();

    // Step 2: process challenge from OCDM
    processChallenge();

    willTeardown();
    willDestruct();
}
/*
 * Component Test: Key APIs.
 * Test Objective:
 *
 *
 * Sequence Diagrams:
 *   Create MKS OCDM
 *   https://wiki.rdkcentral.com/pages/viewpage.action?spaceKey=ASP&title=Rialto+Media+Key+Session+Management+Design
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: RialtoApplicationSessionServer with stubs for RialtoClient and RialtoServerManager
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
 *  Step 1: generateRequest failure
 *      client sends generateRequest message to rialtoServer
 *      rialtoServer passes request to OCDM library
 *      ocdm lib returns an error
 *      rialtoServer returns an error message to the client
 *
 * Test Teardown:
 *  Server is terminated.
 *
 * Expected Results:
 *  All API calls are handled by the server.
 *
 * Code:
 */
TEST_F(MediaKeysTest, generateFail)
{
    createMediaKeysWidevine();
    ocdmSessionWillBeCreated();
    createKeySession();

    // Step 1: generateRequest failure
    willGenerateRequestFail();
    generateRequestFail();
}

} // namespace firebolt::rialto::server::ct
