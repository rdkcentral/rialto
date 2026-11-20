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

#include "CapturedFrameUtils.h"

#include <gtest/gtest.h>

using namespace firebolt::rialto::capture;

namespace
{
// Fills a record as an acquisition would, then marks it released and its descriptors closed.
void fillReleasedFlushedFrame(CapturedFrame &frame)
{
    frame.slotIndex = kInvalidCaptureSlotIndex;
    frame.acquisitionToken = kInvalidAcquisitionToken;
    frame.presentationTimeNs = 123456789;
    frame.visibleRegion = Rectangle{0, 0, Size{1920, 1080}};
    frame.dmaBufObjectsFlushed = true;
    frame.capabilities.drmFormat = 0x3231564e; // NV12
    frame.capabilities.minimumSlots = 2;
    frame.capabilities.maximumSlots = 4;
    frame.colorDescription = {CaptureYcbcrMatrix::BT709, CaptureSampleRange::NARROW, CaptureChromaSiting::COSITED,
                              CaptureChromaSiting::MIDPOINT};
    frame.backingSize = Size{1920, 1088};
    frame.dmaBufObjects = {{101, 4096}};
    frame.planes = {{0, 101, 0, 2048, 1920}, {1, 101, 2048, 2048, 1920}};
    frame.exportedDmaBufs = {{101, -1}};
}
} // namespace

TEST(CapturedFrameUtilsTest, DefaultRecordIsEmpty)
{
    CapturedFrame frame;
    EXPECT_TRUE(isEmpty(frame));
}

TEST(CapturedFrameUtilsTest, RecordWithOpenDescriptorIsNotEmpty)
{
    CapturedFrame frame;
    frame.exportedDmaBufs = {{101, -1}, {102, 7}};
    EXPECT_FALSE(isEmpty(frame));
}

TEST(CapturedFrameUtilsTest, NegativeDescriptorOtherThanMinusOneIsNotEmpty)
{
    CapturedFrame frame;
    frame.exportedDmaBufs = {{101, -2}};
    EXPECT_FALSE(isEmpty(frame));
}

TEST(CapturedFrameUtilsTest, RecordWithLiveLockIsNotEmpty)
{
    CapturedFrame withSlot;
    withSlot.slotIndex = 0;
    EXPECT_FALSE(isEmpty(withSlot));

    CapturedFrame withToken;
    withToken.acquisitionToken = 42;
    EXPECT_FALSE(isEmpty(withToken));
}

TEST(CapturedFrameUtilsTest, ShouldResetReleasedFlushedRecord)
{
    CapturedFrame frame;
    fillReleasedFlushedFrame(frame);

    EXPECT_TRUE(resetReleased(frame));

    EXPECT_TRUE(isEmpty(frame));
    EXPECT_FALSE(frame.dmaBufObjectsFlushed);
    EXPECT_EQ(frame.presentationTimeNs, 0);
    EXPECT_EQ(frame.visibleRegion.size.width, 0U);
    EXPECT_EQ(frame.capabilities.drmFormat, 0U);
    EXPECT_EQ(frame.capabilities.size, sizeof(CaptureCapabilities));
    EXPECT_EQ(frame.colorDescription.matrix, CaptureYcbcrMatrix::UNKNOWN);
    EXPECT_EQ(frame.backingSize.width, 0U);
    EXPECT_TRUE(frame.dmaBufObjects.empty());
    EXPECT_TRUE(frame.planes.empty());
    EXPECT_TRUE(frame.exportedDmaBufs.empty());
}

TEST(CapturedFrameUtilsTest, ShouldNotResetRecordThatHoldsALock)
{
    CapturedFrame frame;
    fillReleasedFlushedFrame(frame);
    frame.slotIndex = 1;
    frame.acquisitionToken = 42;

    EXPECT_FALSE(resetReleased(frame));
    EXPECT_TRUE(frame.dmaBufObjectsFlushed); // unchanged
    EXPECT_EQ(frame.planes.size(), 2U);
}

TEST(CapturedFrameUtilsTest, ShouldNotResetRecordThatHoldsADescriptor)
{
    CapturedFrame frame;
    fillReleasedFlushedFrame(frame);
    frame.exportedDmaBufs[0].fd = 9;

    EXPECT_FALSE(resetReleased(frame));
    EXPECT_TRUE(frame.dmaBufObjectsFlushed); // unchanged
    EXPECT_EQ(frame.exportedDmaBufs[0].fd, 9);
}
