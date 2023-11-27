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

class AudioVideoPlaybackTest : public ClientComponentTest
{
public:
    AudioVideoPlaybackTest()
        : ClientComponentTest()
    {
        // Test Initialize 
        ControlTestMethods::createControl();
        ControlTestMethods::shouldRegisterClient();
        ControlTestMethods::registerClient();
        ControlTestMethods::shouldNotifyApplicationStateInactive();
        ControlTestMethods::sendNotifyApplicationStateInactive();
        ControlTestMethods::shouldNotifyApplicationStateRunning();
        ControlTestMethods::sendNotifyApplicationStateRunning();
    }
};

/*
 * Component Test:
 * Test Objective:
 * 
 * 
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: 
 *
 * Test Initialize:
 *  
 *  
 *
 * Test Steps:
 *  Step 1: 
 * 
 * Test Teardown:
 *  
 * 
 *
 * Expected Results:
 * 
 *  
 *
 * Code:
 */
TEST_F(AudioVideoPlaybackTest, playback)
{
}
