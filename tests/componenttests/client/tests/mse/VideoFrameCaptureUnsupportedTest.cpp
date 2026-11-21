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
#include "IVideoFrameCapture.h"
#include <gtest/gtest.h>

namespace firebolt::rialto::client::ct
{
class VideoFrameCaptureUnsupportedTest : public ClientComponentTest
{
public:
    VideoFrameCaptureUnsupportedTest() : ClientComponentTest()
    {
        ClientComponentTest::startApplicationRunning();
        MediaPipelineTestMethods::startAudioVideoMediaSessionPrerollPaused();
    }

    ~VideoFrameCaptureUnsupportedTest()
    {
        MediaPipelineTestMethods::endAudioVideoMediaSession();
        ClientComponentTest::stopApplication();
    }
};

/*
 * Component Test: Video frame capture against a server without capture support
 * Test Objective:
 *  Check that a V1.1 client fails capture cleanly against a server that has no video frame capture module,
 *  and that normal playback in the same session is unaffected.
 *
 * Test Setup:
 *  Language: C++
 *  Testing Framework: Google Test
 *  Components: MediaPipeline, IVideoFrameCaptureFactory
 *
 * Test Initialize:
 *  Create a server that handles Control and MediaPipeline IPC requests, without a capture module.
 *  Initalise the control state to running for this test application.
 *  Initalise an audio video media session paused and prerolled.
 *
 * Test Steps:
 *  Step 1: Create a GL capture
 *   Expect the factory to exist and return no capture.
 *
 *  Step 2: Attach and detach a capture
 *   Expect both to return false.
 *
 *  Step 3: Play
 *   Expect playback to start as normal.
 *
 * Test Teardown:
 *  Terminate the media session.
 *  Server is terminated.
 *
 * Expected Results:
 *  Capture fails cleanly and playback still works.
 *
 * Code:
 */
TEST_F(VideoFrameCaptureUnsupportedTest, captureFailsAndPlaybackWorks)
{
    // Step 1: Create a GL capture
    std::shared_ptr<IVideoFrameCaptureFactory> factory = IVideoFrameCaptureFactory::createFactory();
    ASSERT_NE(factory, nullptr);
    std::shared_ptr<IGLVideoFrameCapture> capture = factory->createGLVideoFrameCapture(nullptr, nullptr);
    EXPECT_EQ(capture, nullptr);

    // Step 2: Attach and detach a capture
    EXPECT_FALSE(m_mediaPipeline->attachVideoFrameCapture(capture));
    EXPECT_FALSE(m_mediaPipeline->detachVideoFrameCapture());

    // Step 3: Play
    MediaPipelineTestMethods::shouldPlay();
    MediaPipelineTestMethods::play();
    MediaPipelineTestMethods::shouldNotifyPlaybackStatePlaying();
    MediaPipelineTestMethods::sendNotifyPlaybackStatePlaying();
}
} // namespace firebolt::rialto::client::ct
