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

#include "GlCaptureFixture.h"

namespace firebolt::rialto::client::ct
{
/**
 * The client frame lease (R11) end to end: the server stub answers with real descriptors through the IPC, and
 * frames are imported through the fake libEGL.
 */
class VideoFrameLeaseTest : public GlCaptureFixture
{
public:
    void serverWillReturnNoFrame()
    {
        EXPECT_CALL(*m_videoFrameCaptureModuleMock, acquireCurrentFrame(_, _, _, _))
            .WillOnce(Invoke(
                [](::google::protobuf::RpcController *, const AcquireCurrentVideoFrameRequest *,
                   AcquireCurrentVideoFrameResponse *response, ::google::protobuf::Closure *done)
                {
                    response->set_status(VIDEO_FRAME_CAPTURE_STATUS_NO_FRAME);
                    done->Run();
                }))
            .RetiresOnSaturation();
    }
};

/*
 * Component Test: Frame lease
 * Test Objective:
 *  Check acquire, repeat, release, invalidation with reused object IDs, and a release after the pipeline stopped.
 *
 * Test Steps:
 *  Step 1: Acquire a frame: changed, one texture.
 *  Step 2: Acquire again with no new frame: the same frame object, no new import.
 *  Step 3: No frame: the current frame is cleared; dropping the application's reference releases the slot once.
 *  Step 4: A frame reusing the object ID without invalidation reuses the import; one with invalidation gets a new
 *          import even though the ID is the same.
 *  Step 5: Hold a frame, attach the capture to a pipeline and stop it, then drop the frame: its slot is still
 *          released.
 *
 * Code:
 */
TEST_F(VideoFrameLeaseTest, leaseAcrossRepeatReleaseInvalidationAndStop)
{
    auto capture = createCapture();
    ASSERT_NE(capture, nullptr);
    makeApplicationContextCurrent();

    // Step 1: Acquire
    serverWillReturnFrame(0, 1, false);
    GLFrameAcquisition first = capture->acquireCurrentFrame();
    ASSERT_TRUE(first.changed);
    ASSERT_NE(first.frame, nullptr);
    const std::uint32_t kFirstTexture{first.frame->textureIds().at(0)};
    EXPECT_EQ(m_liveTextures(), 1);

    // Step 2: Repeat
    serverWillReturnNoNewFrame();
    GLFrameAcquisition repeat = capture->acquireCurrentFrame();
    EXPECT_FALSE(repeat.changed);
    EXPECT_EQ(repeat.frame, first.frame);
    EXPECT_EQ(m_liveTextures(), 1);

    // Step 3: Release
    serverWillReturnNoFrame();
    EXPECT_EQ(capture->acquireCurrentFrame().frame, nullptr);
    serverWillRelease(0, 1);
    repeat.frame.reset();
    first.frame.reset(); // the last reference: one release

    // Step 4: Reused IDs, then invalidation with the same ID
    serverWillReturnFrame(1, 2, false, 101);
    GLFrameAcquisition reused = capture->acquireCurrentFrame();
    ASSERT_NE(reused.frame, nullptr);
    EXPECT_EQ(reused.frame->textureIds().at(0), kFirstTexture); // the cached import
    serverWillReturnFrame(0, 3, true, 101);
    GLFrameAcquisition flushed = capture->acquireCurrentFrame();
    ASSERT_NE(flushed.frame, nullptr);
    EXPECT_NE(flushed.frame->textureIds().at(0), kFirstTexture); // imported again
    serverWillRelease(1, 2);
    reused.frame.reset();

    // Step 5: Release after stop
    MediaPipelineTestMethods::shouldCreateMediaSession();
    MediaPipelineTestMethods::createMediaPipeline();
    EXPECT_CALL(*m_mediaPipelineModuleMock, attachVideoFrameCapture(_, _, _, _))
        .WillOnce(Invoke([](::google::protobuf::RpcController *, const AttachVideoFrameCaptureRequest *,
                            AttachVideoFrameCaptureResponse *, ::google::protobuf::Closure *done) { done->Run(); }));
    EXPECT_TRUE(m_mediaPipeline->attachVideoFrameCapture(capture));
    MediaPipelineTestMethods::shouldStop();
    MediaPipelineTestMethods::stop();
    serverWillReturnNoFrame();
    EXPECT_EQ(capture->acquireCurrentFrame().frame, nullptr); // the server answers NO_FRAME after stop
    serverWillRelease(0, 3);
    flushed.frame.reset(); // a late release, after stop, still reaches the server

    // the pipeline holds the capture; destroying both tears everything down
    serverWillDestroyCapture();
    capture.reset();
    MediaPipelineTestMethods::shouldDestroyMediaSession();
    MediaPipelineTestMethods::destroyMediaPipeline();
    EXPECT_EQ(m_liveTextures(), 0);
    EXPECT_EQ(m_liveImages(), 0);
    EXPECT_EQ(openTestMemfds(), 0);
    m_makeCurrent(m_display, nullptr, nullptr, nullptr);
}
} // namespace firebolt::rialto::client::ct
