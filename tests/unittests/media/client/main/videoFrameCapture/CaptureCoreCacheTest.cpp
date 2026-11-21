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

class CaptureCoreCacheTest : public CaptureCoreTestBase
{
protected:
    static bool isOpen(int fd) { return fcntl(fd, F_GETFD) != -1; }
};

TEST_F(CaptureCoreCacheTest, HitReusesImportAndClosesNewDescriptors)
{
    CaptureAcquireResult first = okResult(0, 1, 101);
    CaptureAcquireResult second = okResult(1, 2, 205);
    CaptureAcquireResult again = okResult(0, 3, 101); // slot 0 again, same object and layout
    const int kAgainFd = again.fds[0].fd;
    EXPECT_CALL(*m_ipc, acquireCurrentFrame(kCaptureId))
        .WillOnce(Return(first))
        .WillOnce(Return(second))
        .WillOnce(Return(again));
    expectImport(2); // not for the third frame

    m_core->acquireCurrentFrame();
    m_core->acquireCurrentFrame();
    const GLFrameAcquisition kHit = m_core->acquireCurrentFrame();
    EXPECT_TRUE(kHit.changed);
    EXPECT_FALSE(isOpen(kAgainFd)); // closed straight away: no import consumed it
    EXPECT_EQ(m_core->cacheSize(), 2u);
}

TEST_F(CaptureCoreCacheTest, ColourChangeMissesTheCache)
{
    CaptureAcquireResult first = okResult(0, 1, 101);
    CaptureAcquireResult recoloured = okResult(0, 2, 101);
    recoloured.description.colour.matrix = 1; // BT601 instead of BT709, same allocation
    EXPECT_CALL(*m_ipc, acquireCurrentFrame(kCaptureId)).WillOnce(Return(first)).WillOnce(Return(recoloured));
    expectImport(2);

    std::shared_ptr<IGLVideoFrame> older = m_core->acquireCurrentFrame().frame;
    m_core->acquireCurrentFrame();
    EXPECT_EQ(m_core->cacheSize(), 2u);
}

TEST_F(CaptureCoreCacheTest, NeverKeepsMoreReusableImportsThanSlots)
{
    // the pool has 3 slots; five different objects arrive one after another
    EXPECT_CALL(*m_ipc, acquireCurrentFrame(kCaptureId))
        .WillOnce(Return(okResult(0, 1, 101)))
        .WillOnce(Return(okResult(1, 2, 102)))
        .WillOnce(Return(okResult(2, 3, 103)))
        .WillOnce(Return(okResult(0, 4, 104)))
        .WillOnce(Return(okResult(1, 5, 105)));
    expectImport(5);
    EXPECT_CALL(*m_importer, retire(_)).Times(2); // the two least recently used imports

    for (int i = 0; i < 5; ++i)
    {
        m_core->acquireCurrentFrame();
        EXPECT_LE(m_core->cacheSize(), 3u);
    }
    EXPECT_EQ(m_core->cacheSize(), 3u);
    ::testing::Mock::VerifyAndClearExpectations(m_importer.get()); // teardown retires the rest
}

TEST_F(CaptureCoreCacheTest, KeepsImportsUsedByHeldFramesBeyondTheBound)
{
    EXPECT_CALL(*m_ipc, acquireCurrentFrame(kCaptureId))
        .WillOnce(Return(okResult(0, 1, 101)))
        .WillOnce(Return(okResult(1, 2, 102)))
        .WillOnce(Return(okResult(2, 3, 103)))
        .WillOnce(Return(okResult(0, 4, 104)));
    expectImport(4);
    EXPECT_CALL(*m_importer, retire(_)).Times(0);

    std::vector<std::shared_ptr<IGLVideoFrame>> held;
    for (int i = 0; i < 4; ++i)
        held.push_back(m_core->acquireCurrentFrame().frame);
    EXPECT_EQ(m_core->cacheSize(), 4u); // every import is in use, so none can be evicted

    ::testing::Mock::VerifyAndClearExpectations(m_importer.get());
}
