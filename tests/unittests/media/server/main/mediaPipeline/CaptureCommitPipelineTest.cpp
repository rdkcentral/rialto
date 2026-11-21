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

#include "CaptureCommitMock.h"
#include "CaptureSessionMock.h"
#include "MediaPipelineTestBase.h"

using firebolt::rialto::server::CaptureCommitMock;
using firebolt::rialto::server::CaptureSessionHandle;

/**
 * allSourcesAttached() with an attached capture runs the capture commit first (R7 design D1).
 */
class RialtoServerMediaPipelineCaptureCommitTest : public MediaPipelineTestBase
{
protected:
    static constexpr uint32_t kCaptureId{7};

    RialtoServerMediaPipelineCaptureCommitTest()
    {
        m_captureCommit = m_captureCommitMock;
        createMediaPipeline();
    }

    ~RialtoServerMediaPipelineCaptureCommitTest() { destroyMediaPipeline(); }

    void attachCapture()
    {
        mainThreadWillEnqueueTaskAndWait();
        ASSERT_TRUE(m_mediaPipeline->attachCapture(kCaptureId));
    }

    bool allSourcesAttached()
    {
        mainThreadWillEnqueueTaskAndWait();
        return m_mediaPipeline->allSourcesAttached();
    }

    std::shared_ptr<StrictMock<CaptureCommitMock>> m_captureCommitMock{std::make_shared<StrictMock<CaptureCommitMock>>()};
};

TEST_F(RialtoServerMediaPipelineCaptureCommitTest, WithoutCaptureNothingChanges)
{
    loadGstPlayer();
    EXPECT_CALL(*m_gstPlayerMock, allSourcesAttached());
    EXPECT_TRUE(allSourcesAttached()); // no commit call: the strict mock would flag it
}

TEST_F(RialtoServerMediaPipelineCaptureCommitTest, SuccessfulCaptureCommitIsFollowedByShippingSetup)
{
    loadGstPlayer();
    attachCapture();
    testing::Sequence commitThenSetup;
    EXPECT_CALL(*m_captureCommitMock, run(_, m_kSessionId, kCaptureId, _, _))
        .InSequence(commitThenSetup)
        .WillOnce(Return(true));
    EXPECT_CALL(*m_gstPlayerMock, allSourcesAttached()).InSequence(commitThenSetup);
    EXPECT_TRUE(allSourcesAttached());
}

TEST_F(RialtoServerMediaPipelineCaptureCommitTest, FailedCaptureCommitReturnsFalseAndRecordsNothing)
{
    loadGstPlayer();
    attachCapture();
    EXPECT_CALL(*m_captureCommitMock, run(_, m_kSessionId, kCaptureId, _, _)).WillOnce(Return(false));
    EXPECT_FALSE(allSourcesAttached()); // no shipping setup: the strict player mock would flag it
}

TEST_F(RialtoServerMediaPipelineCaptureCommitTest, CommitGetsTheAttachedVideoSource)
{
    loadGstPlayer();
    attachSource(MediaSourceType::VIDEO, "video/h264");
    attachCapture();
    EXPECT_CALL(*m_captureCommitMock, run(_, m_kSessionId, kCaptureId, _, _))
        .WillOnce(Invoke(
            [](IGstGenericPlayer &, int, std::uint32_t, const std::unique_ptr<IMediaPipeline::MediaSource> &source,
               CaptureSessionHandle &)
            {
                EXPECT_NE(source, nullptr);
                EXPECT_EQ(source->getMimeType(), "video/h264");
                return false;
            }));
    EXPECT_FALSE(allSourcesAttached());
}

TEST_F(RialtoServerMediaPipelineCaptureCommitTest, RetryWithTheCaptureRunsTheWholeStepAgain)
{
    loadGstPlayer();
    attachCapture();
    EXPECT_CALL(*m_captureCommitMock, run(_, m_kSessionId, kCaptureId, _, _)).WillOnce(Return(false)).WillOnce(Return(true));
    EXPECT_FALSE(allSourcesAttached());

    EXPECT_CALL(*m_gstPlayerMock, allSourcesAttached());
    EXPECT_TRUE(allSourcesAttached());
}

TEST_F(RialtoServerMediaPipelineCaptureCommitTest, RetryAfterDetachCommitsWithoutCapture)
{
    loadGstPlayer();
    attachCapture();
    EXPECT_CALL(*m_captureCommitMock, run(_, _, _, _, _)).WillOnce(Return(false));
    EXPECT_FALSE(allSourcesAttached());

    mainThreadWillEnqueueTaskAndWait();
    uint32_t detached{0};
    ASSERT_TRUE(m_mediaPipeline->detachCapture(detached));

    EXPECT_CALL(*m_gstPlayerMock, allSourcesAttached()); // and no second commit run
    EXPECT_TRUE(allSourcesAttached());
}

