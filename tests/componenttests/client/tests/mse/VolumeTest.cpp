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
class VolumeTest : public ClientComponentTest
{
public:
    double m_volume = 0;
    uint32_t m_volumeDuration = 0;
    firebolt::rialto::EaseType m_easeType = EaseType::EASE_LINEAR;

    VolumeTest() : ClientComponentTest()
    {
        ClientComponentTest::startApplicationRunning();
        MediaPipelineTestMethods::startAudioVideoMediaSessionPrerollPaused();
    }

    ~VolumeTest()
    {
        MediaPipelineTestMethods::endAudioVideoMediaSession();
        ClientComponentTest::stopApplication();
    }
};

/*
 * Component Test: Volume
 * Test Objective:
 *  Test the set and get Volume APIs.
 *
 * Sequence Diagrams:
 *  Volume - https://wiki.rdkcentral.com/display/ASP/Rialto+MSE+Misc+Sequence+Diagrams
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: MediaPipeline
 *
 * Test Initialize:
 *  Create memory region for the shared buffer.
 *  Create a server that handles Control IPC requests.
 *  Initalise the control state to running for this test application.
 *  Initalise a audio video media session paused and prerolled.
 *
 * Test Steps:
 *  Step 1: Set Volume 0.1
 *   SetVolume to 10%.
 *   Expect that SetVolume propagated to the server.
 *
 *  Step 2: Get Volume
 *   GetVolume.
 *   Expect that GetVolume propagated to the server and returns the volume.
 *
 *  Step 3: Set Volume 1
 *   SetVolume to 100%.
 *   Expect that SetVolume propagated to the server.
 *
 *  Step 4: Get Volume
 *   GetVolume.
 *   Expect that GetVolume propagated to the server and returns the volume.
 *
 * Test Teardown:
 *  Terminate the media session.
 *  Memory region created for the shared buffer is closed.
 *  Server is terminated.
 *
 * Expected Results:
 *  Volume can be set and got multiple times without issue.
 *
 * Code:
 */
TEST_F(VolumeTest, volume)
{
    // Step 1: Set Volume 0.1
    m_volume = 0.1;
    m_volumeDuration = 0;
    m_easeType = EaseType::EASE_LINEAR;

    MediaPipelineTestMethods::shouldSetVolume(m_volume, m_volumeDuration, m_easeType);
    MediaPipelineTestMethods::setVolume(m_volume);

    // Step 2: Get Volume
    MediaPipelineTestMethods::shouldGetVolume(m_volume);
    MediaPipelineTestMethods::getVolume(m_volume);

    // Step 3: Set Volume 1.0
    m_volume = 1.0;
    m_volumeDuration = 0;
    m_easeType = EaseType::EASE_LINEAR;
    MediaPipelineTestMethods::shouldSetVolume(m_volume, m_volumeDuration, m_easeType);
    MediaPipelineTestMethods::setVolume(m_volume);

    // Step 4: Get Volume
    MediaPipelineTestMethods::shouldGetVolume(m_volume);
    MediaPipelineTestMethods::getVolume(m_volume);
}
} // namespace firebolt::rialto::client::ct
