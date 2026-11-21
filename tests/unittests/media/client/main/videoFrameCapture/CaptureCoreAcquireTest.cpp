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

class CaptureCoreAcquireTest : public CaptureCoreTestBase
{
};

TEST_F(CaptureCoreAcquireTest, NewFrameIsChanged)
{
    EXPECT_CALL(*m_ipc, acquireCurrentFrame(kCaptureId)).WillOnce(Return(okResult(0, 1)));
    expectImport();

    const GLFrameAcquisition kAcquisition = m_core->acquireCurrentFrame();
    EXPECT_TRUE(kAcquisition.changed);
    EXPECT_NE(kAcquisition.frame, nullptr);
}

TEST_F(CaptureCoreAcquireTest, RepeatReturnsSameFrameWithoutNewImport)
{
    EXPECT_CALL(*m_ipc, acquireCurrentFrame(kCaptureId))
        .WillOnce(Return(okResult(0, 1)))
        .WillOnce(Return(statusResult(CaptureAcquireStatus::NO_NEW_FRAME)));
    expectImport(1);

    const GLFrameAcquisition kFirst = m_core->acquireCurrentFrame();
    const GLFrameAcquisition kSecond = m_core->acquireCurrentFrame();
    EXPECT_FALSE(kSecond.changed);
    EXPECT_EQ(kSecond.frame, kFirst.frame);
}

TEST_F(CaptureCoreAcquireTest, NoFrameReturnsNull)
{
    EXPECT_CALL(*m_ipc, acquireCurrentFrame(kCaptureId)).WillOnce(Return(statusResult(CaptureAcquireStatus::NO_FRAME)));

    const GLFrameAcquisition kAcquisition = m_core->acquireCurrentFrame();
    EXPECT_FALSE(kAcquisition.changed);
    EXPECT_EQ(kAcquisition.frame, nullptr);
}

TEST_F(CaptureCoreAcquireTest, TransientFailuresKeepCurrentFrame)
{
    EXPECT_CALL(*m_ipc, acquireCurrentFrame(kCaptureId))
        .WillOnce(Return(okResult(0, 1)))
        .WillOnce(Return(statusResult(CaptureAcquireStatus::NO_RESOURCES)))
        .WillOnce(Return(statusResult(CaptureAcquireStatus::IPC_FAILURE)));
    expectImport();

    const GLFrameAcquisition kFirst = m_core->acquireCurrentFrame();
    EXPECT_EQ(m_core->acquireCurrentFrame().frame, kFirst.frame);
    const GLFrameAcquisition kAfterIpcFailure = m_core->acquireCurrentFrame();
    EXPECT_FALSE(kAfterIpcFailure.changed);
    EXPECT_EQ(kAfterIpcFailure.frame, kFirst.frame);
}

TEST_F(CaptureCoreAcquireTest, FatalErrorClearsCurrentFrame)
{
    EXPECT_CALL(*m_ipc, acquireCurrentFrame(kCaptureId))
        .WillOnce(Return(okResult(0, 1)))
        .WillOnce(Return(statusResult(CaptureAcquireStatus::FATAL_ERROR)))
        .WillOnce(Return(statusResult(CaptureAcquireStatus::NO_NEW_FRAME)));
    expectImport();

    m_core->acquireCurrentFrame();
    const GLFrameAcquisition kAfterFatal = m_core->acquireCurrentFrame();
    EXPECT_FALSE(kAfterFatal.changed);
    EXPECT_EQ(kAfterFatal.frame, nullptr);
    EXPECT_EQ(m_core->acquireCurrentFrame().frame, nullptr); // stays cleared
}
