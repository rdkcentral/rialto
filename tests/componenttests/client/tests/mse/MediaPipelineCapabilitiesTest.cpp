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
class MediaPipelineCapabilitiesTest : public ClientComponentTest
{
public:
    MediaPipelineCapabilitiesTest() : ClientComponentTest() { ClientComponentTest::startApplicationRunning(); }

    ~MediaPipelineCapabilitiesTest() { ClientComponentTest::stopApplication(); }
};
/*
 * Component Test: MediaPipelineCapabilities API
 * Test Objective:
 *  Test the getSupportedMimeTypes and isMimeTypeSupported APIs
 *
 * Sequence Diagrams:
 *  Capabilities - Supported MIME types
 *               -
 * https://wiki.rdkcentral.com/display/ASP/Rialto+MSE+Misc+Sequence+Diagrams#RialtoMSEMiscSequenceDiagrams-SupportedMIMETypes
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: MediaPipelineCapabilities
 *
 * Test Initialize:
 *  Create a server that handles Control IPC requests.
 *  Initalise the control state to running for this test application.
 *
 * Test Steps:
 *  Step 1: Create a MediaPipelineCabilities object.
 *
 *  Step 2: Get the supported audio mime types
 *   getSupportedMimeTypes.
 *   Expect that getSupportedMimeTypes is propagated to the server.
 *   Api call return the supported audio mime types.
 *   Check supported audio mime types.
 *
 *  Step 3: Get the supported video mime types
 *   getSupportedMimeTypes.
 *   Expect that getSupportedMimeTypes is propagated to the server.
 *   Api call return the supported video mime types.
 *   Check supported video mime types.
 *
 *  Step 4: Get the unknown mime types
 *   getSupportedMimeTypes.
 *   Expect that getSupportedMimeTypes is propagated to the server.
 *   Api call return the unknown mime types.
 *   Check unknown mime types.
 *
 *  Step 5: Check if mime type is supported - success
 *   isMimeTypeSupported.
 *   Expect that isMimeTypeSupported is propagated to server.
 *   Api call returns with success.
 *   Check if mime type is supported
 *
 *  Step 6: Check if mime type is supported - failure
 *   isMimeTypeSupported.
 *   Expect that isMimeTypeSupported is propagated to server.
 *   Api call returns with failure.
 *
 *  Step 7: Destroy MediaPipelineCapabilities
 *
 * Test Teardown:
 *  Server is terminated.
 *
 * Expected Results:
 *  Client can get and check the MSE mime types supported by Rialto successfully.
 *
 * Code:
 */

TEST_F(MediaPipelineCapabilitiesTest, checkSupportedMimeTypes)
{
    // Step 1: Create a MediaPipelineCabilities object.
    MediaKeysTestMethods::createMediaPipelineCapabilitiesObject();

    // Step 2: Get the supported audio mime types
    MediaKeysTestMethods::shouldGetSupportedAudioMimeTypes();
    MediaKeysTestMethods::getSupportedAudioMimeTypes();

    // Step 3: Get the supported video mime types
    MediaKeysTestMethods::shouldGetSupportedVideoMimeTypes();
    MediaKeysTestMethods::getSupportedVideoMimeTypes();

    // Step 4: Get the unknown mime types
    MediaKeysTestMethods::shouldGetSupportedUnknownMimeTypes();
    MediaKeysTestMethods::getUnknownMimeTypes();

    // Step 5: Check if mime type is supported - success
    MediaKeysTestMethods::shouldCheckIsMimeTypeSupported();
    MediaKeysTestMethods::isMimeTypeSupported();

    // Step 6: Check if mime type is supported - failure
    MediaKeysTestMethods::shouldCheckIsMimeTypeNotSupported();
    MediaKeysTestMethods::isMimeTypeNotSupported();

    // Step 7: Destroy MediaPipelineCapabilities
    MediaKeysTestMethods::destroyMediaPipelineCapabilitiesObject();
}
} // namespace firebolt::rialto::client::ct
