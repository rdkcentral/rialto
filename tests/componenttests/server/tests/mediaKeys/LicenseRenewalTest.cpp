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
class LicenseRenewalTest : public virtual MediaKeysTestLicenceRenewal, public virtual MediaKeysTestUpdateSessionNetflix
{
public:
    LicenseRenewalTest() {}
    virtual ~LicenseRenewalTest() {}
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
 *   Expect that the license renewal notification is processed by the client.
 *
 *  Step 2: Update session
 *   updateSession with the updated license.
 *   Expect that updateSession is processed by the server.
 *   Api call returns with success.
 *
 *  Step 3: Notify key statuses changed
 *   Server notifys the client of key statuses changed.
 *   Expect that the key statuses changed notification is processed by the client.
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
    createMediaKeysNetflix();
    ocdmSessionWillBeCreated();
    createKeySession();

    // Step 1: Notify license renewal
    willLicenseRenew();
    licenseRenew();

    // Step 2: Update session
    willUpdateSessionNetflix();
    updateSessionNetflix();

    // Step 3: Notify key statuses changed
    updateOneKey();
    updateAllKeys();
}

} // namespace firebolt::rialto::server::ct
