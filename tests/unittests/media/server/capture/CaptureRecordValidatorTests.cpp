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
#include "CaptureRecordValidator.h"

#include <gtest/gtest.h>

#include <functional>
#include <string>
#include <vector>

using namespace firebolt::rialto::capture;
using namespace firebolt::rialto::capture::test;

namespace
{
struct CapabilitiesCase
{
    std::string name;
    std::function<void(CaptureCapabilities &)> change;
    std::string failedRule; // empty: expected to pass
};

struct PoolCase
{
    std::string name;
    uint32_t requested;
    std::function<void(CapturePool &)> change;
    std::string failedRule;
};

void expectResult(const ValidationResult &result, const std::string &failedRule, const std::string &caseName)
{
    if (failedRule.empty())
    {
        EXPECT_TRUE(result.ok) << caseName << ": " << result.rule;
    }
    else
    {
        EXPECT_FALSE(result.ok) << caseName;
        EXPECT_EQ(result.rule.rfind(failedRule, 0), 0U) << caseName << ": got " << result.rule;
    }
}
} // namespace

TEST(CaptureRecordValidatorTest, ShouldCheckCapabilities)
{
    const std::vector<CapabilitiesCase> kCases{
        {"valid limits", [](CaptureCapabilities &c) { c.minimumSlots = 2, c.maximumSlots = 4; }, ""},
        {"equal limits", [](CaptureCapabilities &c) { c.minimumSlots = c.maximumSlots = 3; }, ""},
        {"minimum zero", [](CaptureCapabilities &c) { c.minimumSlots = 0; }, "capabilities.minimumSlots"},
        {"minimum above maximum", [](CaptureCapabilities &c) { c.minimumSlots = 5, c.maximumSlots = 4; },
         "capabilities.slotRange"},
        {"zero width", [](CaptureCapabilities &c) { c.maximumContentSize.width = 0; }, "capabilities.maximumContentSize"},
        {"zero height", [](CaptureCapabilities &c) { c.maximumContentSize.height = 0; },
         "capabilities.maximumContentSize"},
        {"short record", [](CaptureCapabilities &c) { c.size = sizeof(CaptureCapabilities) - 4; }, "capabilities.size"},
    };

    for (const auto &kCase : kCases)
    {
        CaptureCapabilities capabilities = CaptureRecordFixtures::nv12Capabilities();
        kCase.change(capabilities);
        expectResult(validateCapabilities(capabilities), kCase.failedRule, kCase.name);
    }
}

TEST(CaptureRecordValidatorTest, ShouldAcceptLargerRecordFromNewerHal)
{
    CaptureCapabilities capabilities = CaptureRecordFixtures::nv12Capabilities();
    capabilities.size = sizeof(CaptureCapabilities) + 8;
    EXPECT_TRUE(validateCapabilities(capabilities).ok);
}

TEST(CaptureRecordValidatorTest, ShouldCheckPools)
{
    const std::vector<PoolCase> kCases{
        {"valid pool", 3, [](CapturePool &) {}, ""},
        {"wrong slot count", 3, [](CapturePool &p) { p.slotLayouts.pop_back(); }, "pool.slotCount"},
        {"slot out of position", 3, [](CapturePool &p) { p.slotLayouts[1].slotIndex = 5; }, "pool.slotIndex"},
        {"non-zero template ID", 3, [](CapturePool &p) { p.slotLayouts[2].planes[0].dmaBufObjectId = 101; },
         "pool.templateObjectId"},
        {"oversized backing", 3, [](CapturePool &p) { p.backingSize.width = kWidth + 2; }, "pool.backingSize"},
        {"zero backing", 3, [](CapturePool &p) { p.backingSize.height = 0; }, "pool.backingSize"},
        {"count above the limits", 5, [](CapturePool &) {}, "pool.requestedCount"},
    };

    for (const auto &kCase : kCases)
    {
        CapturePool pool;
        CaptureRecordFixtures::makeNv12Pool(pool, kCase.requested);
        kCase.change(pool);
        expectResult(validatePool(pool, CaptureRecordFixtures::nv12Capabilities(1, 4), kCase.requested),
                     kCase.failedRule, kCase.name);
    }
}