TEST_F(RialtoServerMediaPipelineCaptureCommitTest, CommitCreatesTheSessionRuntime)
{
    loadGstPlayer();
    attachCapture();
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_EQ(m_mediaPipeline->captureRuntime(), nullptr);

    EXPECT_CALL(*m_captureCommitMock, run(_, m_kSessionId, kCaptureId, _, _))
        .WillOnce(Invoke(
            [](IGstGenericPlayer &, int, std::uint32_t, const std::unique_ptr<IMediaPipeline::MediaSource> &,
               CaptureSessionHandle &committed)
            {
                committed.session =
                    firebolt::rialto::wrappers::CaptureSessionPtr{new testing::NiceMock<
                                                                      firebolt::rialto::capture::CaptureSessionMock>(),
                                                                  [](firebolt::rialto::capture::ICaptureSession *session)
                                                                  { delete session; }};
                committed.poolId = 6;
                committed.pool.slotLayouts.resize(2);
                return true;
            }));
    EXPECT_CALL(*m_gstPlayerMock, allSourcesAttached());
    EXPECT_TRUE(allSourcesAttached());

    mainThreadWillEnqueueTaskAndWait();
    auto runtime = m_mediaPipeline->captureRuntime();
    ASSERT_NE(runtime, nullptr);
    EXPECT_EQ(runtime->poolId(), 6U);
    EXPECT_EQ(runtime->slotCount(), 2U);
}

class RialtoServerMediaPipelineCaptureRetireTest : public RialtoServerMediaPipelineCaptureCommitTest
{
protected:
    // A committed capture; returns its session runtime.
    std::shared_ptr<CaptureSessionRuntime> commit()
    {
        loadGstPlayer();
        attachCapture();
        EXPECT_CALL(*m_captureCommitMock, run(_, _, _, _, _))
            .WillOnce(Invoke(
                [](IGstGenericPlayer &, int, std::uint32_t, const std::unique_ptr<IMediaPipeline::MediaSource> &,
                   CaptureSessionHandle &committed)
                {
                    committed.session = firebolt::rialto::wrappers::
                        CaptureSessionPtr{new testing::NiceMock<firebolt::rialto::capture::CaptureSessionMock>(),
                                          [](firebolt::rialto::capture::ICaptureSession *session) { delete session; }};
                    committed.poolId = 6;
                    return true;
                }));
        EXPECT_CALL(*m_gstPlayerMock, allSourcesAttached());
        EXPECT_TRUE(allSourcesAttached());
        mainThreadWillEnqueueTaskAndWait();
        auto runtime = m_mediaPipeline->captureRuntime();
        EXPECT_NE(runtime, nullptr);
        if (runtime)
        {
            EXPECT_FALSE(runtime->isRetired());
        }
        return runtime;
    }

    void setState(PlaybackState state)
    {
        mainThreadWillEnqueueTask();
        EXPECT_CALL(*m_mediaPipelineClientMock, notifyPlaybackState(state));
        m_gstPlayerCallback->notifyPlaybackState(state);
    }
};

TEST_F(RialtoServerMediaPipelineCaptureRetireTest, StopRetires)
{
    auto runtime = commit();
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, stop());
    EXPECT_TRUE(m_mediaPipeline->stop());
    EXPECT_TRUE(runtime->isRetired());
}

TEST_F(RialtoServerMediaPipelineCaptureRetireTest, FailureRetires)
{
    auto runtime = commit();
    setState(PlaybackState::FAILURE);
    EXPECT_TRUE(runtime->isRetired());
}

TEST_F(RialtoServerMediaPipelineCaptureRetireTest, OtherStatesDoNotRetire)
{
    auto runtime = commit();
    setState(PlaybackState::PLAYING);
    setState(PlaybackState::END_OF_STREAM);
    EXPECT_FALSE(runtime->isRetired());
}

TEST_F(RialtoServerMediaPipelineCaptureRetireTest, DetachInStoppedRetires)
{
    auto runtime = commit();
    setState(PlaybackState::STOPPED); // reached without stop(), so only the detach retires it
    EXPECT_FALSE(runtime->isRetired());
    mainThreadWillEnqueueTaskAndWait();
    uint32_t detached{0};
    ASSERT_TRUE(m_mediaPipeline->detachCapture(detached));
    EXPECT_TRUE(runtime->isRetired());
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_EQ(m_mediaPipeline->captureRuntime(), nullptr);
}

TEST_F(RialtoServerMediaPipelineCaptureRetireTest, PipelineDestructionRetires)
{
    auto runtime = commit();
    destroyMediaPipeline();
    EXPECT_TRUE(runtime->isRetired());
    createMediaPipeline(); // for the fixture's teardown
}

TEST_F(RialtoServerMediaPipelineCaptureRetireTest, StoppedMarksTheSinkAtNullAndDeletesAnIdleSession)
{
    auto runtime = commit();
    mainThreadWillEnqueueTaskAndWait();
    EXPECT_CALL(*m_gstPlayerMock, stop());
    EXPECT_TRUE(m_mediaPipeline->stop());
    EXPECT_FALSE(runtime->isSessionDeleted()); // retired, but playbin is not at NULL yet
    setState(PlaybackState::STOPPED);
    EXPECT_TRUE(runtime->isSessionDeleted()); // no frame held: deleted
}

TEST_F(RialtoServerMediaPipelineCaptureRetireTest, DestructionMarksTheSinkAtNull)
{
    auto runtime = commit();
    destroyMediaPipeline();
    EXPECT_TRUE(runtime->isSessionDeleted());
    createMediaPipeline(); // for the fixture's teardown
}
