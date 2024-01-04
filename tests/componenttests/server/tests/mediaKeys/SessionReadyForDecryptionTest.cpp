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
class SessionReadyForDecryptionTest : public MediaKeysTestMethods
{
public:
    SessionReadyForDecryptionTest() {}
    virtual ~SessionReadyForDecryptionTest() {}
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
 *  Step 6: Destroy media keys
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
    // Step 1: Create a new widevine media keys object
    createMediaKeysWidevine();
    ocdmSessionWillBeCreated();

    // Step 2: Create new key session
    createKeySession();

    // Step 3: Generate license request
    licenseRenewal();

    // Step 4: Update session
    updateSessionWidevine();

    // Step 5: Close session
    closeKeySessionWidevine();

    // Step 6: Destroy media keys
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
 *   Expect that create key session is processed by the server.
 *   Check that a valid key id is returned.
 *
 *  Step 3: Generate license request
 *   generateRequest.
 *   Expect that generateRequest is processed by the server.
 *   Before replying to the API call notify the client of license request.
 *   Expect that the license request notification is processed by the client.
 *   Api call returns with success.
 *
 *  Step 4: Update session
 *   updateSession with a key.
 *   Expect that updateSession is processed by the server.
 *   Api call returns with success.
 *   Server notifys the client of key statuses changed.
 *   Expect that the key statuses changed notification is processed by the client.
 *
 *  Step 5: Close session
 *   closeSession.
 *   Expect that closeSession is processed by the server.
 *   Api call returns with success.
 *
 *  Step 6: Destroy media keys
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
    // Step 1: Create a new playready media keys object
    createMediaKeysNetflix();
    ocdmSessionWillBeCreated();

    // Step 2: Create new key session
    createKeySession();

    // Step 3: Generate license request
    licenseRenewal();

    // Step 4: Update session
    updateSessionNetflix();

    // Step 5: Close session
    closeKeySessionNetflix();

    // Step 6: Destroy media keys
    destroyMediaKeysRequest();
}

} // namespace firebolt::rialto::server::ct
