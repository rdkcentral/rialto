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

#include "CaptureSessionRuntime.h"
#include "CaptureRecordFixtures.h"
#include "CaptureSessionMock.h"

#include <gtest/gtest.h>

using namespace firebolt::rialto;
using namespace firebolt::rialto::server;
using firebolt::rialto::capture::test::CaptureRecordFixtures;
using ::testing::_;
using ::testing::Invoke;
using ::testing::NiceMock;
using ::testing::Return;

namespace
{
capture::CapturePool poolOf(std::uint32_t slots)
{
    capture::CapturePool pool;
    pool.backingSize = {1920, 1088};
    for (std::uint32_t slot = 0; slot < slots; ++slot)
        pool.slotLayouts.push_back(capture::CaptureSlotLayout{slot, {}});
    return pool;
}

wrappers::CaptureSessionPtr sessionTracking(bool &deleted)
{
    return wrappers::CaptureSessionPtr{new NiceMock<capture::CaptureSessionMock>(),
                                       [&deleted](capture::ICaptureSession *session)
                                       {
                                           deleted = true;
                                           delete session;
                                       }};
}
} // namespace

TEST(CaptureSessionRuntimeTest, ShouldKeepOneEmptySlotPerAdmittedSlot)
{
    bool deleted{false};
    {
        CaptureSessionRuntime runtime{sessionTracking(deleted), 4, poolOf(3)};
        EXPECT_EQ(runtime.poolId(), 4U);
        EXPECT_EQ(runtime.slotCount(), 3U);
        EXPECT_EQ(runtime.pool().backingSize.width, 1920U);
        for (std::uint32_t slot = 0; slot < 4; ++slot)
            EXPECT_FALSE(runtime.isSlotLocked(slot)); // slot 3 is outside the pool
        EXPECT_FALSE(runtime.isFatal());
        EXPECT_FALSE(runtime.isFlushPending());
        EXPECT_FALSE(deleted);
    }
    EXPECT_TRUE(deleted); // the runtime owns the HAL session
}

class CaptureSessionRuntimeDeliveryTest : public ::testing::Test
{
protected:
    CaptureSessionRuntimeDeliveryTest()
    {
        capture::CapturePool pool;
        CaptureRecordFixtures::makeNv12Pool(pool, 3);
        m_runtime =
            std::make_unique<CaptureSessionRuntime>(wrappers::CaptureSessionPtr{m_session,
                                                                                [](capture::ICaptureSession *session)
                                                                                { delete session; }},
                                                    1, pool);
    }

    // The HAL returns an Example B frame in the slot; the reply's flush flag is returned.
    bool acquire(std::uint32_t slot, std::uint64_t token, bool flushed, CaptureSessionRuntime::DeliveryTicket &ticket)
    {
        EXPECT_CALL(*m_session, acquireCurrentFrame(_))
            .WillOnce(Invoke(
                [&, slot, token, flushed](capture::CapturedFrame &frame)
                {
                    m_records.makeExampleB(frame, slot, token);
                    frame.dmaBufObjectsFlushed = flushed;
                    return capture::CaptureStatus::OK;
                }))
            .RetiresOnSaturation();
        bool replyFlushed{false};
        EXPECT_EQ(m_runtime->acquire([&](const capture::CapturedFrame &, bool flag, bool) { replyFlushed = flag; },
                                     ticket),
                  capture::CaptureStatus::OK);
        return replyFlushed;
    }

    CaptureRecordFixtures m_records;
    NiceMock<capture::CaptureSessionMock> *m_session{new NiceMock<capture::CaptureSessionMock>()};
    std::unique_ptr<CaptureSessionRuntime> m_runtime;
};

TEST_F(CaptureSessionRuntimeDeliveryTest, DeliveredClearsTheLatchAndKeepsTheLock)
{
    CaptureSessionRuntime::DeliveryTicket ticket;
    EXPECT_TRUE(acquire(0, 1, true, ticket));
    EXPECT_TRUE(m_runtime->isFlushPending());
    EXPECT_CALL(*m_session, releaseFrame(_)).Times(0);
    m_runtime->onDelivery(ticket, true);
    EXPECT_FALSE(m_runtime->isFlushPending());
    EXPECT_TRUE(m_runtime->isSlotLocked(0)); // the client holds it until it releases
}

