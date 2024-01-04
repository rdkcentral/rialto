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
class GetSessionInfoTest : public MediaKeysTestMethods
{
public:
    GetSessionInfoTest() {}
    virtual ~GetSessionInfoTest() {}
};

/*
 * Component Test: Get various infomation stored in the session.
 * Test Objective:
 *  Test the getLastDrmError & getCdmKeySessionId APIs.
 *
 * Sequence Diagrams:
 *  Get Last DRM Error, Get CDM Key Session Id
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
 *  Create a MediaKeySession.
 *
 * Test Steps:
 *  Step 1: Get the cdm key session id
 *   Expect that getCdmKeySessionId is processed by the server.
 *   Api call returns with success.
 *   Check cdm key session id.
 *
 *  Step 2: Get the last drm error
 *   Expect that getLastDrmError is processed by the server.
 *   Api call returns with success.
 *   Check error code.
 *
 * Test Teardown:
 *  Close session.
 *  Destroy MediaKeys.
 *  Server is terminated.
 *
 * Expected Results:
 *  Client can get both the cdm key session id and last drm error from the media key session.
 *
 * Code:
 */
TEST_F(GetSessionInfoTest, getApis)
{
    createMediaKeysNetflix();
    ocdmSessionWillBeCreated();
    createKeySession();

    // Step 1: Get the cdm key session id
    getCdmKeySessionId();

    // Step 2: Get the last drm error
    getLastDrmError();
}

} // namespace firebolt::rialto::server::ct
