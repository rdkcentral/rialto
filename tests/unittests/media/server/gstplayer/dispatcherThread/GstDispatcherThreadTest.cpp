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

#include "GstDispatcherThread.h"
#include "GstDispatcherThreadClientMock.h"
#include "GstWrapperMock.h"
#include <condition_variable>
#include <gst/gst.h>
#include <gtest/gtest.h>
#include <memory>
#include <mutex>

using namespace firebolt::rialto::server;
using namespace firebolt::rialto::wrappers;

using ::testing::_;
using ::testing::ByMove;
using ::testing::DoAll;
using ::testing::InSequence;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::StrictMock;

class GstDispatcherThreadTest : public ::testing::Test
{
protected:
    GstElement m_pipeline{};
    StrictMock<firebolt::rialto::server::GstDispatcherThreadClientMock> m_client;
    std::shared_ptr<StrictMock<GstWrapperMock>> m_gstWrapperMock{std::make_shared<StrictMock<GstWrapperMock>>()};

    std::mutex m_dispatcherThreadMutex;
    std::condition_variable m_dispatcherThreadCond;
    bool m_dispatcherThreadDone{false};
    GstBus m_bus{};
    GstMessage m_message{};
};

/**
 * Test when gstBusTimedPopFiltered exits with timeout
 */
TEST_F(GstDispatcherThreadTest, PollTimeout)
{
    GST_MESSAGE_SRC(&m_message) = GST_OBJECT(&m_pipeline);
    GST_MESSAGE_TYPE(&m_message) = GST_MESSAGE_ERROR;
    EXPECT_CALL(*m_gstWrapperMock, gstPipelineGetBus(GST_PIPELINE(&m_pipeline))).WillOnce(Return(&m_bus));
    {
        InSequence seq;
        EXPECT_CALL(*m_gstWrapperMock, gstBusTimedPopFiltered(&m_bus, 100 * GST_MSECOND, _)).WillOnce(Return(nullptr));
        EXPECT_CALL(*m_gstWrapperMock, gstBusTimedPopFiltered(&m_bus, 100 * GST_MSECOND, _)).WillOnce(Return(&m_message));
        EXPECT_CALL(m_client, handleBusMessage(_));
    }

    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_bus))
        .WillOnce(Invoke(
            [this](gpointer bus)
            {
                std::unique_lock<std::mutex> lock(m_dispatcherThreadMutex);
                m_dispatcherThreadDone = true;
                m_dispatcherThreadCond.notify_all();
            }));

    auto sut = std::make_unique<GstDispatcherThread>(m_client, &m_pipeline, m_gstWrapperMock);

    // wait for dispatcher thread
    std::unique_lock<std::mutex> dispatcherLock(m_dispatcherThreadMutex);
    bool status = m_dispatcherThreadCond.wait_for(dispatcherLock, std::chrono::milliseconds(200),
                                                  [this]() { return m_dispatcherThreadDone; });
    EXPECT_TRUE(status);
}
#if 0
/**
 * Test that a GST_MESSAGE_STATE_CHANGED message is handled correctly.
 */
TEST_F(GstDispatcherThreadTest, StateChangedToPaused)
{
    GST_MESSAGE_SRC(&m_message) = GST_OBJECT(&m_pipeline);
    GST_MESSAGE_TYPE(&m_message) = GST_MESSAGE_STATE_CHANGED;

    GstState oldState = GST_STATE_READY;
    GstState newState = GST_STATE_PAUSED;
    GstState pending = GST_STATE_VOID_PENDING;

    GstMessage messageError = {};
    GST_MESSAGE_SRC(&messageError) = GST_OBJECT(&m_pipeline);
    GST_MESSAGE_TYPE(&messageError) = GST_MESSAGE_ERROR;

    EXPECT_CALL(*m_gstWrapperMock, gstPipelineGetBus(GST_PIPELINE(&m_pipeline))).WillOnce(Return(&m_bus));

    EXPECT_CALL(*m_gstWrapperMock, gstMessageParseStateChanged(&m_message, _, _, _))
        .WillOnce(DoAll(SetArgPointee<1>(oldState), SetArgPointee<2>(newState), SetArgPointee<3>(pending)));
    std::unique_ptr<IPlayerTask> stateChangeTask{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*stateChangeTask), execute());
    std::unique_ptr<IPlayerTask> errorTask{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*errorTask), execute());
    EXPECT_CALL(m_workerThreadMock, enqueueTask(_))
        .WillRepeatedly(Invoke([](std::unique_ptr<IPlayerTask> &&task) { task->execute(); }));

    {
        InSequence seq;
        EXPECT_CALL(*m_gstWrapperMock, gstBusTimedPopFiltered(&m_bus, 100 * GST_MSECOND, _)).WillOnce(Return(&m_message));
        EXPECT_CALL(m_taskFactoryMock, createHandleBusMessage(_, _, _)).WillOnce(Return(ByMove(std::move(stateChangeTask))));
        EXPECT_CALL(*m_gstWrapperMock, gstBusTimedPopFiltered(&m_bus, 100 * GST_MSECOND, _)).WillOnce(Return(&messageError));
        EXPECT_CALL(m_taskFactoryMock, createHandleBusMessage(_, _, _)).WillOnce(Return(ByMove(std::move(errorTask))));
    }

    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_bus))
        .WillOnce(Invoke(
            [this](gpointer bus)
            {
                std::unique_lock<std::mutex> lock(m_dispatcherThreadMutex);
                m_dispatcherThreadDone = true;
                m_dispatcherThreadCond.notify_all();
            }));

    auto sut = std::make_unique<GstDispatcherThread>(m_client, &m_pipeline, m_gstWrapperMock);

    // wait for dispatcher thread
    std::unique_lock<std::mutex> dispatcherLock(m_dispatcherThreadMutex);
    bool status = m_dispatcherThreadCond.wait_for(dispatcherLock, std::chrono::milliseconds(200),
                                                  [this]() { return m_dispatcherThreadDone; });
    EXPECT_TRUE(status);
}

