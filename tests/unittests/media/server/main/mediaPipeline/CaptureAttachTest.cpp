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

#include "MediaPipelineTestBase.h"

/**
 * Attach and detach rules for every pipeline state (proposal 3.2 and 3.5).
 */
class RialtoServerMediaPipelineCaptureAttachTest : public MediaPipelineTestBase
{
protected:
    static constexpr uint32_t kCaptureId{7};

    RialtoServerMediaPipelineCaptureAttachTest() { createMediaPipeline(); }
    ~RialtoServerMediaPipelineCaptureAttachTest() { destroyMediaPipeline(); }

    bool attach(uint32_t captureId = kCaptureId)
    {
        mainThreadWillEnqueueTaskAndWait();
        return m_mediaPipeline->attachCapture(captureId);
    }

    bool detach(uint32_t &captureId)
    {
        mainThreadWillEnqueueTaskAndWait();
        return m_mediaPipeline->detachCapture(captureId);
    }

    bool detach()
    {
        uint32_t captureId{0};
        return detach(captureId);
    }

    void commit()
    {
        mainThreadWillEnqueueTaskAndWait();
        EXPECT_CALL(*m_gstPlayerMock, allSourcesAttached());
        EXPECT_TRUE(m_mediaPipeline->allSourcesAttached());
    }

    void setState(PlaybackState state)
    {
        mainThreadWillEnqueueTask();
        EXPECT_CALL(*m_mediaPipelineClientMock, notifyPlaybackState(state));
        m_gstPlayerCallback->notifyPlaybackState(state);
    }
};

TEST_F(RialtoServerMediaPipelineCaptureAttachTest, AttachBeforeLoadFails)
{
    EXPECT_FALSE(attach());
}

TEST_F(RialtoServerMediaPipelineCaptureAttachTest, AttachAndDetachAfterLoad)
{
    loadGstPlayer();
    EXPECT_TRUE(attach());
    uint32_t detached{0};
    EXPECT_TRUE(detach(detached));
    EXPECT_EQ(detached, kCaptureId);
    EXPECT_FALSE(detach()); // nothing attached any more
}

TEST_F(RialtoServerMediaPipelineCaptureAttachTest, SecondCaptureOnSamePipelineFails)
{
    loadGstPlayer();
    EXPECT_TRUE(attach(kCaptureId));
    EXPECT_FALSE(attach(kCaptureId + 1));
    uint32_t detached{0};
    EXPECT_TRUE(detach(detached));
    EXPECT_EQ(detached, kCaptureId); // the first stayed attached
}

TEST_F(RialtoServerMediaPipelineCaptureAttachTest, AttachAfterCommitFails)
{
    loadGstPlayer();
    commit();
    EXPECT_FALSE(attach());
}

TEST_F(RialtoServerMediaPipelineCaptureAttachTest, DetachWithNothingAttachedFails)
{
    loadGstPlayer();
    EXPECT_FALSE(detach());
}

TEST_F(RialtoServerMediaPipelineCaptureAttachTest, CaptureStaysAttachedThroughCommit)
{
    loadGstPlayer();
    EXPECT_TRUE(attach());
    commit();
    EXPECT_FALSE(detach()); // committed and not stopped
    setState(PlaybackState::STOPPED);
    EXPECT_TRUE(detach());
}

class RialtoServerMediaPipelineCaptureStateTest : public RialtoServerMediaPipelineCaptureAttachTest,
                                                  public ::testing::WithParamInterface<PlaybackState>
{
};

TEST_P(RialtoServerMediaPipelineCaptureStateTest, AttachAndDetachFollowTheStateTable)
{
    const PlaybackState kState{GetParam()};
    loadGstPlayer();
    EXPECT_TRUE(attach());
    commit();
    setState(kState);

    EXPECT_FALSE(attach(kCaptureId + 1)); // never after commit
    EXPECT_EQ(detach(), kState == PlaybackState::STOPPED);
}

INSTANTIATE_TEST_SUITE_P(AllShippingStates, RialtoServerMediaPipelineCaptureStateTest,
                         ::testing::Values(PlaybackState::IDLE, PlaybackState::PAUSED, PlaybackState::PLAYING,
                                           PlaybackState::SEEKING, PlaybackState::SEEK_DONE,
                                           PlaybackState::END_OF_STREAM, PlaybackState::STOPPED, PlaybackState::FAILURE));

TEST_F(RialtoServerMediaPipelineCaptureAttachTest, FailureBeforeCommitBlocksAttachAndDetach)
{
    loadGstPlayer();
    EXPECT_TRUE(attach());
    setState(PlaybackState::FAILURE);
    EXPECT_FALSE(attach(kCaptureId + 1));
    EXPECT_FALSE(detach());
}

TEST_F(RialtoServerMediaPipelineCaptureAttachTest, DropRemovesOnlyTheNamedCaptureInAnyState)
{
    loadGstPlayer();
    EXPECT_TRUE(attach());
    commit();
    setState(PlaybackState::PLAYING);

    mainThreadWillEnqueueTaskAndWait();
    m_mediaPipeline->dropCapture(kCaptureId + 1); // not the attached one: no effect
    setState(PlaybackState::STOPPED);
    uint32_t detached{0};
    EXPECT_TRUE(detach(detached));
    EXPECT_EQ(detached, kCaptureId);
}

TEST_F(RialtoServerMediaPipelineCaptureAttachTest, DropDuringPlaybackDetaches)
{
    loadGstPlayer();
    EXPECT_TRUE(attach());
    commit();
    setState(PlaybackState::PLAYING);

    mainThreadWillEnqueueTaskAndWait();
    m_mediaPipeline->dropCapture(kCaptureId);
    setState(PlaybackState::STOPPED);
    EXPECT_FALSE(detach()); // nothing left to detach
}
