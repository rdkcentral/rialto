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
class DrmStoreTest : public MediaKeysTestMethods
{
public:
    DrmStoreTest() {}
    virtual ~DrmStoreTest() {}
};

/*
 * Component Test: Drm Store APIs.
 * Test Objective:
 *  Test the deleteDrmStore and getDrmStoreHash APIs.
 *
 * Sequence Diagrams:
 *  Delete DRM Store, Get DRM Store Hash
 *   - https://wiki.rdkcentral.com/display/ASP/Rialto+EME+Misc+Design
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
 *  Step 1: Get the drm store
 *   Expect that getDrmStoreHash is processed by the server.
 *   Api call returns with success
 *   Check drm store hash.
 *
 *  Step 2: Get the drm store failure
 *   Expect that getDrmStoreHash is processed by the server.
 *   Api call returns with failure.
 *
 *  Step 3: Delete the drm store
 *   Expect that deleteDrmStore is processed by the server.
 *   Api call returns with success.
 *
 * Test Teardown:
 *  Destroy MediaKeys.
 *  Server is terminated.
 *
 * Expected Results:
 *  Client can get and delete the drm store successfully.
 *
 * Code:
 */
TEST_F(DrmStoreTest, shouldDrmstore)
{
    createMediaKeysNetflix();
    ocdmSessionWillBeCreated();
    createKeySession();

    // Step 1: Get the drm store
    getDrmStoreHashRequest();

    // Step 2: Get the drm store failure
    getDrmStoreHashRequestFails();

    // Step 3: Delete the drm store
    deleteDrmStoreRequest();
}

} // namespace firebolt::rialto::server::ct
