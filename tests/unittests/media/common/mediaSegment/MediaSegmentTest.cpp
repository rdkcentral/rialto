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

#include "IMediaPipeline.h"
#include "MediaCommon.h"
#include <gtest/gtest.h>

using firebolt::rialto::IMediaPipeline;
using firebolt::rialto::MediaSourceType;
using firebolt::rialto::SubSamplePair;

TEST(MediaSegmentTests, audioCopyConstructorShouldCopyAllMembers)
{
    constexpr int32_t kSourceId{3};
    constexpr int64_t kTimeStamp{1000};
    constexpr int64_t kDuration{250};
    constexpr int32_t kSampleRate{48000};
    constexpr int32_t kNumberOfChannels{2};
    constexpr uint64_t kClippingStart{10};
    constexpr uint64_t kClippingEnd{20};

    IMediaPipeline::MediaSegmentAudio segment{kSourceId, kTimeStamp,     kDuration,     kSampleRate,
                                            kNumberOfChannels, kClippingStart, kClippingEnd};
    IMediaPipeline::MediaSegmentAudio copy{segment};

    EXPECT_EQ(copy.getId(), kSourceId);
    EXPECT_EQ(copy.getType(), MediaSourceType::AUDIO);
    EXPECT_EQ(copy.getTimeStamp(), kTimeStamp);
    EXPECT_EQ(copy.getDuration(), kDuration);
    EXPECT_EQ(copy.getSampleRate(), kSampleRate);
    EXPECT_EQ(copy.getNumberOfChannels(), kNumberOfChannels);
    EXPECT_EQ(copy.getClippingStart(), kClippingStart);
    EXPECT_EQ(copy.getClippingEnd(), kClippingEnd);
}

TEST(MediaSegmentTests, audioCopyAssignmentShouldCopyAllMembers)
{
    constexpr uint64_t kClippingStart{10};
    constexpr uint64_t kClippingEnd{20};

    IMediaPipeline::MediaSegmentAudio segment{1, 1000, 250, 48000, 2, kClippingStart, kClippingEnd};
    IMediaPipeline::MediaSegmentAudio copy{};
    copy = segment;

    EXPECT_EQ(copy.getId(), 1);
    EXPECT_EQ(copy.getSampleRate(), 48000);
    EXPECT_EQ(copy.getNumberOfChannels(), 2);
    EXPECT_EQ(copy.getClippingStart(), kClippingStart);
    EXPECT_EQ(copy.getClippingEnd(), kClippingEnd);
}

TEST(MediaSegmentTests, videoCopyAssignmentShouldCopyAllMembers)
{
    constexpr int32_t kWidth{1920};
    constexpr int32_t kHeight{1080};
    const firebolt::rialto::Fraction kFrameRate{25, 1};

    IMediaPipeline::MediaSegmentVideo segment{2, 1000, 250, kWidth, kHeight, kFrameRate};
    IMediaPipeline::MediaSegmentVideo copy{};
    copy = segment;

    EXPECT_EQ(copy.getId(), 2);
    EXPECT_EQ(copy.getWidth(), kWidth);
    EXPECT_EQ(copy.getHeight(), kHeight);
    EXPECT_EQ(copy.getFrameRate().numerator, kFrameRate.numerator);
    EXPECT_EQ(copy.getFrameRate().denominator, kFrameRate.denominator);
}

TEST(MediaSegmentTests, audioShallowCopyShouldCopyClippingFields)
{
    constexpr uint64_t kClippingStart{30};
    constexpr uint64_t kClippingEnd{40};

    IMediaPipeline::MediaSegmentAudio segment{1, 1000, 250, 48000, 2, kClippingStart, kClippingEnd};
    std::unique_ptr<IMediaPipeline::MediaSegment> copy{segment.copy()};

    const auto *audioCopy = dynamic_cast<const IMediaPipeline::MediaSegmentAudio *>(copy.get());
    ASSERT_NE(audioCopy, nullptr);
    EXPECT_EQ(audioCopy->getClippingStart(), kClippingStart);
    EXPECT_EQ(audioCopy->getClippingEnd(), kClippingEnd);
}

TEST(MediaSegmentTests, subSamplePairShouldBeDefaultInitialized)
{
    SubSamplePair pair{};
    EXPECT_EQ(pair.numClearBytes, 0u);
    EXPECT_EQ(pair.numEncryptedBytes, 0u);
}
