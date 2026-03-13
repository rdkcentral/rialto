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

#include "IProfiler.h"

#include <fstream>
#include <gtest/gtest.h>
#include <random>
#include <string>

using namespace firebolt::rialto::common;

class ProfilerTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        factory = IProfilerFactory::createFactory();
        ASSERT_TRUE(factory);

        profiler = factory->createProfiler("UnitTestModule");
        ASSERT_TRUE(profiler);
    }

    std::shared_ptr<IProfilerFactory> factory;
    std::unique_ptr<IProfiler> profiler;
};

TEST_F(ProfilerTests, RecordAndFindByStage)
{
    if (!profiler->enabled())
        GTEST_SKIP() << "Profiler disabled in current configuration";

    auto id = profiler->record("Stage1");
    ASSERT_TRUE(id.has_value());

    auto found = profiler->find("Stage1");
    ASSERT_TRUE(found.has_value());

    EXPECT_EQ(found.value(), id.value());
}

TEST_F(ProfilerTests, RecordAndFindByStageAndInfo)
{
    if (!profiler->enabled())
        GTEST_SKIP() << "Profiler disabled in current configuration";

    auto id = profiler->record("Stage1", "InfoA");
    ASSERT_TRUE(id.has_value());

    EXPECT_TRUE(profiler->find("Stage1", "InfoA").has_value());
    EXPECT_FALSE(profiler->find("Stage1", "InfoB").has_value());
}

TEST_F(ProfilerTests, DumpCreatesFile)
{
    if (!profiler->enabled())
        GTEST_SKIP() << "Profiler disabled in current configuration";

    (void)profiler->record("StageDump", "InfoDump");

    const auto suffix = std::to_string(std::random_device{}());
    const std::string path = std::string{"/tmp/rialto_profiler_ut_dump_"} + suffix + ".txt";
    ASSERT_TRUE(profiler->dump(path));

    std::ifstream in(path);
    ASSERT_TRUE(in.good());

    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_FALSE(content.empty());
    EXPECT_NE(content.find("StageDump"), std::string::npos);
}
