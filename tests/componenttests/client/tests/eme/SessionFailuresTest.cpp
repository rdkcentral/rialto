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

#include "ClientComponentTest.h"
#include <gtest/gtest.h>

namespace firebolt::rialto::client::ct
{
class SessionFailuresTest : public ClientComponentTest
{
public:
    SessionFailuresTest() : ClientComponentTest()
    {
        ClientComponentTest::startApplicationRunning();

        // Create a new widevine media keys object
        MediaKeysTestMethods::shouldCreateMediaKeysWidevine();
        MediaKeysTestMethods::createMediaKeysWidevine();
    }

    ~SessionFailuresTest()
    {
        // Destroy media keys
        MediaKeysTestMethods::shouldDestroyMediaKeys();
        MediaKeysTestMethods::destroyMediaKeys();

        ClientComponentTest::stopApplication();
    }
};

/*
 * Component Test: Failure scenarios during setup of media key session.
 * Test Objective:
 *  Test the failure scenarios that can occur while a session is created, license is requested
 *  and updating of the session.
 *
 * Sequence Diagrams:
 *  Create MKS - Cobalt/OCDM, Update MKS - Cobalt/OCDM, Close - Cobalt/OCDM, "Destroy" MKS - Cobalt/OCDM
 *   - https://wiki.rdkcentral.com/display/ASP/Rialto+Media+Key+Session+Management+Design
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: MediaKeys
 *
 * Test Initialize:
 *  Create a server that handles Control IPC requests.
 *  Initalise the control state to running for this test application.
 *  Create a MediaKeys object.
 *
 * Test Steps:
 *  Step 1: Create new key session failure
 *   Create temporary key session.
 *   Expect that create key session is propagated to the server.
 *   Set media key status failed in the response.
 *   Api call returns with failure.
 *
 *  Step 2: Create new key session
 *   Create temporary key session.
 *   Expect that create key session is propagated to the server.
 *   Check that a valid key id is returned.
 *
 *  Step 3: Generate request failure
 *   generateRequest.
 *   Expect that generateRequest is propagated to the server.
 *   Set media key status failed in the response.
 *   Api call returns with failure.
 *
 *  Step 4: Generate license request
 *   generateRequest.
 *   Expect that generateRequest is propagated to the server.
 *   Api call returns with success.
 *   Server notifys the client of license request.
 *   Expect that the license request notification is propagated to the client.
 *
 *  Step 5: Update session failure
 *   updateSession.
 *   Expect that updateSession is propagated to the server.
 *   Set media key status failed in the response.
 *   Api call returns with failure.
 *
 *  Step 6: Update session
 *   updateSession with a key.
 *   Expect that updateSession is propagated to the server.
 *   Api call returns with success.
 *   Server notifys the client of key statuses changed.
 *   Expect that the key statuses changed notification is propagated to the client.
 *
 *  Step 7: Close session failure
 *   closeSession.
 *   Expect that closeSession is propagated to the server.
 *   Set media key status failed in the response.
 *   Api call returns with failure.
 *
 *  Step 8: Close session
 *   closeSession.
 *   Expect that closeSession is propagated to the server.
 *   Api call returns with success.
 *
 * Test Teardown:
 *  Destroy MediaKeys.
 *  Server is terminated.
 *
 * Expected Results:
 *  APIs called during the initalisation of a media key session fail gracefully and are recoverable.
 *
 * Code:
 */
TEST_F(SessionFailuresTest, failures)
{
    // Step 1: Create new key session failure
    MediaKeysTestMethods::shouldCreateKeySessionFailure();
    MediaKeysTestMethods::createKeySessionFailure();

    // Step 2: Create new key session
    MediaKeysTestMethods::shouldCreateKeySession();
    MediaKeysTestMethods::createKeySession();

    // Step 3: Generate request failure
    MediaKeysTestMethods::shouldGenerateRequestFailure();
    MediaKeysTestMethods::generateRequestFailure();

    // Step 4: Generate license request
    MediaKeysTestMethods::shouldGenerateRequest();
    MediaKeysTestMethods::generateRequest();
    MediaKeysTestMethods::shouldNotifyLicenseRequest();
    MediaKeysTestMethods::sendNotifyLicenseRequest();

    // Step 5: Update session failure
    MediaKeysTestMethods::shouldUpdateSessionFailure();
    MediaKeysTestMethods::updateSessionFailure();

    // Step 6: Update session
    MediaKeysTestMethods::shouldUpdateSession();
    MediaKeysTestMethods::updateSession();
    MediaKeysTestMethods::shouldNotifyKeyStatusesChanged();
    MediaKeysTestMethods::sendNotifyKeyStatusesChanged();

    // Step 7: Close session failure
    MediaKeysTestMethods::shouldCloseKeySessionFailure();
    MediaKeysTestMethods::closeKeySessionFailure();

    // Step 8: Close session
    MediaKeysTestMethods::shouldCloseKeySession();
    MediaKeysTestMethods::closeKeySession();
}
} // namespace firebolt::rialto::client::ct
