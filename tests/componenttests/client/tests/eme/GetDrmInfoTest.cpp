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
class GetDrmInfoTest : public ClientComponentTest
{
public:
    GetDrmInfoTest() : ClientComponentTest()
    {
        ClientComponentTest::startApplicationRunning();

        // Create a new playready media keys object
        MediaKeysTestMethods::shouldCreateMediaKeysPlayready();
        MediaKeysTestMethods::createMediaKeysPlayready();
    }

    ~GetDrmInfoTest()
    {
        // Destroy media keys
        MediaKeysTestMethods::shouldDestroyMediaKeys();
        MediaKeysTestMethods::destroyMediaKeys();

        ClientComponentTest::stopApplication();
    }
};

/*
 * Component Test: Get various infomation stored in the system.
 * Test Objective:
 *  Test the getLdlSessionsLimit & getDrmTime APIs.
 *
 * Sequence Diagrams:
 *  Get LDL Session Limit, Get DRM Time
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
 *  Step 1: Get the ldl session limit
 *   getLdlSessionsLimit.
 *   Expect that getLdlSessionsLimit is propagated to the server.
 *   Api call returns with success.
 *   Check ldl session limit.
 *
 *  Step 2: Get the drm time
 *   getDrmTime.
 *   Expect that getDrmTime is propagated to the server.
 *   Api call returns with success.
 *   Check drm time.
 *
 * Test Teardown:
 *  Destroy MediaKeys.
 *  Server is terminated.
 *
 * Expected Results:
 *  Client can get both the ldl session limit and DRM time from the MediaKeys object.
 *
 * Code:
 */
TEST_F(GetDrmInfoTest, getApis)
{
    // Step 1: Get the ldl session limit
    MediaKeysTestMethods::shouldgetLdlSessionsLimit();
    MediaKeysTestMethods::getLdlSessionsLimit();

    // Step 2: Get the drm time
    MediaKeysTestMethods::shouldGetDrmTime();
    MediaKeysTestMethods::getDrmTime();
}
} // namespace firebolt::rialto::client::ct
