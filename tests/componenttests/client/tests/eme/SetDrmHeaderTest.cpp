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
class SetDrmHeaderTest : public ClientComponentTest
{
public:
    SetDrmHeaderTest() : ClientComponentTest()
    {
        ClientComponentTest::startApplicationRunning();

        // Create a new playready media keys object
        MediaKeysTestMethods::shouldCreateMediaKeysPlayready();
        MediaKeysTestMethods::createMediaKeysPlayready();

        // Create new key session
        MediaKeysTestMethods::shouldCreateKeySession();
        MediaKeysTestMethods::createKeySession();
    }

    ~SetDrmHeaderTest()
    {
        // Close session
        MediaKeysTestMethods::shouldCloseKeySession();
        MediaKeysTestMethods::closeKeySession();

        // Destroy media keys
        MediaKeysTestMethods::shouldDestroyMediaKeys();
        MediaKeysTestMethods::destroyMediaKeys();

        ClientComponentTest::stopApplication();
    }
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
 *   Expect that setDrmHeader is propagated to the server.
 *   Api call returns with success.
 *
 *  Step 2: Set the drm header for a second time with different header
 *   setDrmHeader second header.
 *   Expect that setDrmHeader is propagated to the server.
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
    // Step 1: Set the drm header
    MediaKeysTestMethods::shouldSetDrmHeader();
    MediaKeysTestMethods::setDrmHeader();

    // Step 2: Set the drm header for a second time with different header
    MediaKeysTestMethods::shouldSetDrmHeaderSecond();
    MediaKeysTestMethods::setDrmHeaderSecond();
}
} // namespace firebolt::rialto::client::ct
