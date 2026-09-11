/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026 Sky UK
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
class MediaCapabilitiesTest : public ClientComponentTest
{
public:
    MediaCapabilitiesTest() { ClientComponentTest::startApplicationRunning(); }

    ~MediaCapabilitiesTest() { ClientComponentTest::stopApplication(); }
};

/*
 * Component Test: IMediaCapabilities API
 * Test Objective:
 *  Test the getSupportedAudioCapabilities and getSupportedVideoCapabilities APIs
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: IMediaCapabilities
 *
 * Test Initialize:
 *  Create a server that handles MediaCapabilities IPC requests.
 *  Initialize the control state to running for this test application.
 *
 * Test Steps:
 *  Step 1: Create a MediaCapabilities object.
 *
 *  Step 2: Get the supported audio capabilities
 *   getSupportedAudioCapabilities.
 *   Expect that getSupportedAudioCapabilities is propagated to the server.
 *   Api call returns the supported audio capabilities.
 *   Check supported audio capabilities.
 *
 *  Step 3: Get the supported video capabilities
 *   getSupportedVideoCapabilities.
 *   Expect that getSupportedVideoCapabilities is propagated to the server.
 *   Api call returns the supported video capabilities.
 *   Check supported video capabilities.
 *
 *  Step 4: Destroy MediaCapabilities
 *
 * Test Teardown:
 *  Server is terminated.
 *
 * Expected Results:
 *  Client can get the audio and video capabilities supported by Rialto successfully.
 *
 * Code:
 */

TEST_F(MediaCapabilitiesTest, getSupportedAudioAndVideoCapabilities)
{
    // Step 1: Create a MediaCapabilities object
    createMediaCapabilitiesObject();

    // Step 2: Get the supported audio capabilities
    getSupportedAudioCapabilities();

    // Step 3: Get the supported video capabilities
    getSupportedVideoCapabilities();

    // Step 4: Destroy MediaCapabilities
    destroyMediaCapabilitiesObject();
}

TEST_F(MediaCapabilitiesTest, getSupportedAudioCapabilitiesFailure)
{
    // Step 1: Create a MediaCapabilities object
    createMediaCapabilitiesObject();

    // Step 2: Get the supported audio capabilities - failure case
    getSupportedAudioCapabilitiesFailure();

    // Step 3: Destroy MediaCapabilities
    destroyMediaCapabilitiesObject();
}

TEST_F(MediaCapabilitiesTest, getSupportedVideoCapabilitiesFailure)
{
    // Step 1: Create a MediaCapabilities object
    createMediaCapabilitiesObject();

    // Step 2: Get the supported video capabilities - failure case
    getSupportedVideoCapabilitiesFailure();

    // Step 3: Destroy MediaCapabilities
    destroyMediaCapabilitiesObject();
}
} // namespace firebolt::rialto::client::ct
