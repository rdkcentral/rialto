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

#include <cstdint>
#include <functional>
#include <limits>
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

namespace
{
constexpr uint32_t kAdmittedSlots{3};

using FrameBuilder = void (CaptureRecordFixtures::*)(CapturedFrame &, uint32_t, uint64_t);

struct FrameCase
{
    std::string name;
    FrameBuilder build;
    std::function<void(CapturedFrame &, CaptureRecordFixtures &)> change;
    std::string failedRule;
};

void runFrameCases(const std::vector<FrameCase> &cases)
{
    for (const auto &kCase : cases)
    {
        CaptureRecordFixtures fixtures;
        CapturedFrame frame;
        (fixtures.*kCase.build)(frame, 1, 7);
        kCase.change(frame, fixtures);
        expectResult(validateFrame(frame, kAdmittedSlots), kCase.failedRule, kCase.name);
    }
}

void noChange(CapturedFrame &, CaptureRecordFixtures &) {}
} // namespace

TEST(CaptureRecordValidatorTest, ShouldAcceptSpecExamples)
{
    runFrameCases({
        {"example A", &CaptureRecordFixtures::makeExampleA, noChange, ""},
        {"example B", &CaptureRecordFixtures::makeExampleB, noChange, ""},
        {"example C", &CaptureRecordFixtures::makeExampleC, noChange, ""},
        {"example D", &CaptureRecordFixtures::makeExampleD, noChange, ""},
    });
}

TEST(CaptureRecordValidatorTest, ShouldCheckFrameIdentityAndDescriptors)
{
    runFrameCases({
        {"slot outside the pool", &CaptureRecordFixtures::makeExampleB,
         [](CapturedFrame &f, CaptureRecordFixtures &) { f.slotIndex = kAdmittedSlots; }, "frame.slotIndex"},
        {"zero token", &CaptureRecordFixtures::makeExampleB,
         [](CapturedFrame &f, CaptureRecordFixtures &) { f.acquisitionToken = 0; }, "frame.acquisitionToken"},
        {"no planes", &CaptureRecordFixtures::makeExampleB,
         [](CapturedFrame &f, CaptureRecordFixtures &) { f.planes.clear(); }, "frame.planes"},
        {"zero object ID", &CaptureRecordFixtures::makeExampleB,
         [](CapturedFrame &f, CaptureRecordFixtures &)
         {
             f.dmaBufObjects[0].objectId = 0;
             f.planes[0].dmaBufObjectId = f.planes[1].dmaBufObjectId = 0;
             f.exportedDmaBufs[0].dmaBufObjectId = 0;
         },
         "frame.objectId"},
        {"object listed twice", &CaptureRecordFixtures::makeExampleB, [](CapturedFrame &f, CaptureRecordFixtures &)
         { f.dmaBufObjects.push_back(f.dmaBufObjects[0]); }, "frame.duplicateObject"},
        {"plane references unlisted object", &CaptureRecordFixtures::makeExampleB,
         [](CapturedFrame &f, CaptureRecordFixtures &) { f.planes[1].dmaBufObjectId = 999; }, "frame.planeObject"},
        {"listed object not referenced", &CaptureRecordFixtures::makeExampleC,
         [](CapturedFrame &f, CaptureRecordFixtures &) { f.planes[1].dmaBufObjectId = kFirstObjectId; },
         "frame.unreferencedObject"},
        {"duplicate descriptor", &CaptureRecordFixtures::makeExampleB, [](CapturedFrame &f, CaptureRecordFixtures &fx)
         { f.exportedDmaBufs.push_back({kFirstObjectId, fx.createFile(16)}); }, "frame.duplicateDescriptor"},
        {"unrelated descriptor", &CaptureRecordFixtures::makeExampleB, [](CapturedFrame &f, CaptureRecordFixtures &fx)
         { f.exportedDmaBufs.push_back({999, fx.createFile(16)}); }, "frame.unrelatedDescriptor"},
        {"missing descriptor", &CaptureRecordFixtures::makeExampleC,
         [](CapturedFrame &f, CaptureRecordFixtures &) { f.exportedDmaBufs.pop_back(); }, "frame.missingDescriptor"},
        {"negative descriptor", &CaptureRecordFixtures::makeExampleA,
         [](CapturedFrame &f, CaptureRecordFixtures &) { f.exportedDmaBufs[0].fd = -1; }, "frame.invalidDescriptor"},
    });
}

TEST(CaptureRecordValidatorTest, ShouldCheckRangesWithoutOverflow)
{
    constexpr uint64_t kMax64{std::numeric_limits<uint64_t>::max()};
    constexpr uint32_t kMax32{std::numeric_limits<uint32_t>::max()};
    runFrameCases({
        {"plane ends exactly at object end", &CaptureRecordFixtures::makeExampleA,
         [](CapturedFrame &f, CaptureRecordFixtures &) { f.planes[0].lengthBytes = f.dmaBufObjects[0].sizeBytes; }, ""},
        {"plane one byte too long", &CaptureRecordFixtures::makeExampleA, [](CapturedFrame &f, CaptureRecordFixtures &)
         { f.planes[0].lengthBytes = f.dmaBufObjects[0].sizeBytes + 1; }, "frame.planeRange"},
        {"offset past object end", &CaptureRecordFixtures::makeExampleB,
         [](CapturedFrame &f, CaptureRecordFixtures &)
         {
             f.planes[1].offsetBytes = f.dmaBufObjects[0].sizeBytes + 1;
             f.planes[1].lengthBytes = 0;
         },
         "frame.planeRange"},
        {"offset plus length wraps 64 bits", &CaptureRecordFixtures::makeExampleB,
         [=](CapturedFrame &f, CaptureRecordFixtures &) { f.planes[1].lengthBytes = kMax64 - 4; }, "frame.planeRange"},
        {"visible region exactly the backing", &CaptureRecordFixtures::makeExampleB,
         [](CapturedFrame &f, CaptureRecordFixtures &) { f.visibleRegion = Rectangle{0, 0, f.backingSize}; }, ""},
        {"visible width too large", &CaptureRecordFixtures::makeExampleB, [](CapturedFrame &f, CaptureRecordFixtures &)
         { f.visibleRegion.size.width = f.backingSize.width + 1; }, "frame.visibleRegion"},
        {"visible height too large from its origin", &CaptureRecordFixtures::makeExampleB,
         [](CapturedFrame &f, CaptureRecordFixtures &)
         {
             f.visibleRegion.y = 16;
             f.visibleRegion.size.height = f.backingSize.height - 8;
         },
         "frame.visibleRegion"},
        {"visible x plus width wraps 32 bits", &CaptureRecordFixtures::makeExampleB,
         [=](CapturedFrame &f, CaptureRecordFixtures &)
         {
             f.visibleRegion.x = 2;
             f.visibleRegion.size.width = kMax32;
         },
         "frame.visibleRegion"},
    });
}
