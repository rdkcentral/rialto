/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 Sky UK
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
class GetMetricSystemDataTest : public ClientComponentTest
{
public:
    GetMetricSystemDataTest() : ClientComponentTest()
    {
        ClientComponentTest::startApplicationRunning();

        // Create a new widevine media keys object
        MediaKeysTestMethods::shouldCreateMediaKeysWidevine();
        MediaKeysTestMethods::createMediaKeysWidevine();
    }

    ~GetMetricSystemDataTest()
    {
        // Destroy media keys
        MediaKeysTestMethods::shouldDestroyMediaKeys();
        MediaKeysTestMethods::destroyMediaKeys();

        ClientComponentTest::stopApplication();
    }
};

/*
 * Component Test: Get metric system data API.
 * Test Objective:
 *  Test the getMetricSystemData API.
 *
 * Sequence Diagrams:
 *  Get Metric System Data
 *   - 
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
 * 
 * Step 1: Get the metric system data
 * getMetricSystemData.
 * Expect that getMetricSystemData is propagated to the server.
 * Api call returns with success.
 * Check metric system data.
 * 
 * Step 2: Get the metric system data failure
 * getMetricSystemData.
 * Expect that getMetricSystemData is propagated to the server.
 * Api call returns with failure.
 * 
 * Step 3: Get the metric system data (interface not implemented)
 * getMetricSystemData.
 * Expect that getMetricSystemData is propagated to the server.
 * Api call returns with interface not implemented.
 * 
 * Test Tear-down:
 * Destroy MediaKeys.
 * Server is terminated.
 *  
 * Expected Results:
 *  Client can get the metric system data successfully.
*/
TEST_F(GetMetricSystemDataTest, getApi)
{

    // Step 1: Get the metric system data
    MediaKeysTestMethods::shouldGetMetricSystemData();
    MediaKeysTestMethods::getMetricSystemData();

    // Step 2: Get the metric system data failure
    MediaKeysTestMethods::shouldFailToGetMetricSystemData();
    MediaKeysTestMethods::getMetricSystemDataFailure();

    // Step 3: Get the metric system data (interface not implemented)

}
} // namespace firebolt::rialto::client::ct