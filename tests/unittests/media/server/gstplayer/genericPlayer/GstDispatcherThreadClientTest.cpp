/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Sky UK
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

#include "FlushWatcherMock.h"
#include "GstGenericPlayerTestCommon.h"
#include "IGstDispatcherThreadClient.h"
#include "PlayerTaskMock.h"

using testing::_;
using testing::ByMove;
using testing::Invoke;
using testing::Ref;
using testing::Return;

class GstDispatcherThreadClientTest : public GstGenericPlayerTestCommon
{
protected:
    std::unique_ptr<IGstDispatcherThreadClient> m_sut;
    VideoRequirements m_videoReq = {kMinPrimaryVideoWidth, kMinPrimaryVideoHeight};
    const bool m_kIsLive{false};

    GstDispatcherThreadClientTest()
    {
        gstPlayerWillBeCreated();
        m_sut = std::make_unique<GstGenericPlayer>(&m_gstPlayerClient, m_decryptionServiceMock, MediaType::MSE,
                                                   m_videoReq, m_kIsLive, m_gstWrapperMock, m_glibWrapperMock,
                                                   m_rdkGstreamerUtilsWrapperMock, m_gstInitialiserMock,
                                                   std::move(m_flushWatcher), m_gstSrcFactoryMock, m_timerFactoryMock,
                                                   std::move(m_taskFactory), std::move(workerThreadFactory),
                                                   std::move(gstDispatcherThreadFactory),
                                                   m_gstProtectionMetadataFactoryMock);
    }

    ~GstDispatcherThreadClientTest() override
    {
        gstPlayerWillBeDestroyed();
        m_sut.reset();
    }
};

TEST_F(GstDispatcherThreadClientTest, shouldHandleBusMessage)
{
    GstMessage message{};
    std::unique_ptr<IPlayerTask> messageTask{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*messageTask), execute());
    EXPECT_CALL(m_workerThreadMock, enqueueTask(_))
        .WillRepeatedly(Invoke([](std::unique_ptr<IPlayerTask> &&task) { task->execute(); }));
    EXPECT_CALL(m_taskFactoryMock, createHandleBusMessage(_, _, &message, _))
        .WillOnce(Return(ByMove(std::move(messageTask))));

    m_sut->handleBusMessage(&message, false);
}

TEST_F(GstDispatcherThreadClientTest, shouldEnqueuePriorityBusMessageAsPriorityTask)
{
    GstMessage message{};
    std::unique_ptr<IPlayerTask> messageTask{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*messageTask), execute());
    EXPECT_CALL(m_flushWatcherMock, isFlushOngoing()).WillOnce(Return(false));
    EXPECT_CALL(m_workerThreadMock, enqueuePriorityTask(_))
        .WillOnce(Invoke([](std::unique_ptr<IPlayerTask> &&task) { task->execute(); }));
    EXPECT_CALL(m_taskFactoryMock, createHandleBusMessage(_, _, &message, _))
        .WillOnce(Return(ByMove(std::move(messageTask))));

    m_sut->handleBusMessage(&message, true);
}

TEST_F(GstDispatcherThreadClientTest, shouldNotPrioritiseBusMessagesAfterFlushWasOngoing)
{
    GstMessage firstMessage{};
    GstMessage secondMessage{};
    std::unique_ptr<IPlayerTask> firstTask{std::make_unique<StrictMock<PlayerTaskMock>>()};
    std::unique_ptr<IPlayerTask> secondTask{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*firstTask), execute());
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*secondTask), execute());
    EXPECT_CALL(m_flushWatcherMock, isFlushOngoing()).WillOnce(Return(true));
    EXPECT_CALL(m_workerThreadMock, enqueueTask(_))
        .WillRepeatedly(Invoke([](std::unique_ptr<IPlayerTask> &&task) { task->execute(); }));
    EXPECT_CALL(m_taskFactoryMock, createHandleBusMessage(_, _, &firstMessage, _))
        .WillOnce(Return(ByMove(std::move(firstTask))));
    EXPECT_CALL(m_taskFactoryMock, createHandleBusMessage(_, _, &secondMessage, _))
        .WillOnce(Return(ByMove(std::move(secondTask))));

    m_sut->handleBusMessage(&firstMessage, true);
    m_sut->handleBusMessage(&secondMessage, true);
}

TEST_F(GstDispatcherThreadClientTest, shouldNotPrioritiseBusMessageAfterSetPosition)
{
    constexpr std::int64_t kPosition{1234};
    GstMessage message{};
    std::unique_ptr<IPlayerTask> setPositionTask{std::make_unique<StrictMock<PlayerTaskMock>>()};
    std::unique_ptr<IPlayerTask> messageTask{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*setPositionTask), execute());
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*messageTask), execute());
    EXPECT_CALL(m_workerThreadMock, enqueueTask(_))
        .WillRepeatedly(Invoke([](std::unique_ptr<IPlayerTask> &&task) { task->execute(); }));
    EXPECT_CALL(m_taskFactoryMock, createSetPosition(_, _, kPosition)).WillOnce(Return(ByMove(std::move(setPositionTask))));
    EXPECT_CALL(m_taskFactoryMock, createHandleBusMessage(_, _, &message, _))
        .WillOnce(Return(ByMove(std::move(messageTask))));

    dynamic_cast<GstGenericPlayer &>(*m_sut).setPosition(kPosition);
    m_sut->handleBusMessage(&message, true);
}
