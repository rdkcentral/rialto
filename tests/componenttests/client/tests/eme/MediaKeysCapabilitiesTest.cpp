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
class MediaKeysCapabiltiesTest : public ClientComponentTest
{
public:
    MediaKeysCapabiltiesTest() : ClientComponentTest()
    {
        ClientComponentTest::startApplicationRunning();
        MediaKeysTestMethods::initaliseWidevineMediaKeySession();
    }

    ~MediaKeysCapabiltiesTest()
    {
        MediaKeysTestMethods::terminateMediaKeySession();
        ClientComponentTest::stopApplication();
    }
};
/*
 * Component Test: MediaKeyCapabilities API
 * Test Objective:
 *  Test the getSupportedKeySystems, supportKeySystem and getSupportedKeySystemVersion APIs
 *
 * Sequence Diagrams:
 *  Check Supported Key Systems - indicative use of Rialto and Get DRM Version - Netflix/native Rialto
 *                              - https://wiki.rdkcentral.com/pages/viewpage.action?pageId=226375556
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: MediaKeysCapabilities
 *
 * Test Initialize:
 *  Create a server that handles Control IPC requests.
 *  Initalise the control state to running for this test application.
 *  Create a MediaKeysCabilities object.
 *
 * Test Steps:
 *  Step 1: Get the supported key systems
 *   getSupportedKeySystems.
 *   Expect that getSupportedKeySystems is propagated to the server.
 *   Api call return the supported key systems.
 *   Check supported key systems.
 *
 *  Step 2: Check if key is supported - success
 *   supportsKeySystem.
 *   Expect that supportsKeySystem is propagated to server.
 *   Api call returns with success.
 *   Check if key is supported
 *
 *  Step 3: Check if key is supported - failure
 *   supportsKeySystem.
 *   Expect that supportsKeySystem is propagated to server.
 *   Api call returns with failure.
 *
 *  Step 4: Get the supported key system version - success
 *   getSupportedKeySystemVersion.
 *   Expect that supportsKeySystem is propagated to server.
 *   Api call returns with success.
 *   Check if version is supported for the key system
 *
 *  Step 5: Get the supported key system version - failure
 *   getSupportedKeySystemVersion.
 *   Expect that supportsKeySystem is propagated to server.
 *   Api call returns with failure.
 *
 * Test Teardown:
 *  Destroy MediaKeysCabilities .
 *  Server is terminated.
 *
 * Expected Results:
 *  Client can get and check the EME key systems supported by Rialto successfully.
 *
 * Code:
 */

TEST_F(MediaKeysCapabilitiesTest, checkSupportedKeySystems)
{
    // Step 1: Get the supported key systems
    MediaKeysTestMethods::shouldGetSupportedKeySystems();
    MediaKeysTestMethods::getSupportedKeySystems();

    // Step 2: Check if key is supported - success
    MediaKeysTestMethods::shouldSupportKeySystems();
    MediaKeysTestMethods::supportsKeySystem();

    // Step 3: Check if key is supported - failure
    MediaKeysTestMethods::shouldNotSupportKeySystems();
    MediaKeysTestMethods::doesNotsupportsKeySystem();

    // Step 4: Get the supported key system version - success
    MediaKeysTestMethods::shouldGetSupportedKeySystemVersion();
    MediaKeysTestMethods::getSupportedKeySystemVersion()

        // Step 5: Get the supported key system version - failure
        MediaKeysTestMethods::shouldNotGetSupportedKeySystemVersion();
    MediaKeysTestMethods::doesNotGetSupportedKeySystemVersion();
}
} // namespace firebolt::rialto::client::ct