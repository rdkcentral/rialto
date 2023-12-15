/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Sky UK
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
class SessionReadyForDecryptionTest : public ClientComponentTest
{
public:
    SessionReadyForDecryptionTest() : ClientComponentTest()
    {
        ClientComponentTest::startApplicationRunning();
    }

    ~SessionReadyForDecryptionTest()
    {
        ClientComponentTest::stopApplication();
    }
};

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
 *  Create a server that handles Control IPC requests.
 *  Initalise the control state to running for this test application.
 *
 * Test Steps:
 *  Step 1: Create a new widevine media keys object
 *   Create an instance of MediaKeys with widevine key system.
 *   Expect that a create media keys is called on the server.
 *   Check that the object returned is valid.
 *
 *  Step 2: Create new key session
 *   Create temporary key session.
 *   Expect that create key session is propagated to the server.
 *   Check that a valid key id is returned.
 *
 *  Step 3: Generate license request
 *   generateRequest.
 *   Expect that generateRequest is propagated to the server.
 *   Api call returns with success.
 *   Server notifys the client of license request.
 *   Expect that the license request notification is propagated to the client.
 *
 *  Step 4: Update session
 *   updateSession with a key.
 *   Expect that updateSession is propagated to the server.
 *   Api call returns with success.
 *   Server notifys the client of key statuses changed.
 *   Expect that the key statuses changed notification is propagated to the client.
 *
 *  Step 5: Close session
 *   closeSession.
 *   Expect that closeSession is propagated to the server.
 *   Api call returns with success.
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
TEST_F(SessionReadyForDecryptionTest, widevine)
{
    // Step 1: Create a new widevine media keys object
    MediaKeysTestMethods::shouldCreateMediaKeysWidevine();
    MediaKeysTestMethods::createMediaKeysWidevine();

    // Step 2: Create new key session
    MediaKeysTestMethods::shouldCreateKeySession();
    MediaKeysTestMethods::createKeySession();

    // Step 3: Generate license request
    MediaKeysTestMethods::shouldGenerateRequest();
    MediaKeysTestMethods::generateRequest();
    MediaKeysTestMethods::shouldNotifyLicenseRequest();
    MediaKeysTestMethods::sendNotifyLicenseRequest();

    // Step 4: Update session
    MediaKeysTestMethods::shouldUpdateSession();
    MediaKeysTestMethods::updateSession();
    MediaKeysTestMethods::shouldNotifyKeyStatusesChanged();
    MediaKeysTestMethods::sendNotifyKeyStatusesChanged();

    // Step 5: Close session
    MediaKeysTestMethods::shouldCloseKeySession();
    MediaKeysTestMethods::closeKeySession();
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
 *  Create a server that handles Control IPC requests.
 *  Initalise the control state to running for this test application.
 *
 * Test Steps:
 *  Step 1: Create a new playready media keys object
 *   Create an instance of MediaKeys with widevine key system.
 *   Expect that a create media keys is called on the server.
 *   Check that the object returned is valid.
 *
 *  Step 2: Create new key session
 *   Create temporary key session.
 *   Expect that create key session is propagated to the server.
 *   Check that a valid key id is returned.
 *
 *  Step 3: Generate license request
 *   generateRequest.
 *   Expect that generateRequest is propagated to the server.
 *   Before replying to the API call notify the client of license request.
 *   Expect that the license request notification is propagated to the client.
 *   Api call returns with success.
 *
 *  Step 4: Update session
 *   updateSession with a key.
 *   Expect that updateSession is propagated to the server.
 *   Api call returns with success.
 *   Server notifys the client of key statuses changed.
 *   Expect that the key statuses changed notification is propagated to the client.
 *
 *  Step 5: Close session
 *   closeSession.
 *   Expect that closeSession is propagated to the server.
 *   Api call returns with success.
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
TEST_F(SessionReadyForDecryptionTest, playready)
{
    // Step 1: Create a new playready media keys object
    MediaKeysTestMethods::shouldCreateMediaKeysPlayready();
    MediaKeysTestMethods::createMediaKeysPlayready();

    // Step 2: Create new key session
    MediaKeysTestMethods::shouldCreateKeySession();
    MediaKeysTestMethods::createKeySession();

    // Step 3: Generate license request
    MediaKeysTestMethods::shouldGenerateRequestAndNotifyLicenseRequest();
    MediaKeysTestMethods::shouldNotifyLicenseRequest();
    MediaKeysTestMethods::generateRequest();
    ClientComponentTest::waitEvent();

    // Step 4: Update session
    MediaKeysTestMethods::shouldUpdateSession();
    MediaKeysTestMethods::updateSession();
    MediaKeysTestMethods::shouldNotifyKeyStatusesChanged();
    MediaKeysTestMethods::sendNotifyKeyStatusesChanged();

    // Step 5: Close session
    MediaKeysTestMethods::shouldCloseKeySession();
    MediaKeysTestMethods::closeKeySession();
}
} // namespace firebolt::rialto::client::ct
