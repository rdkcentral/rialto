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
class SetDrmHeaderTest : public MediaKeysTestMethods
{
public:
    SetDrmHeaderTest() {}
    virtual ~SetDrmHeaderTest() {}
};

/*
 * Component Test: Set Drm Header API.
 * Test Objective:
 *  Test the setDrmHeader API.
 *
 * Sequence Diagrams:
 *  Set DRM Header
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
 *  Step 1: Set the drm header
 *   setDrmHeader first header.
 *   Expect that setDrmHeader is processed by the server.
 *   Api call returns with success.
 *
 *  Step 2: Set the drm header for a second time with different header
 *   setDrmHeader second header.
 *   Expect that setDrmHeader is processed by the server.
 *   Api call returns with success.
 *
 * Test Teardown:
 *  Close session.
 *  Destroy MediaKeys.
 *  Server is terminated.
 *
 * Expected Results:
 *  Client can set the drm header at will.
 *
 * Code:
 */
TEST_F(SetDrmHeaderTest, multiple)
{
    createMediaKeysNetflix();
    ocdmSessionWillBeCreated();
    createKeySession();

    // Step 1: Set the drm header
    setDrmHeader();

    // Step 2: Set the drm header for a second time with different header
    setDrmHeader2();
}

} // namespace firebolt::rialto::server::ct
