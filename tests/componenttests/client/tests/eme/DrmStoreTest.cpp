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
class DrmStoreTest : public ClientComponentTest
{
public:
    DrmStoreTest() : ClientComponentTest() 
    { 
        ClientComponentTest::startApplicationRunning(); 
        ClientComponentTest::initaliseWidevineMediaKeySession(); 
    }

    ~DrmStoreTest() 
    { 
        ClientComponentTest::terminateMediaKeySession();
        ClientComponentTest::stopApplication(); 
    }
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
 *  Initalise MediaKeys object ready for decryption.
 *
 * Test Steps:
 *  Step 1: Get the drm store
 *   getDrmStoreHash.
 *   Expect that getDrmStoreHash is propagated to the server.
 *   Api call returns with success.
 *   Check drm store hash.
 *
 *  Step 2: Delete the drm store
 *   deleteDrmStore.
 *   Expect that deleteDrmStore is propagated to the server.
 *   Api call returns with success.
 *
 *  Step 3: Get the drm store failure
 *   getDrmStoreHash.
 *   Expect that getDrmStoreHash is propagated to the server.
 *   Api call returns with failure.
 *
 * Test Teardown:
 *  Terminate MediaKeys.
 *  Server is terminated.
 *
 * Expected Results:
 *  Client can get and delete the drm store successfully.
 *
 * Code:
 */
TEST_F(DrmStoreTest, getAndDelete)
{
    // Step 1: Get the drm store
    // Step 2: Delete the drm store
    // Step 3: Get the drm store failure
}
} // namespace firebolt::rialto::client::ct
