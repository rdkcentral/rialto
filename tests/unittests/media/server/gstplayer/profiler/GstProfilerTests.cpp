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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <string>

#include "GstProfiler.h"
#include "common/interface/IProfiler.h"
#include "common/interface/IProfilerFactory.h"
#include "wrappers/IGlibWrapper.h"
#include "wrappers/IGstWrapper.h"

#include "common/mocks/ProfilerFactoryMock.h"
#include "common/mocks/ProfilerMock.h"
#include "wrappers/mocks/GlibWrapperMock.h"
#include "wrappers/mocks/GstWrapperMock.h"

using namespace firebolt::rialto::server;

using ::testing::_;
using ::testing::ByMove;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::StrictMock;

class GstProfilerTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        gstWrapper = std::make_shared<NiceMock<firebolt::rialto::wrappers::GstWrapperMock>>();
        glibWrapper = std::make_shared<NiceMock<firebolt::rialto::wrappers::GlibWrapperMock>>();

        factoryMock = std::make_shared<StrictMock<firebolt::rialto::common::ProfilerFactoryMock>>();
        ASSERT_TRUE(factoryMock);
    }

    std::shared_ptr<firebolt::rialto::wrappers::IGstWrapper> gstWrapper;
    std::shared_ptr<firebolt::rialto::wrappers::IGlibWrapper> glibWrapper;
    std::shared_ptr<StrictMock<firebolt::rialto::common::ProfilerFactoryMock>> factoryMock;
};

TEST_F(GstProfilerTests, DisabledProfilerDoesNotRefPipeline)
{
    auto profilerMock = std::make_unique<StrictMock<firebolt::rialto::common::ProfilerMock>>();
    EXPECT_CALL(*profilerMock, enabled()).WillOnce(Return(false));

    EXPECT_CALL(*factoryMock, createProfiler(_)).WillOnce(Return(ByMove(std::move(profilerMock))));

    auto *pipeline = reinterpret_cast<GstElement *>(0x1234);

    // Disabled profiler: no pipeline ref/unref expected.
    GstProfiler gstProfiler{pipeline, gstWrapper, glibWrapper, factoryMock};
}

TEST_F(GstProfilerTests, EnabledProfilerRefsAndUnrefsPipeline)
{
    auto profilerMock = std::make_unique<StrictMock<firebolt::rialto::common::ProfilerMock>>();
    EXPECT_CALL(*profilerMock, enabled()).WillOnce(Return(true));

    EXPECT_CALL(*factoryMock, createProfiler(_)).WillOnce(Return(ByMove(std::move(profilerMock))));

    auto *pipeline = reinterpret_cast<GstElement *>(0x1234);

    auto gstWrapperMock = std::static_pointer_cast<NiceMock<firebolt::rialto::wrappers::GstWrapperMock>>(gstWrapper);

    EXPECT_CALL(*gstWrapperMock, gstObjectRef(pipeline));
    EXPECT_CALL(*gstWrapperMock, gstObjectUnref(pipeline));

    {
        GstProfiler gstProfiler{pipeline, gstWrapper, glibWrapper, factoryMock};
    }
}

TEST_F(GstProfilerTests, CreateRecordNoOpWhenDisabled)
{
    auto profilerMock = std::make_unique<StrictMock<firebolt::rialto::common::ProfilerMock>>();
    EXPECT_CALL(*profilerMock, enabled()).WillOnce(Return(false));

    EXPECT_CALL(*factoryMock, createProfiler(_)).WillOnce(Return(ByMove(std::move(profilerMock))));

    GstProfiler gstProfiler{nullptr, gstWrapper, glibWrapper, factoryMock};

    EXPECT_FALSE(gstProfiler.createRecord("Stage").has_value());
    EXPECT_FALSE(gstProfiler.createRecord("Stage", "Info").has_value());

    gstProfiler.logRecord(123U); // no-op when disabled
}

TEST_F(GstProfilerTests, CreateRecordForwardsWhenEnabled)
{
    constexpr firebolt::rialto::common::IProfiler::RecordId kRecordId{123U};

    auto profilerMock = std::make_unique<StrictMock<firebolt::rialto::common::ProfilerMock>>();
    EXPECT_CALL(*profilerMock, enabled()).WillOnce(Return(true));
    EXPECT_CALL(*profilerMock, record(std::string{"StageOnly"}))
        .WillOnce(Return(std::optional<firebolt::rialto::common::IProfiler::RecordId>{kRecordId}));
    EXPECT_CALL(*profilerMock, log(kRecordId));

    EXPECT_CALL(*factoryMock, createProfiler(_)).WillOnce(Return(ByMove(std::move(profilerMock))));

    GstProfiler gstProfiler{nullptr, gstWrapper, glibWrapper, factoryMock};

    const auto id = gstProfiler.createRecord("StageOnly");
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(*id, kRecordId);

    gstProfiler.logRecord(*id);
}

TEST_F(GstProfilerTests, CreateRecordWithInfoForwardsWhenEnabled)
{
    constexpr firebolt::rialto::common::IProfiler::RecordId kRecordId{456U};

    auto profilerMock = std::make_unique<StrictMock<firebolt::rialto::common::ProfilerMock>>();
    EXPECT_CALL(*profilerMock, enabled()).WillOnce(Return(true));
    EXPECT_CALL(*profilerMock, record(std::string{"StageWithInfo"}, std::string{"InfoValue"}))
        .WillOnce(Return(std::optional<firebolt::rialto::common::IProfiler::RecordId>{kRecordId}));
    EXPECT_CALL(*profilerMock, log(kRecordId));

    EXPECT_CALL(*factoryMock, createProfiler(_)).WillOnce(Return(ByMove(std::move(profilerMock))));

    GstProfiler gstProfiler{nullptr, gstWrapper, glibWrapper, factoryMock};

    const auto id = gstProfiler.createRecord("StageWithInfo", "InfoValue");
    ASSERT_TRUE(id.has_value());
    EXPECT_EQ(*id, kRecordId);

    gstProfiler.logRecord(*id);
}

TEST_F(GstProfilerTests, ScheduleNullElementIsSafe)
{
    auto profilerMock = std::make_unique<StrictMock<firebolt::rialto::common::ProfilerMock>>();
    EXPECT_CALL(*profilerMock, enabled()).WillOnce(Return(true));

    EXPECT_CALL(*factoryMock, createProfiler(_)).WillOnce(Return(ByMove(std::move(profilerMock))));

    GstProfiler gstProfiler{nullptr, gstWrapper, glibWrapper, factoryMock};

    gstProfiler.scheduleGstElementRecord(nullptr);
}
