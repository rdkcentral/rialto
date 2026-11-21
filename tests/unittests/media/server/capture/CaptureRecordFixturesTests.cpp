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

#include "CaptureRecordFixtures.h"

#include <dirent.h>
#include <gtest/gtest.h>
#include <unistd.h>

#include <string>

using namespace firebolt::rialto::capture;
using namespace firebolt::rialto::capture::test;

namespace
{
size_t countFixtureFiles()
{
    size_t count{0};
    DIR *dir = opendir("/proc/self/fd");
    if (!dir)
        return 0;
    while (struct dirent *entry = readdir(dir))
    {
        char link[256]{};
        const std::string kPath = std::string("/proc/self/fd/") + entry->d_name;
        if (readlink(kPath.c_str(), link, sizeof(link) - 1) > 0 &&
            std::string(link).rfind("/memfd:capture-record-fixture", 0) == 0)
            ++count;
    }
    closedir(dir);
    return count;
}
} // namespace

TEST(CaptureRecordFixturesTest, ShouldBuildExamplesWithRealDescriptorsAndCloseThem)
{
    {
        CaptureRecordFixtures fixtures;
        CapturedFrame a, b, c, d;
        fixtures.makeExampleA(a);
        fixtures.makeExampleB(b);
        fixtures.makeExampleC(c);
        fixtures.makeExampleD(d);

        EXPECT_EQ(a.exportedDmaBufs.size(), 1U);
        EXPECT_EQ(b.exportedDmaBufs.size(), 1U);
        EXPECT_EQ(b.planes.size(), 2U);
        EXPECT_EQ(c.exportedDmaBufs.size(), 2U);
        EXPECT_EQ(d.planes.size(), 3U);
        EXPECT_EQ(countFixtureFiles(), 6U);

        // a test may close a descriptor itself first
        close(c.exportedDmaBufs[0].fd);
        c.exportedDmaBufs[0].fd = -1;
    }
    EXPECT_EQ(countFixtureFiles(), 0U);
}

TEST(CaptureRecordFixturesTest, ShouldBuildPoolTemplateWithZeroObjectIds)
{
    CapturePool pool;
    CaptureRecordFixtures::makeNv12Pool(pool, 3);

    ASSERT_EQ(pool.slotLayouts.size(), 3U);
    for (uint32_t slot = 0; slot < 3; ++slot)
    {
        EXPECT_EQ(pool.slotLayouts[slot].slotIndex, slot);
        for (const auto &plane : pool.slotLayouts[slot].planes)
            EXPECT_EQ(plane.dmaBufObjectId, 0U);
    }
}
