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
class GlVideoFrameCaptureTest : public GlCaptureFixture
{
};

/*
 * Component Test: GL capture lifecycle without leaks
 * Test Objective:
 *  Check acquisition, invalidation, release on another thread and teardown from a thread without a context, and
 *  that no descriptor or GL object is left afterwards.
 *
 * Test Steps:
 *  Step 1: Acquire a frame with the application's context current.
 *   Expect one external texture.
 *  Step 2: Acquire a flushed frame while the first is held.
 *   Expect a second texture; the first frame's import is retired but still in use.
 *  Step 3: Drop the first frame on another thread without a context.
 *   Expect its slot to be released and no EGL or GL call on that thread.
 *  Step 4: Acquire again (no new frame).
 *   Expect the retired import to be deleted.
 *  Step 5: Drop the last frame and the capture on another thread without a context.
 *   Expect the release and the capture's destruction, and every GL object deleted on a temporary context.
 *
 * Expected Results:
 *  No live texture, image or context remains, and every descriptor of the test's frames is closed.
 *
 * Code:
 */
TEST_F(GlVideoFrameCaptureTest, lifecycleLeavesNoDescriptorsOrGlObjects)
{
    ASSERT_EQ(openTestMemfds(), 0);
    auto capture = createCapture();
    ASSERT_NE(capture, nullptr);
    ASSERT_TRUE(m_liveTextures && m_liveImages && m_liveContexts && m_threadCalls && m_resetThreadCalls && m_makeCurrent);
    makeApplicationContextCurrent();

    // Step 1: Acquire
    serverWillReturnFrame(0, 1, false);
    GLFrameAcquisition first = capture->acquireCurrentFrame();
    ASSERT_TRUE(first.changed);
    ASSERT_NE(first.frame, nullptr);
    EXPECT_EQ(first.frame->textureIds().size(), 1U);
    EXPECT_EQ(m_liveTextures(), 1);
    EXPECT_EQ(openTestMemfds(), 0); // EGL only borrowed the descriptor

    // Step 2: Invalidate while the first frame is held
    serverWillReturnFrame(1, 2, true);
    GLFrameAcquisition second = capture->acquireCurrentFrame();
    ASSERT_TRUE(second.changed);
    EXPECT_EQ(m_liveTextures(), 2);

    // Step 3: Release on another thread
    serverWillRelease(0, 1);
    int callsOnReleasingThread{-1};
    std::thread releaser{[&]()
                         {
                             m_resetThreadCalls();
                             first.frame.reset();
                             callsOnReleasingThread = m_threadCalls();
                         }};
    releaser.join();
    EXPECT_EQ(callsOnReleasingThread, 0);
    EXPECT_EQ(m_liveTextures(), 2);

    // Step 4: Acquire again
    serverWillReturnNoNewFrame();
    EXPECT_FALSE(capture->acquireCurrentFrame().changed);
    EXPECT_EQ(m_liveTextures(), 1);

    // Step 5: Destroy from a thread without a context
    serverWillRelease(1, 2);
    serverWillDestroyCapture();
    std::thread destroyer{[&]()
                          {
                              second.frame.reset();
                              capture.reset();
                          }};
    destroyer.join();

    EXPECT_EQ(m_liveTextures(), 0);
    EXPECT_EQ(m_liveImages(), 0);
    EXPECT_EQ(m_liveContexts(), 0); // the temporary context is gone too
    EXPECT_EQ(openTestMemfds(), 0);
    m_makeCurrent(m_display, nullptr, nullptr, nullptr);
}
} // namespace firebolt::rialto::client::ct
