#include "IProfiler.h"

#include <gtest/gtest.h>
#include <fstream>
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

    const std::string path = "/tmp/rialto_profiler_ut_dump.txt";
    ASSERT_TRUE(profiler->dump(path));

    std::ifstream in(path);
    ASSERT_TRUE(in.good());

    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_FALSE(content.empty());
    EXPECT_NE(content.find("StageDump"), std::string::npos);
}
