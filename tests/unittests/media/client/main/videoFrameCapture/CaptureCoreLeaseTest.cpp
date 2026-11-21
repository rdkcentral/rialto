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

#include "CaptureCoreTestBase.h"

#include <fcntl.h>

#include <atomic>
#include <chrono>
#include <thread>

class CaptureCoreLeaseTest : public CaptureCoreTestBase
{
protected:
    // strict: every release must be expected explicitly
    void SetUp() override {}

    CaptureAcquireResult noFrame() { return statusResult(CaptureAcquireStatus::NO_FRAME); }
};

TEST_F(CaptureCoreLeaseTest, ShouldReleaseOnceWhenLastReferenceGoes)
{
    EXPECT_CALL(*m_ipc, acquireCurrentFrame(kCaptureId)).WillOnce(Return(okResult(1, 11))).WillOnce(Return(noFrame()));
    expectImport();

    std::shared_ptr<IGLVideoFrame> frame = m_core->acquireCurrentFrame().frame;
    std::shared_ptr<IGLVideoFrame> secondReference = frame;
    m_core->acquireCurrentFrame(); // the capture drops its current-frame reference

    frame.reset(); // one reference left: no release yet
    EXPECT_CALL(*m_ipc, releaseFrame(kCaptureId, kPoolId, 1, 11)).WillOnce(Return(true));
    secondReference.reset();
}

TEST_F(CaptureCoreLeaseTest, ShouldReleaseFromSeveralThreadsWithoutOverlap)
{
    EXPECT_CALL(*m_ipc, acquireCurrentFrame(kCaptureId))
        .WillOnce(Return(okResult(0, 1)))
        .WillOnce(Return(okResult(1, 2, 102))) // a different object, so it is imported too
        .WillOnce(Return(noFrame()));
    expectImport(2);

    std::shared_ptr<IGLVideoFrame> first = m_core->acquireCurrentFrame().frame;
    std::shared_ptr<IGLVideoFrame> second = m_core->acquireCurrentFrame().frame;
    m_core->acquireCurrentFrame();

    std::atomic<int> inFlight{0};
    std::atomic<int> maxInFlight{0};
    auto slowRelease = [&](std::uint32_t, std::uint32_t, std::uint32_t, std::uint64_t)
    {
        const int kNow = ++inFlight;
        maxInFlight = std::max(maxInFlight.load(), kNow);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        --inFlight;
        return true;
    };
    EXPECT_CALL(*m_ipc, releaseFrame(kCaptureId, kPoolId, 0, 1)).WillOnce(Invoke(slowRelease));
    EXPECT_CALL(*m_ipc, releaseFrame(kCaptureId, kPoolId, 1, 2)).WillOnce(Invoke(slowRelease));

    std::thread firstDropper{[&first]() { first.reset(); }};
    std::thread secondDropper{[&second]() { second.reset(); }};
    firstDropper.join();
    secondDropper.join();
    EXPECT_EQ(maxInFlight.load(), 1);
}

TEST_F(CaptureCoreLeaseTest, ShouldReleaseFrameThatOutlivesTheCapture)
{
    EXPECT_CALL(*m_ipc, acquireCurrentFrame(kCaptureId)).WillOnce(Return(okResult(2, 7)));
    expectImport();

    std::shared_ptr<IGLVideoFrame> frame = m_core->acquireCurrentFrame().frame;
    m_core.reset(); // the capture is destroyed first

    EXPECT_CALL(*m_ipc, releaseFrame(kCaptureId, kPoolId, 2, 7)).WillOnce(Return(true));
    frame.reset();
}

TEST_F(CaptureCoreLeaseTest, FailedImportClosesFdsReleasesSlotAndKeepsPreviousFrame)
{
    CaptureAcquireResult failing = okResult(1, 12, 102);
    const int kFailingFd = failing.fds[0].fd;
    EXPECT_CALL(*m_ipc, acquireCurrentFrame(kCaptureId)).WillOnce(Return(okResult(0, 11, 101))).WillOnce(Return(failing));
    EXPECT_CALL(*m_importer, importFrame(_, _, _))
        .WillOnce(Invoke(
            [](const CaptureFrameDescription &, const std::vector<CaptureObjectFd> &, bool &fdsConsumed)
            {
                fdsConsumed = false;
                return std::make_shared<NiceMock<ImportedImageMock>>();
            }))
        .WillOnce(Return(nullptr));

    const GLFrameAcquisition kFirst = m_core->acquireCurrentFrame();
    EXPECT_CALL(*m_ipc, releaseFrame(kCaptureId, kPoolId, 1, 12)).WillOnce(Return(true)); // the failed frame's slot
    const GLFrameAcquisition kAfterFailure = m_core->acquireCurrentFrame();
    ::testing::Mock::VerifyAndClearExpectations(m_ipc.get());

    EXPECT_FALSE(kAfterFailure.changed);
    EXPECT_EQ(kAfterFailure.frame, kFirst.frame);
    EXPECT_EQ(fcntl(kFailingFd, F_GETFD), -1); // closed

    EXPECT_CALL(*m_ipc, releaseFrame(kCaptureId, kPoolId, 0, 11)).WillOnce(Return(true));
    m_core.reset();
}
