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

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <gtest/gtest.h>
#include <optional>
#include <random>
#include <string>

using namespace firebolt::rialto::common;

namespace
{
const IProfiler::Record *findRecord(const std::vector<IProfiler::Record> &records, const std::string &stage,
                                    const std::optional<std::string> &info = std::nullopt)
{
    const auto it = std::find_if(records.begin(), records.end(), [&](const auto &record)
                                 { return record.stage == stage && (!info.has_value() || record.info == info.value()); });

    return it != records.end() ? &(*it) : nullptr;
}
} // namespace

class ProfilerTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        saveEnv("PROFILER_ENABLED", m_originalProfilerEnabled);
        saveEnv("PROFILER_DUMP_FILE_NAME", m_originalProfilerDumpFileName);

        factory = IProfilerFactory::createFactory();
        ASSERT_TRUE(factory);

        profiler = factory->createProfiler("UnitTestModule");
        ASSERT_TRUE(profiler);
        ASSERT_TRUE(profiler->isEnabled());
    }

    void TearDown() override
    {
        restoreEnv("PROFILER_ENABLED", m_originalProfilerEnabled);
        restoreEnv("PROFILER_DUMP_FILE_NAME", m_originalProfilerDumpFileName);
    }

    static void saveEnv(const char *name, std::optional<std::string> &value)
    {
        const char *envValue = std::getenv(name);
        if (envValue)
            value = envValue;
        else
            value = std::nullopt;
    }

    static void restoreEnv(const char *name, const std::optional<std::string> &value)
    {
        if (value)
            setenv(name, value->c_str(), 1);
        else
            unsetenv(name);
    }

    std::shared_ptr<IProfilerFactory> factory;
    std::unique_ptr<IProfiler> profiler;
    std::optional<std::string> m_originalProfilerEnabled;
    std::optional<std::string> m_originalProfilerDumpFileName;
};

TEST_F(ProfilerTests, RecordAndFindByStage)
{
    const auto id = profiler->record("Stage1");
    ASSERT_TRUE(id.has_value());

    const auto *found = findRecord(profiler->getRecords(), "Stage1");
    ASSERT_NE(found, nullptr);

    EXPECT_EQ(found->id, id.value());
}

TEST_F(ProfilerTests, RecordAndFindByStageAndInfo)
{
    const auto id = profiler->record("Stage1", "InfoA");
    ASSERT_TRUE(id.has_value());

    const auto *found = findRecord(profiler->getRecords(), "Stage1", "InfoA");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->id, id.value());

    EXPECT_EQ(findRecord(profiler->getRecords(), "Stage1", "InfoB"), nullptr);
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
    setenv("PROFILER_DUMP_FILE_NAME", path.c_str(), 1);

    auto dumpProfiler = factory->createProfiler("UnitTestModule");
    ASSERT_TRUE(dumpProfiler);
    ASSERT_TRUE(dumpProfiler->record("StageDump", "InfoDump").has_value());
    ASSERT_TRUE(dumpProfiler->dumpToFile());

    std::ifstream in(path);
    ASSERT_TRUE(in.good());

    const std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_FALSE(content.empty());
    EXPECT_NE(content.find("StageDump"), std::string::npos);

    std::remove(path.c_str());
}

TEST_F(ProfilerTests, DumpAppendsToExistingFile)
{
    const auto suffix = std::to_string(std::random_device{}());
    const std::string path = std::string{"/tmp/rialto_profiler_ut_append_"} + suffix + ".txt";
    setenv("PROFILER_DUMP_FILE_NAME", path.c_str(), 1);

    auto dumpProfiler = factory->createProfiler("UnitTestModule");
    ASSERT_TRUE(dumpProfiler);
    ASSERT_TRUE(dumpProfiler->record("Stage1").has_value());
    ASSERT_TRUE(dumpProfiler->dumpToFile());

    ASSERT_TRUE(dumpProfiler->record("Stage2").has_value());
    ASSERT_TRUE(dumpProfiler->dumpToFile());

    std::ifstream in(path);
    ASSERT_TRUE(in.good());

    const std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("Stage1"), std::string::npos);
    EXPECT_NE(content.find("Stage2"), std::string::npos);
    EXPECT_LT(content.find("Stage1"), content.rfind("Stage2"));

    std::remove(path.c_str());
}

TEST_F(ProfilerTests, DumpToFileUsesCachedEnvValue)
{
    const auto suffix = std::to_string(std::random_device{}());
    const std::string path = std::string{"/tmp/rialto_profiler_ut_configured_"} + suffix + ".txt";
    setenv("PROFILER_DUMP_FILE_NAME", path.c_str(), 1);

    auto configuredProfiler = factory->createProfiler("UnitTestModule");
    ASSERT_TRUE(configuredProfiler);
    ASSERT_TRUE(configuredProfiler->record("StageConfigured").has_value());
    ASSERT_TRUE(configuredProfiler->dumpToFile());

    unsetenv("PROFILER_DUMP_FILE_NAME");
    ASSERT_TRUE(configuredProfiler->record("StageConfiguredCached").has_value());
    ASSERT_TRUE(configuredProfiler->dumpToFile());

    std::ifstream in(path);
    ASSERT_TRUE(in.good());

    const std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("StageConfigured"), std::string::npos);
    EXPECT_NE(content.find("StageConfiguredCached"), std::string::npos);

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

TEST_F(ProfilerTests, GetRecordsDoesNotContainMissingStage)
{
    ASSERT_TRUE(profiler->record("Stage1").has_value());

    EXPECT_EQ(findRecord(profiler->getRecords(), "MissingStage"), nullptr);
}

TEST_F(ProfilerTests, DumpToFileReturnsFalseForInvalidPath)
{
    setenv("PROFILER_DUMP_FILE_NAME", "/proc/rialto_profiler_ut_dump.txt", 1);
    auto dumpProfiler = factory->createProfiler("UnitTestModule");
    ASSERT_TRUE(dumpProfiler);
    EXPECT_FALSE(dumpProfiler->dumpToFile());
}

TEST_F(ProfilerTests, DumpToFileSucceedsWhenEnvMissing)
{
    EXPECT_TRUE(profiler->dumpToFile());
}
