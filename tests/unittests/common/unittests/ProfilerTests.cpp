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

#include <cstdlib>
#include <fstream>
#include <gtest/gtest.h>
#include <optional>
#include <random>
#include <string>

using namespace firebolt::rialto::common;

class ProfilerTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        const char *envValue = std::getenv("PROFILER_ENABLED");
        if (envValue)
            m_originalProfilerEnabled = envValue;
        else
            m_originalProfilerEnabled = std::nullopt;

        factory = IProfilerFactory::createFactory();
        ASSERT_TRUE(factory);

        profiler = factory->createProfiler("UnitTestModule");
        ASSERT_TRUE(profiler);
        ASSERT_TRUE(profiler->isEnabled());
    }

    void TearDown() override
    {
        if (m_originalProfilerEnabled)
            setenv("PROFILER_ENABLED", m_originalProfilerEnabled->c_str(), 1);
        else
            unsetenv("PROFILER_ENABLED");
    }

    std::shared_ptr<IProfilerFactory> factory;
    std::unique_ptr<IProfiler> profiler;
    std::optional<std::string> m_originalProfilerEnabled;
};

TEST_F(ProfilerTests, RecordAndFindByStage)
{
    const auto id = profiler->record("Stage1");
    ASSERT_TRUE(id.has_value());

    const auto found = profiler->find("Stage1");
    ASSERT_TRUE(found.has_value());

    EXPECT_EQ(found.value(), id.value());
}

TEST_F(ProfilerTests, RecordAndFindByStageAndInfo)
{
    const auto id = profiler->record("Stage1", "InfoA");
    ASSERT_TRUE(id.has_value());

    const auto found = profiler->find("Stage1", "InfoA");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found.value(), id.value());

    EXPECT_FALSE(profiler->find("Stage1", "InfoB").has_value());
}

TEST_F(ProfilerTests, GetRecordsReturnsRecordedEntries)
{
    const auto id1 = profiler->record("Stage1");
    ASSERT_TRUE(id1.has_value());

    const auto id2 = profiler->record("Stage2", "Info2");
    ASSERT_TRUE(id2.has_value());

    const auto &records = profiler->getRecords();
    ASSERT_GE(records.size(), 2U);

    const auto &record1 = records[records.size() - 2];
    EXPECT_EQ(record1.module, "UnitTestModule");
    EXPECT_EQ(record1.id, id1.value());
    EXPECT_EQ(record1.stage, "Stage1");
    EXPECT_TRUE(record1.info.empty());

    const auto &record2 = records[records.size() - 1];
    EXPECT_EQ(record2.module, "UnitTestModule");
    EXPECT_EQ(record2.id, id2.value());
    EXPECT_EQ(record2.stage, "Stage2");
    EXPECT_EQ(record2.info, "Info2");
}

TEST_F(ProfilerTests, DumpCreatesFile)
{
    (void)profiler->record("StageDump", "InfoDump");

    const auto suffix = std::to_string(std::random_device{}());
    const std::string path = std::string{"/tmp/rialto_profiler_ut_dump_"} + suffix + ".txt";

    ASSERT_TRUE(profiler->dump(path));

    std::ifstream in(path);
    ASSERT_TRUE(in.good());

    const std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_FALSE(content.empty());
    EXPECT_NE(content.find("StageDump"), std::string::npos);

    std::remove(path.c_str());
}

TEST_F(ProfilerTests, StartsEnabledWhenEnvTrue)
{
    setenv("PROFILER_ENABLED", "true", 1);

    auto envProfiler = factory->createProfiler("UnitTestModule");

    ASSERT_TRUE(envProfiler);
    EXPECT_TRUE(envProfiler->isEnabled());
}

TEST_F(ProfilerTests, StartsDisabledWhenEnvValueInvalid)
{
    setenv("PROFILER_ENABLED", "definitely-not-a-bool", 1);

    auto envProfiler = factory->createProfiler("UnitTestModule");

    ASSERT_TRUE(envProfiler);
    EXPECT_FALSE(envProfiler->isEnabled());
}

TEST_F(ProfilerTests, StartsDisabledWhenEnvFalse)
{
    setenv("PROFILER_ENABLED", "false", 1);

    auto envProfiler = factory->createProfiler("UnitTestModule");

    ASSERT_TRUE(envProfiler);
    EXPECT_FALSE(envProfiler->isEnabled());
}

TEST_F(ProfilerTests, FindByStageReturnsNulloptWhenMissing)
{
    ASSERT_TRUE(profiler->record("Stage1").has_value());

    EXPECT_EQ(profiler->find("MissingStage"), std::nullopt);
}

TEST_F(ProfilerTests, DumpReturnsFalseForInvalidPath)
{
    EXPECT_FALSE(profiler->dump("/proc/rialto_profiler_ut_dump.txt"));
}
