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

#include "DataReaderFactory.h"
#include "DataReaderV1.h"
#include "DataReaderV2.h"
#include <gtest/gtest.h>

class DataReaderFactoryTests : public testing::Test
{
protected:
    firebolt::rialto::server::DataReaderFactory m_sut;
};

TEST_F(DataReaderFactoryTests, shouldFailToCreateDataReaderForUnknownVersion)
{
    constexpr auto mediaSourceType = firebolt::rialto::MediaSourceType::VIDEO;
    constexpr std::uint32_t numFrames{1};
    std::uint32_t version{23};
    std::uint8_t *data{reinterpret_cast<std::uint8_t *>(&version)};
    auto reader = m_sut.createDataReader(mediaSourceType, data, 0, numFrames);
    ASSERT_EQ(nullptr, reader);
}

TEST_F(DataReaderFactoryTests, shouldCreateDataReaderV1)
{
    constexpr auto mediaSourceType = firebolt::rialto::MediaSourceType::VIDEO;
    constexpr std::uint32_t numFrames{1};
    std::uint32_t version{1};
    std::uint8_t *data{reinterpret_cast<std::uint8_t *>(&version)};
    auto reader = m_sut.createDataReader(mediaSourceType, data, 0, numFrames);
    ASSERT_NE(nullptr, reader);
    firebolt::rialto::server::DataReaderV1 *v1Reader =
        dynamic_cast<firebolt::rialto::server::DataReaderV1 *>(reader.get());
    ASSERT_NE(nullptr, v1Reader);
}

TEST_F(DataReaderFactoryTests, shouldCreateDataReaderV2)
{
    constexpr auto mediaSourceType = firebolt::rialto::MediaSourceType::VIDEO;
    constexpr std::uint32_t numFrames{1};
    std::uint32_t version{2};
    std::uint8_t *data{reinterpret_cast<std::uint8_t *>(&version)};
    auto reader = m_sut.createDataReader(mediaSourceType, data, 0, numFrames);
    ASSERT_NE(nullptr, reader);
    firebolt::rialto::server::DataReaderV2 *v2Reader =
        dynamic_cast<firebolt::rialto::server::DataReaderV2 *>(reader.get());
    ASSERT_NE(nullptr, v2Reader);
}
