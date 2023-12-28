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
class LicenseRenewalTest : public ClientComponentTest
{
public:
    LicenseRenewalTest() : ClientComponentTest() 
    { 
        ClientComponentTest::startApplicationRunning(); 
        ClientComponentTest::initaliseMediaKeySession(); 
    }

    ~LicenseRenewalTest() 
    { 
        ClientComponentTest::terminateMediaKeySession();
        ClientComponentTest::stopApplication(); 
    }
};

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
 *  Create a server that handles Control IPC requests.
 *  Initalise the control state to running for this test application.
 *  Initalise MediaKeys object ready for decryption.
 *
 * Test Steps:
 *  Step 1: Notify license renewal
 *   Server notifys the client that of license renewal.
 *   Expect that the license renewal notification is propagated to the client.
 *
 *  Step 2: Update session
 *   updateSession with a the updated license.
 *   Expect that updateSession is propagated to the server.
 *   Api call returns with success.
 *
 *  Step 3: Notify key statuses changed
 *   Server notifys the client of key statuses changed.
 *   Expect that the key statuses changed notification is propagated to the client.
 *
 * Test Teardown:
 *  Terminate MediaKeys.
 *  Server is terminated.
 *
 * Expected Results:
 *  Client can be notified of license renewal and update the key session successfully.
 *
 * Code:
 */
TEST_F(LicenseRenewalTest, licenseRenewal)
{
    // Step 1: Notify license renewal
    MediaKeysTestMethods::shouldNotifyLicenseRenewal();
    MediaKeysTestMethods::sendNotifyLicenseRenewal();

    // Step 2: Update session
    MediaKeysTestMethods::shouldUpdateSessionRenewal();
    MediaKeysTestMethods::updateSessionRenewal();

    // Step 3: Notify key statuses changed
    MediaKeysTestMethods::shouldNotifyKeyStatusesChanged();
    MediaKeysTestMethods::sendNotifyKeyStatusesChanged();
}
} // namespace firebolt::rialto::client::ct
