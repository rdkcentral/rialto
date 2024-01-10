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

    void willGenerateRequestFail();
    void generateRequestFail();

    void shouldFailToCreateKeySessionWhenMksIdIsWrong();

    const std::vector<unsigned char> m_kInitData{1, 2, 7};
};

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
 *    test the validation of the mediaKeysHandle when sending a CreateKeySession
 *    request to rialtoServer. To accomplish this we simply pass an invalid
 *    handle in the message
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
 * Test Tear-down:
 *  Server is terminated.
 *
 * Expected Results:
 *  The rialtoServer should respond with a FAIL in the response status
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
 *   To test the rialtoServer component in the sequence diagram below
 *   with OCDM functionality failing (this could be due to incorrect request
 *   data being sent from the client to rialtoServer, for example)
 *   A session is created via the usual initialization and then
 *   generateRequest should be passed to OCDM. OCDM mock returns a fail
 *   status
 *
 * Sequence Diagrams:
 *   Create MKS - Cobalt/OCDM
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
 * Test Tear-down:
 *  Server is terminated.
 *
 * Expected Results:
 *  The fail status returned by the OCDM mock should be echoed back
 *  to the rialto client (the client should receive a response message
 *  from rialtoServer with FAIL in the error_status)
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