/**
 * Test that a GST_MESSAGE_STATE_CHANGED message is handled correctly.
 */
TEST_F(GstDispatcherThreadTest, StateChangedToStop)
{
    GST_MESSAGE_SRC(&m_message) = GST_OBJECT(&m_pipeline);
    GST_MESSAGE_TYPE(&m_message) = GST_MESSAGE_STATE_CHANGED;

    GstState oldState = GST_STATE_PLAYING;
    GstState newState = GST_STATE_NULL;
    GstState pending = GST_STATE_VOID_PENDING;

    EXPECT_CALL(*m_gstWrapperMock, gstMessageParseStateChanged(&m_message, _, _, _))
        .WillOnce(DoAll(SetArgPointee<1>(oldState), SetArgPointee<2>(newState), SetArgPointee<3>(pending)));
    std::unique_ptr<IPlayerTask> stateChangeTask{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(m_workerThreadMock, enqueueTask(_))
        .WillOnce(Invoke([](std::unique_ptr<IPlayerTask> &&task) { task->execute(); }));
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*stateChangeTask), execute());
    EXPECT_CALL(*m_gstWrapperMock, gstPipelineGetBus(GST_PIPELINE(&m_pipeline))).WillOnce(Return(&m_bus));
    EXPECT_CALL(*m_gstWrapperMock, gstBusTimedPopFiltered(&m_bus, 100 * GST_MSECOND, _)).WillOnce(Return(&m_message));
    EXPECT_CALL(m_taskFactoryMock, createHandleBusMessage(_, _, _)).WillOnce(Return(ByMove(std::move(stateChangeTask))));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_bus))
        .WillOnce(Invoke(
            [this](gpointer bus)
            {
                std::unique_lock<std::mutex> lock(m_dispatcherThreadMutex);
                m_dispatcherThreadDone = true;
                m_dispatcherThreadCond.notify_all();
            }));

    auto sut = std::make_unique<GstDispatcherThread>(m_client, &m_pipeline, m_gstWrapperMock);

    // wait for dispatcher thread
    std::unique_lock<std::mutex> dispatcherLock(m_dispatcherThreadMutex);
    bool status = m_dispatcherThreadCond.wait_for(dispatcherLock, std::chrono::milliseconds(200),
                                                  [this]() { return m_dispatcherThreadDone; });
    EXPECT_TRUE(status);
}

/**
 * Test that a GST_MESSAGE_ERROR message is handled correctly.
 */
TEST_F(GstDispatcherThreadTest, Error)
{
    GST_MESSAGE_SRC(&m_message) = GST_OBJECT(&m_pipeline);
    GST_MESSAGE_TYPE(&m_message) = GST_MESSAGE_ERROR;
    std::unique_ptr<IPlayerTask> errorTask{std::make_unique<StrictMock<PlayerTaskMock>>()};
    EXPECT_CALL(dynamic_cast<StrictMock<PlayerTaskMock> &>(*errorTask), execute());
    EXPECT_CALL(m_workerThreadMock, enqueueTask(_))
        .WillOnce(Invoke([](std::unique_ptr<IPlayerTask> &&task) { task->execute(); }));
    EXPECT_CALL(*m_gstWrapperMock, gstPipelineGetBus(GST_PIPELINE(&m_pipeline))).WillOnce(Return(&m_bus));
    EXPECT_CALL(*m_gstWrapperMock, gstBusTimedPopFiltered(&m_bus, 100 * GST_MSECOND, _)).WillOnce(Return(&m_message));
    EXPECT_CALL(m_taskFactoryMock, createHandleBusMessage(_, _, _)).WillOnce(Return(ByMove(std::move(errorTask))));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_bus))
        .WillOnce(Invoke(
            [this](gpointer bus)
            {
                std::unique_lock<std::mutex> lock(m_dispatcherThreadMutex);
                m_dispatcherThreadDone = true;
                m_dispatcherThreadCond.notify_all();
            }));

    auto sut = std::make_unique<GstDispatcherThread>(m_client, &m_pipeline, m_gstWrapperMock);

    // wait for dispatcher thread
    std::unique_lock<std::mutex> dispatcherLock(m_dispatcherThreadMutex);
    bool status = m_dispatcherThreadCond.wait_for(dispatcherLock, std::chrono::milliseconds(200),
                                                  [this]() { return m_dispatcherThreadDone; });
    EXPECT_TRUE(status);
}
#endif
