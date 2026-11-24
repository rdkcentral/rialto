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

#include "CaptureAdmission.h"
#include "CaptureAdmissionMock.h"
#include "CaptureSessionMock.h"
#include "ICaptureAdmission.h"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdlib>
#include <set>

using firebolt::rialto::capture::CaptureCapabilities;
using firebolt::rialto::capture::CapturePool;
using firebolt::rialto::capture::CaptureSessionMock;
using firebolt::rialto::capture::CaptureStatus;
using firebolt::rialto::server::CaptureAdmission;
using firebolt::rialto::server::CaptureAdmissionMock;
using firebolt::rialto::server::CaptureSessionHandle;
using firebolt::rialto::server::ICaptureAdmission;
using firebolt::rialto::server::ICaptureAdmissionFactory;
using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::StrictMock;

namespace
{
CaptureSessionHandle handleWithSession(bool &deleted)
{
    CaptureSessionHandle handle;
    handle.sessionId = 4;
    handle.captureId = 9;
    handle.session =
        firebolt::rialto::wrappers::CaptureSessionPtr{new ::testing::NiceMock<firebolt::rialto::capture::CaptureSessionMock>(),
                                                      [&deleted](firebolt::rialto::capture::ICaptureSession *session)
                                                      {
                                                          deleted = true;
                                                          delete session;
                                                      }};
    return handle;
}
} // namespace

TEST(CaptureAdmissionTest, MockAdmissionReceivesTheConfiguredSession)
{
    bool deleted{false};
    CaptureSessionHandle handle{handleWithSession(deleted)};
    std::unique_ptr<ICaptureAdmission> admission{std::make_unique<CaptureAdmissionMock>()};
    EXPECT_CALL(static_cast<CaptureAdmissionMock &>(*admission), admit(_))
        .WillOnce(Invoke(
            [](CaptureSessionHandle &admitted)
            {
                EXPECT_EQ(admitted.sessionId, 4);
                EXPECT_EQ(admitted.captureId, 9U);
                EXPECT_NE(admitted.session, nullptr);
                return true;
            }));
    EXPECT_TRUE(admission->admit(handle));
    EXPECT_FALSE(deleted); // the handle still owns the session
}

namespace
{
constexpr const char *kReadyTimeout{"RIALTO_VIDEO_FRAME_CAPTURE_READY_TIMEOUT_MS"};
constexpr const char *kTargetSlots{"RIALTO_VIDEO_FRAME_CAPTURE_TARGET_SLOTS"};

CaptureCapabilities capabilities(std::uint32_t minimumSlots, std::uint32_t maximumSlots, std::uint32_t maxWidth = 1920,
                                 std::uint32_t maxHeight = 1088)
{
    CaptureCapabilities caps{};
    caps.drmFormat = 0x3231564e; // NV12
    caps.drmModifier = 0;
    caps.maximumContentSize = {maxWidth, maxHeight};
    caps.minimumSlots = minimumSlots;
    caps.maximumSlots = maximumSlots;
    return caps;
}

CapturePool poolOf(std::uint32_t slots)
{
    CapturePool pool{};
    pool.backingSize = {1920, 1088};
    for (std::uint32_t i = 0; i < slots; ++i)
    {
        firebolt::rialto::capture::CaptureSlotLayout layout{};
        layout.slotIndex = i;
        pool.slotLayouts.push_back(layout);
    }
    return pool;
}
} // namespace

class CaptureAdmissionRealTest : public ::testing::Test
{
protected:
    CaptureAdmissionRealTest()
    {
        m_handle.sessionId = 4;
        m_handle.captureId = 9;
        m_handle.videoRequirements = {1920, 1080};
        m_handle.session =
            firebolt::rialto::wrappers::CaptureSessionPtr{m_session,
                                                          [](firebolt::rialto::capture::ICaptureSession *session)
                                                          { delete session; }};
    }
    void TearDown() override
    {
        unsetenv(kReadyTimeout);
        unsetenv(kTargetSlots);
    }

    void halReports(const CaptureCapabilities &caps)
    {
        EXPECT_CALL(*m_session, getCapabilities(_)).WillOnce(DoAll(SetArgReferee<0>(caps), Return(CaptureStatus::OK)));
    }
    void halCreatesPool(std::uint32_t slots)
    {
        EXPECT_CALL(*m_session, createPool(slots, _))
            .WillOnce(DoAll(SetArgReferee<1>(poolOf(slots)), Return(CaptureStatus::OK)));
    }

    // time passes only when admission sleeps
    std::chrono::steady_clock::time_point m_now{};
    int m_sleeps{0};
    CaptureAdmission m_sut{[this]() { return m_now; },
                           [this](std::chrono::milliseconds duration)
                           {
                               ++m_sleeps;
                               m_now += duration;
                           }};
    StrictMock<CaptureSessionMock> *m_session{new StrictMock<CaptureSessionMock>()};
    CaptureSessionHandle m_handle;
};

