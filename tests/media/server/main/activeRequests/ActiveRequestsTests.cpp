/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Sky UK
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

#include "ActiveRequests.h"
#include <gtest/gtest.h>

struct ActiveRequestsTests : public testing::Test
{
    firebolt::rialto::server::ActiveRequests m_sut;
};

TEST_F(ActiveRequestsTests, getTypeShouldReturnUnknownForInvalidId)
{
    EXPECT_EQ(firebolt::rialto::MediaSourceType::UNKNOWN, m_sut.getType(123));
}

TEST_F(ActiveRequestsTests, getSegmentsShouldThrowForInvalidId)
{
    EXPECT_THROW(m_sut.getSegments(123), std::runtime_error);
}

TEST_F(ActiveRequestsTests, addSegmentShouldReturnFalseForNullSegment)
{
    EXPECT_EQ(m_sut.addSegment(123, nullptr), firebolt::rialto::AddSegmentStatus::ERROR);
}

TEST_F(ActiveRequestsTests, addSegmentShouldReturnErrorForInvalidData)
{
    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSegment> segment =
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSegment>();
    EXPECT_EQ(0, m_sut.insert(firebolt::rialto::MediaSourceType::AUDIO, std::numeric_limits<std::uint32_t>::max()));
    EXPECT_EQ(m_sut.addSegment(0, segment), firebolt::rialto::AddSegmentStatus::ERROR);
}

TEST_F(ActiveRequestsTests, addSegmentShouldReturnErrorForInvalidId)
{
    std::vector<uint8_t> data{'T', 'E', 'S', 'T'};
    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSegment> segment =
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSegment>();
    segment->setData(data.size(), data.data());
    EXPECT_EQ(m_sut.addSegment(123, segment), firebolt::rialto::AddSegmentStatus::ERROR);
}

TEST_F(ActiveRequestsTests, addSegmentsOverLimitShouldReturnNoSpace)
{
    EXPECT_EQ(0, m_sut.insert(firebolt::rialto::MediaSourceType::AUDIO, 5));
    std::vector<uint8_t> data{'T', 'E', 'S', 'T'};
    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSegment> segment =
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSegment>();
    segment->setData(data.size(), data.data());
    EXPECT_EQ(m_sut.addSegment(0, segment), firebolt::rialto::AddSegmentStatus::OK);

    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSegment> segmentOverlimit =
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSegment>();
    segmentOverlimit->setData(data.size(), data.data());
    EXPECT_EQ(m_sut.addSegment(0, segmentOverlimit), firebolt::rialto::AddSegmentStatus::NO_SPACE);
}

TEST_F(ActiveRequestsTests, shouldGenerateGetAndEraseIds)
{
    EXPECT_EQ(firebolt::rialto::MediaSourceType::UNKNOWN, m_sut.getType(0));
    EXPECT_EQ(0, m_sut.insert(firebolt::rialto::MediaSourceType::AUDIO, std::numeric_limits<std::uint32_t>::max()));
    EXPECT_EQ(firebolt::rialto::MediaSourceType::AUDIO, m_sut.getType(0));
    m_sut.erase(0);
    EXPECT_EQ(firebolt::rialto::MediaSourceType::UNKNOWN, m_sut.getType(0));

    EXPECT_EQ(firebolt::rialto::MediaSourceType::UNKNOWN, m_sut.getType(1));
    EXPECT_EQ(1, m_sut.insert(firebolt::rialto::MediaSourceType::VIDEO, std::numeric_limits<std::uint32_t>::max()));
    EXPECT_EQ(firebolt::rialto::MediaSourceType::VIDEO, m_sut.getType(1));
    m_sut.erase(1);
    EXPECT_EQ(firebolt::rialto::MediaSourceType::UNKNOWN, m_sut.getType(1));
}

TEST_F(ActiveRequestsTests, shouldClearIds)
{
    EXPECT_EQ(firebolt::rialto::MediaSourceType::UNKNOWN, m_sut.getType(0));
    EXPECT_EQ(0, m_sut.insert(firebolt::rialto::MediaSourceType::AUDIO, std::numeric_limits<std::uint32_t>::max()));
    EXPECT_EQ(1, m_sut.insert(firebolt::rialto::MediaSourceType::VIDEO, std::numeric_limits<std::uint32_t>::max()));
    EXPECT_EQ(firebolt::rialto::MediaSourceType::AUDIO, m_sut.getType(0));
    EXPECT_EQ(firebolt::rialto::MediaSourceType::VIDEO, m_sut.getType(1));
    m_sut.clear();
    EXPECT_EQ(firebolt::rialto::MediaSourceType::UNKNOWN, m_sut.getType(0));
    EXPECT_EQ(firebolt::rialto::MediaSourceType::UNKNOWN, m_sut.getType(1));
}

TEST_F(ActiveRequestsTests, shouldAddAndGetSegments)
{
    std::vector<uint8_t> data{'T', 'E', 'S', 'T'};
    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSegment> segment =
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSegmentAudio>();
    segment->setData(data.size(), data.data());
    EXPECT_EQ(0, m_sut.insert(firebolt::rialto::MediaSourceType::AUDIO, std::numeric_limits<std::uint32_t>::max()));
    EXPECT_EQ(m_sut.addSegment(0, segment), firebolt::rialto::AddSegmentStatus::OK);
    const firebolt::rialto::IMediaPipeline::MediaSegmentVector &segments = m_sut.getSegments(0);
    ASSERT_EQ(1, segments.size());
    EXPECT_EQ(segments[0]->getType(), firebolt::rialto::MediaSourceType::AUDIO);
}

TEST_F(ActiveRequestsTests, shouldAddAndRemoveSegments)
{
    std::vector<uint8_t> data{'T', 'E', 'S', 'T'};
    std::unique_ptr<firebolt::rialto::IMediaPipeline::MediaSegment> segment =
        std::make_unique<firebolt::rialto::IMediaPipeline::MediaSegment>();
    segment->setData(data.size(), data.data());
    EXPECT_EQ(0, m_sut.insert(firebolt::rialto::MediaSourceType::AUDIO, std::numeric_limits<std::uint32_t>::max()));
    EXPECT_EQ(m_sut.addSegment(0, segment), firebolt::rialto::AddSegmentStatus::OK);
    m_sut.clear();
    EXPECT_THROW(m_sut.getSegments(0), std::runtime_error);
}
