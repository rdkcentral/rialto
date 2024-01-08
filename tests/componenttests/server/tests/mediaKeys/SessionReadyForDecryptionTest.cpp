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

    void willUpdateSessionWidevine();
    void updateSessionWidevine();
    void closeKeySessionWidevine();

    void willCloseKeySessionNetflix();
    void closeKeySessionNetflix();

    void destroyMediaKeysRequest();

    const std::vector<unsigned char> kResponse{4, 1, 3};
};

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
 *  Step 2: Update session
 *   updateSession with a key.
 *   Expect that updateSession is propagated to the server.
 *   Api call returns with success.
 *   Server notifys the client of key statuses changed.
 *   Expect that the key statuses changed notification is propagated to the client.
 *
 *  Step 3: Close session
 *   closeSession.
 *   Expect that closeSession is propagated to the server.
 *   Api call returns with success.
 *
 *  Step 4: Destroy media keys
 *   Destroy instance of MediaKeys.
 *   Expect that media keys is destroyed on the server.
 *
 * Test Teardown:
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
    licenseRenew();

    // Step 2: Update session
    willUpdateSessionWidevine();
    updateSessionWidevine();

    // Step 3: Close session
    willTeardown();
    willDestruct();
    closeKeySessionWidevine();

    // Step 4: Destroy media keys
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
 *  Step 1: Generate license request
 *   Expect that the license request notification is processed by the client.
 *   Api call returns with success.
 *
 *  Step 2: Update session
 *   updateSession with a key.
 *   Expect that updateSession is processed by the server.
 *   Api call returns with success.
 *   Server notifys the client of key statuses changed.
 *   Expect that the key statuses changed notification is processed by the client.
 *
 *  Step 3: Close session
 *   closeSession.
 *   Expect that closeSession is processed by the server.
 *   Api call returns with success.
 *
 *  Step 4: Destroy media keys
 *   Destroy instance of MediaKeys.
 *   Expect that media keys is destroyed on the server.
 *
 * Test Teardown:
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

    // Step 1: Generate license request
    licenseRenew();

    // Step 2: Update session
    willUpdateSessionNetflix();
    updateSessionNetflix();

    // Step 3: Close session
    willDestruct();
    willCloseKeySessionNetflix();
    closeKeySessionNetflix();

    // Step 4: Destroy media keys
    destroyMediaKeysRequest();
}

} // namespace firebolt::rialto::server::ct