TEST_F(CaptureSessionRuntimeDeliveryTest, NotDeliveredReleasesOnceAndKeepsTheLatch)
{
    CaptureSessionRuntime::DeliveryTicket ticket;
    EXPECT_TRUE(acquire(0, 1, true, ticket));
    EXPECT_CALL(*m_session, releaseFrame(_)).WillOnce(Return(capture::CaptureStatus::OK));
    m_runtime->onDelivery(ticket, false);
    m_runtime->onDelivery(ticket, false); // already resolved
    EXPECT_FALSE(m_runtime->isSlotLocked(0));
    EXPECT_TRUE(m_runtime->isFlushPending());

    // the next frame carries the invalidation although the HAL flag is false
    CaptureSessionRuntime::DeliveryTicket next;
    EXPECT_TRUE(acquire(1, 2, false, next));
    m_runtime->onDelivery(next, true);
    EXPECT_FALSE(m_runtime->isFlushPending());
}

TEST_F(CaptureSessionRuntimeDeliveryTest, NewerInvalidationIsNotLostWhenAnOlderReplyIsDelivered)
{
    CaptureSessionRuntime::DeliveryTicket older;
    EXPECT_FALSE(acquire(0, 1, false, older));
    CaptureSessionRuntime::DeliveryTicket newer;
    EXPECT_TRUE(acquire(1, 2, true, newer)); // a flush before the older reply's outcome is known
    m_runtime->onDelivery(older, true);
    EXPECT_TRUE(m_runtime->isFlushPending()); // the older reply did not carry it
    m_runtime->onDelivery(newer, true);
    EXPECT_FALSE(m_runtime->isFlushPending());
}

TEST_F(CaptureSessionRuntimeDeliveryTest, DescriptionChangeIsReportedUntilDelivered)
{
    auto acquireReportingChange = [this](std::uint32_t slot, std::uint64_t token, std::uint32_t backingHeight,
                                         CaptureSessionRuntime::DeliveryTicket &ticket)
    {
        EXPECT_CALL(*m_session, acquireCurrentFrame(_))
            .WillOnce(Invoke(
                [&, slot, token, backingHeight](capture::CapturedFrame &frame)
                {
                    m_records.makeExampleB(frame, slot, token);
                    CaptureRecordFixtures::setSingleObjectId(frame, 100 + slot); // each slot has its own memory
                    frame.backingSize.height = backingHeight;
                    return capture::CaptureStatus::OK;
                }))
            .RetiresOnSaturation();
        bool changed{false};
        EXPECT_EQ(m_runtime->acquire([&](const capture::CapturedFrame &, bool, bool flag) { changed = flag; }, ticket),
                  capture::CaptureStatus::OK);
        return changed;
    };
    CaptureSessionRuntime::DeliveryTicket first, second, resized, resizedAgain;
    EXPECT_TRUE(acquireReportingChange(0, 1, 1088, first)); // nothing delivered yet
    m_runtime->onDelivery(first, true);
    EXPECT_FALSE(acquireReportingChange(1, 2, 1088, second)); // unchanged (only the object differs)
    m_runtime->onDelivery(second, true);
    EXPECT_TRUE(acquireReportingChange(2, 3, 1080, resized)); // backing size changed
    m_runtime->onDelivery(resized, false);                    // not delivered: the client still has the old one
    EXPECT_TRUE(acquireReportingChange(2, 4, 1080, resizedAgain));
    m_runtime->onDelivery(resizedAgain, true);
}

TEST_F(CaptureSessionRuntimeDeliveryTest, RetiredSessionAnswersNoFrameWithoutAskingTheHal)
{
    CaptureSessionRuntime::DeliveryTicket held;
    acquire(0, 1, false, held);
    m_runtime->onDelivery(held, true);

    m_runtime->retire();
    EXPECT_TRUE(m_runtime->isRetired());
    EXPECT_CALL(*m_session, acquireCurrentFrame(_)).Times(0);
    CaptureSessionRuntime::DeliveryTicket ticket;
    EXPECT_EQ(m_runtime->acquire([](const capture::CapturedFrame &, bool, bool) {}, ticket),
              capture::CaptureStatus::NO_FRAME);
    EXPECT_TRUE(m_runtime->isSlotLocked(0)); // the frame the client holds stays locked
}