TEST_F(CaptureAdmissionRealTest, AdmitsWithTheChosenSlotCount)
{
    halReports(capabilities(1, 4));
    halCreatesPool(3);
    EXPECT_TRUE(m_sut.admit(m_handle));
    EXPECT_NE(m_handle.poolId, 0u);
    EXPECT_EQ(m_handle.pool.slotLayouts.size(), 3u);
    EXPECT_EQ(m_handle.capabilities.maximumSlots, 4u);
    EXPECT_EQ(m_handle.capabilities.drmFormat, 0x3231564eu);
    EXPECT_NE(m_handle.session, nullptr);
}

TEST_F(CaptureAdmissionRealTest, ReadyOnTheThirdTry)
{
    EXPECT_CALL(*m_session, getCapabilities(_))
        .WillOnce(Return(CaptureStatus::CAPABILITIES_NOT_AVAILABLE_YET))
        .WillOnce(Return(CaptureStatus::CAPABILITIES_NOT_AVAILABLE_YET))
        .WillOnce(DoAll(SetArgReferee<0>(capabilities(1, 4)), Return(CaptureStatus::OK)));
    halCreatesPool(3);
    EXPECT_TRUE(m_sut.admit(m_handle));
    EXPECT_EQ(m_sleeps, 2);
}

TEST_F(CaptureAdmissionRealTest, NeverReadyFailsAtTheTimeout)
{
    setenv(kReadyTimeout, "200", 1);
    EXPECT_CALL(*m_session, getCapabilities(_)).WillRepeatedly(Return(CaptureStatus::CAPABILITIES_NOT_AVAILABLE_YET));
    EXPECT_FALSE(m_sut.admit(m_handle)); // no createPool: the strict mock would flag it
    EXPECT_EQ(m_now - std::chrono::steady_clock::time_point{}, std::chrono::milliseconds{200});
    EXPECT_EQ(m_sleeps, 200 / CaptureAdmission::kRetryInterval.count());
    EXPECT_EQ(m_handle.poolId, 0u);
}

TEST_F(CaptureAdmissionRealTest, HalErrorFailsWithoutRetry)
{
    EXPECT_CALL(*m_session, getCapabilities(_)).WillOnce(Return(CaptureStatus::FATAL_ERROR));
    EXPECT_FALSE(m_sut.admit(m_handle));
    EXPECT_EQ(m_sleeps, 0);
}

TEST_F(CaptureAdmissionRealTest, InvertedLimitsFail)
{
    halReports(capabilities(4, 2));
    EXPECT_FALSE(m_sut.admit(m_handle));
}

TEST_F(CaptureAdmissionRealTest, LargerThanTheCaptureSupportsFails)
{
    m_handle.videoRequirements = {3840, 2160};
    halReports(capabilities(1, 4, 1920, 1088));
    EXPECT_FALSE(m_sut.admit(m_handle));
}

TEST_F(CaptureAdmissionRealTest, WithinTheCaptureLimitsIsAdmitted)
{
    m_handle.videoRequirements = {1920, 1080};
    halReports(capabilities(1, 4, 1920, 1088));
    halCreatesPool(3);
    EXPECT_TRUE(m_sut.admit(m_handle));
}

TEST_F(CaptureAdmissionRealTest, ConfiguredTargetIsUsed)
{
    setenv(kTargetSlots, "2", 1);
    halReports(capabilities(1, 4));
    halCreatesPool(2);
    EXPECT_TRUE(m_sut.admit(m_handle));
}

TEST_F(CaptureAdmissionRealTest, SingleSlotHalFails)
{
    halReports(capabilities(1, 1));
    EXPECT_FALSE(m_sut.admit(m_handle));
}

TEST_F(CaptureAdmissionRealTest, PoolRefusalFailsAndIsNotRetried)
{
    halReports(capabilities(1, 4));
    EXPECT_CALL(*m_session, createPool(3, _)).WillOnce(Return(CaptureStatus::NO_RESOURCES));
    EXPECT_FALSE(m_sut.admit(m_handle));
    EXPECT_EQ(m_handle.poolId, 0u);
}

TEST_F(CaptureAdmissionRealTest, WrongSlotCountFails)
{
    halReports(capabilities(1, 4));
    EXPECT_CALL(*m_session, createPool(3, _)).WillOnce(DoAll(SetArgReferee<1>(poolOf(2)), Return(CaptureStatus::OK)));
    EXPECT_FALSE(m_sut.admit(m_handle));
}

TEST_F(CaptureAdmissionRealTest, NoSessionFails)
{
    m_handle.session.reset();
    m_session = nullptr;
    EXPECT_FALSE(m_sut.admit(m_handle));
}

TEST(CaptureAdmissionPoolIdTest, PoolIdsAreNonZeroAndDistinct)
{
    std::set<std::uint32_t> ids;
    for (int i = 0; i < 100; ++i)
    {
        const std::uint32_t kId{firebolt::rialto::server::nextCapturePoolId()};
        EXPECT_NE(kId, 0u);
        EXPECT_TRUE(ids.insert(kId).second);
    }
}

TEST(CaptureAdmissionPoolIdTest, DefaultFactoryCreatesTheRealAdmission)
{
    auto admission = ICaptureAdmissionFactory::instance()->createCaptureAdmission();
    EXPECT_NE(dynamic_cast<CaptureAdmission *>(admission.get()), nullptr);
}
