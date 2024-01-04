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
class KeyApisTest : public ClientComponentTest
{
public:
    KeyApisTest() : ClientComponentTest()
    {
        ClientComponentTest::startApplicationRunning();
        ClientComponentTest::initaliseWidevineMediaKeySession();
    }

    ~KeyApisTest()
    {
        ClientComponentTest::terminateMediaKeySession();
        ClientComponentTest::stopApplication();
    }
};

/*
 * Component Test: Key APIs.
 * Test Objective:
 *  Test the key session management apis containsKey, loadKeySession & removeKeySession,
 *  that are not tested in the normal media key session sequence.
 *
 * Sequence Diagrams:
 *  Contains Key, Load/OCDM, Remove/OCDM
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
 *  Initalise MediaKeys object ready for decryption.
 *
 * Test Steps:
 *  Step 1: Close session
 *   closeKeySession.
 *   Expect that closeKeySession is propagated to the server.
 *   Api call returns with success.
 *
 *  Step 2: Load session
 *   loadSession of the closed session.
 *   Expect that loadSession is propagated to the server.
 *   Api call returns with success.
 *
 *  Step 3: Does not contain key
 *   containsKey with key id that has not been set.
 *   Expect that containsKey is propagated to the server.
 *   containsKey returns false.
 *
 *  Step 4: Does contain key
 *   containsKey with key id that has been selected.
 *   Expect that containsKey is propagated to the server.
 *   containsKey returns true.
 *
 *  Step 5: Remove session
 *   removeKeySession.
 *   Expect that removeKeySession is propagated to the server.
 *   Api call returns with success.
 *
 * Test Teardown:
 *  Terminate MediaKeys.
 *  Server is terminated.
 *
 * Expected Results:
 *   Load a closed key session successfully.
 *   Query if keys exist or not in the session using containsKey.
 *   Remove key session successfully.
 *
 * Code:
 */
TEST_F(KeyApisTest, keyManagement)
{
    // Step 1: Close session
    MediaKeysTestMethods::shouldCloseKeySession();
    MediaKeysTestMethods::closeKeySession();

    // Step 2: Load session
    MediaKeysTestMethods::shouldLoadSession();
    MediaKeysTestMethods::loadSession();

    // Step 3: Does not contain key
    MediaKeysTestMethods::shouldNotContainKey();
    MediaKeysTestMethods::doesNotContainKey();

    // Step 4: Does contain key
    MediaKeysTestMethods::shouldContainsKey();
    MediaKeysTestMethods::containsKey();

    // Step 5: Remove session
    MediaKeysTestMethods::shouldRemoveKeySession();
    MediaKeysTestMethods::removeKeySession();
}
} // namespace firebolt::rialto::client::ct
