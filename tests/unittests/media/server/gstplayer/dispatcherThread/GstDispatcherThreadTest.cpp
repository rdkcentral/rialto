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
#include "FlushOnPrerollControllerMock.h"
#include "GlibWrapperMock.h"
#include "GstDispatcherThreadClientMock.h"
#include "GstWrapperMock.h"
#include <gst/gst.h>
#include <gtest/gtest.h>
#include <memory>

using namespace firebolt::rialto::server;
using namespace firebolt::rialto::wrappers;

using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::SetArgPointee;
using ::testing::StrictMock;

namespace
{
// GSource is a public (partially-opaque) glib struct, but GMainContext/GMainLoop are fully opaque -
// their addresses are only ever compared, never dereferenced, so fake sentinel pointers are safe here.
GMainContext *const kMainContext = reinterpret_cast<GMainContext *>(0x1);
GMainLoop *const kMainLoop = reinterpret_cast<GMainLoop *>(0x2);
} // namespace

class GstDispatcherThreadTest : public ::testing::Test
{
protected:
    GstElement m_pipeline{};
    GstBus m_bus{};
    GSource m_busSource{};
    GstMessage m_message{};

    StrictMock<firebolt::rialto::server::GstDispatcherThreadClientMock> m_client;
    std::shared_ptr<StrictMock<GstWrapperMock>> m_gstWrapperMock{std::make_shared<StrictMock<GstWrapperMock>>()};
    std::shared_ptr<StrictMock<GlibWrapperMock>> m_glibWrapperMock{std::make_shared<StrictMock<GlibWrapperMock>>()};
    std::shared_ptr<StrictMock<FlushOnPrerollControllerMock>> m_flushOnPrerollControllerMock{
        std::make_shared<StrictMock<FlushOnPrerollControllerMock>>()};

    GSourceFunc m_capturedFunc{nullptr};
    gpointer m_capturedUserData{nullptr};

    // Sets up the expectations for construction (bus watch wiring + the loop thread) and returns the sut.
    std::unique_ptr<GstDispatcherThread> createSut()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstPipelineGetBus(GST_PIPELINE(&m_pipeline))).WillOnce(Return(&m_bus));
        EXPECT_CALL(*m_glibWrapperMock, gMainContextNew()).WillOnce(Return(kMainContext));
        EXPECT_CALL(*m_glibWrapperMock, gMainLoopNew(kMainContext, FALSE)).WillOnce(Return(kMainLoop));
        EXPECT_CALL(*m_gstWrapperMock, gstBusCreateWatch(&m_bus)).WillOnce(Return(&m_busSource));
        EXPECT_CALL(*m_glibWrapperMock, gSourceSetCallback(&m_busSource, _, _, nullptr))
            .WillOnce(Invoke(
                [this](GSource *, GSourceFunc func, gpointer data, GDestroyNotify)
                {
                    m_capturedFunc = func;
                    m_capturedUserData = data;
                }));
        EXPECT_CALL(*m_glibWrapperMock, gSourceAttach(&m_busSource, kMainContext)).WillOnce(Return(1));
        EXPECT_CALL(*m_glibWrapperMock, gSourceUnref(&m_busSource));
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(&m_bus));

        EXPECT_CALL(*m_glibWrapperMock, gMainContextPushThreadDefault(kMainContext));
        EXPECT_CALL(*m_glibWrapperMock, gMainLoopRun(kMainLoop));
        EXPECT_CALL(*m_glibWrapperMock, gMainContextPopThreadDefault(kMainContext));

        auto sut = std::make_unique<GstDispatcherThread>(m_client, &m_pipeline, m_flushOnPrerollControllerMock,
                                                          m_gstWrapperMock, m_glibWrapperMock);
        EXPECT_TRUE(m_capturedFunc);
        return sut;
    }

    // Invokes the captured GSourceFunc as GStreamer's bus-watch dispatch would, i.e. as a GstBusFunc.
    gboolean triggerBusMessage(GstMessage *message)
    {
        auto busFunc = reinterpret_cast<gboolean (*)(GstBus *, GstMessage *, gpointer)>(m_capturedFunc);
        return busFunc(&m_bus, message, m_capturedUserData);
    }

    // Expects the graceful-shutdown teardown (destructor) calls, common to every test.
    void expectDestruction()
    {
        EXPECT_CALL(*m_glibWrapperMock, gMainLoopUnref(kMainLoop));
        EXPECT_CALL(*m_glibWrapperMock, gMainContextUnref(kMainContext));
    }
};

/**
 * Test that construction wires up a GMainLoop-based bus watch and that destruction tears it down,
 * with no message ever having been received.
 */
TEST_F(GstDispatcherThreadTest, ConstructionAndDestructionWireUpAndTearDownTheMainLoop)
{
    auto sut = createSut();

    EXPECT_CALL(*m_glibWrapperMock, gMainLoopQuit(kMainLoop));
    expectDestruction();
    sut.reset();
}

/**
 * Test that a message of a type never requested from the bus is dropped without notifying the client.
 */
TEST_F(GstDispatcherThreadTest, UnfilteredMessageTypeIsIgnored)
{
    auto sut = createSut();

    GST_MESSAGE_SRC(&m_message) = GST_OBJECT(&m_pipeline);
    GST_MESSAGE_TYPE(&m_message) = GST_MESSAGE_BUFFERING;
    EXPECT_EQ(G_SOURCE_CONTINUE, triggerBusMessage(&m_message));

    EXPECT_CALL(*m_glibWrapperMock, gMainLoopQuit(kMainLoop));
    expectDestruction();
    sut.reset();
}

/**
 * Test that a GST_MESSAGE_STATE_CHANGED message (to GST_STATE_PAUSED) is handled correctly.
 */
TEST_F(GstDispatcherThreadTest, StateChangedToPaused)
{
    auto sut = createSut();

    GST_MESSAGE_SRC(&m_message) = GST_OBJECT(&m_pipeline);
    GST_MESSAGE_TYPE(&m_message) = GST_MESSAGE_STATE_CHANGED;

    GstState oldState = GST_STATE_READY;
    GstState newState = GST_STATE_PAUSED;
    GstState pending = GST_STATE_VOID_PENDING;

    EXPECT_CALL(*m_gstWrapperMock, gstMessageParseStateChanged(&m_message, _, _, _))
        .WillOnce(DoAll(SetArgPointee<1>(oldState), SetArgPointee<2>(newState), SetArgPointee<3>(pending)));
    EXPECT_CALL(*m_flushOnPrerollControllerMock, stateReached(GST_STATE_PAUSED));
    EXPECT_CALL(*m_gstWrapperMock, gstMessageRef(&m_message)).WillOnce(Return(&m_message));
    EXPECT_CALL(m_client, handleBusMessage(&m_message));

    EXPECT_EQ(G_SOURCE_CONTINUE, triggerBusMessage(&m_message));

    EXPECT_CALL(*m_glibWrapperMock, gMainLoopQuit(kMainLoop));
    expectDestruction();
    sut.reset();
}

/**
 * Test that a GST_MESSAGE_STATE_CHANGED message (to GST_STATE_PLAYING) is handled correctly.
 */
TEST_F(GstDispatcherThreadTest, StateChangedToPlaying)
{
    auto sut = createSut();

    GST_MESSAGE_SRC(&m_message) = GST_OBJECT(&m_pipeline);
    GST_MESSAGE_TYPE(&m_message) = GST_MESSAGE_STATE_CHANGED;

    GstState oldState = GST_STATE_PAUSED;
    GstState newState = GST_STATE_PLAYING;
    GstState pending = GST_STATE_VOID_PENDING;

    EXPECT_CALL(*m_gstWrapperMock, gstMessageParseStateChanged(&m_message, _, _, _))
        .WillOnce(DoAll(SetArgPointee<1>(oldState), SetArgPointee<2>(newState), SetArgPointee<3>(pending)));
    EXPECT_CALL(*m_flushOnPrerollControllerMock, stateReached(GST_STATE_PLAYING));
    EXPECT_CALL(*m_gstWrapperMock, gstMessageRef(&m_message)).WillOnce(Return(&m_message));
    EXPECT_CALL(m_client, handleBusMessage(&m_message));

    EXPECT_EQ(G_SOURCE_CONTINUE, triggerBusMessage(&m_message));

    EXPECT_CALL(*m_glibWrapperMock, gMainLoopQuit(kMainLoop));
    expectDestruction();
    sut.reset();
}

/**
 * Test that a GST_MESSAGE_STATE_CHANGED message (to GST_STATE_PAUSED, pending PAUSED) is handled correctly.
 */
TEST_F(GstDispatcherThreadTest, StateChangedToPrerolling)
{
    auto sut = createSut();

    GST_MESSAGE_SRC(&m_message) = GST_OBJECT(&m_pipeline);
    GST_MESSAGE_TYPE(&m_message) = GST_MESSAGE_STATE_CHANGED;

    GstState oldState = GST_STATE_READY;
    GstState newState = GST_STATE_PAUSED;
    GstState pending = GST_STATE_PAUSED;

    EXPECT_CALL(*m_gstWrapperMock, gstMessageParseStateChanged(&m_message, _, _, _))
        .WillOnce(DoAll(SetArgPointee<1>(oldState), SetArgPointee<2>(newState), SetArgPointee<3>(pending)));
    EXPECT_CALL(*m_flushOnPrerollControllerMock, setPrerolling());
    EXPECT_CALL(*m_gstWrapperMock, gstMessageRef(&m_message)).WillOnce(Return(&m_message));
    EXPECT_CALL(m_client, handleBusMessage(&m_message));

    EXPECT_EQ(G_SOURCE_CONTINUE, triggerBusMessage(&m_message));

    EXPECT_CALL(*m_glibWrapperMock, gMainLoopQuit(kMainLoop));
    expectDestruction();
    sut.reset();
}

/**
 * Test that a GST_MESSAGE_STATE_CHANGED message (to GST_STATE_NULL) stops the dispatcher.
 */
TEST_F(GstDispatcherThreadTest, StateChangedToStop)
{
    auto sut = createSut();

    GST_MESSAGE_SRC(&m_message) = GST_OBJECT(&m_pipeline);
    GST_MESSAGE_TYPE(&m_message) = GST_MESSAGE_STATE_CHANGED;

    GstState oldState = GST_STATE_PLAYING;
    GstState newState = GST_STATE_NULL;
    GstState pending = GST_STATE_VOID_PENDING;

    EXPECT_CALL(*m_gstWrapperMock, gstMessageParseStateChanged(&m_message, _, _, _))
        .WillOnce(DoAll(SetArgPointee<1>(oldState), SetArgPointee<2>(newState), SetArgPointee<3>(pending)));
    EXPECT_CALL(*m_flushOnPrerollControllerMock, reset());
    EXPECT_CALL(*m_gstWrapperMock, gstMessageRef(&m_message)).WillOnce(Return(&m_message));
    EXPECT_CALL(m_client, handleBusMessage(&m_message));
    // Reaching GST_STATE_NULL quits the loop immediately, from within the callback itself.
    EXPECT_CALL(*m_glibWrapperMock, gMainLoopQuit(kMainLoop));

    EXPECT_EQ(G_SOURCE_CONTINUE, triggerBusMessage(&m_message));

    // Destructor calls gMainLoopQuit again unconditionally; safe/idempotent on a real GMainLoop.
    EXPECT_CALL(*m_glibWrapperMock, gMainLoopQuit(kMainLoop));
    expectDestruction();
    sut.reset();
}

/**
 * Test that a GST_MESSAGE_ERROR message is handled correctly and stops the dispatcher.
 */
TEST_F(GstDispatcherThreadTest, Error)
{
    auto sut = createSut();

    GST_MESSAGE_SRC(&m_message) = GST_OBJECT(&m_pipeline);
    GST_MESSAGE_TYPE(&m_message) = GST_MESSAGE_ERROR;

    EXPECT_CALL(*m_gstWrapperMock, gstMessageRef(&m_message)).WillOnce(Return(&m_message));
    EXPECT_CALL(m_client, handleBusMessage(&m_message));
    EXPECT_CALL(*m_glibWrapperMock, gMainLoopQuit(kMainLoop));

    EXPECT_EQ(G_SOURCE_CONTINUE, triggerBusMessage(&m_message));

    EXPECT_CALL(*m_glibWrapperMock, gMainLoopQuit(kMainLoop));
    expectDestruction();
    sut.reset();
}

/**
 * Test that a GST_MESSAGE_STATE_CHANGED message is not handled for a non-pipeline object.
 */
TEST_F(GstDispatcherThreadTest, StateChangedToPausedNonPipeline)
{
    auto sut = createSut();

    GstElement someElement{};
    GST_MESSAGE_SRC(&m_message) = GST_OBJECT(&someElement);
    GST_MESSAGE_TYPE(&m_message) = GST_MESSAGE_STATE_CHANGED;

    EXPECT_EQ(G_SOURCE_CONTINUE, triggerBusMessage(&m_message));

    EXPECT_CALL(*m_glibWrapperMock, gMainLoopQuit(kMainLoop));
    expectDestruction();
    sut.reset();
}

/**
 * Test that a GST_MESSAGE_QOS message from a non-pipeline object is still forwarded to the client.
 */
TEST_F(GstDispatcherThreadTest, QosFromNonPipelineObjectIsForwarded)
{
    auto sut = createSut();

    GstElement someElement{};
    GST_MESSAGE_SRC(&m_message) = GST_OBJECT(&someElement);
    GST_MESSAGE_TYPE(&m_message) = GST_MESSAGE_QOS;

    EXPECT_CALL(*m_gstWrapperMock, gstMessageRef(&m_message)).WillOnce(Return(&m_message));
    EXPECT_CALL(m_client, handleBusMessage(&m_message));

    EXPECT_EQ(G_SOURCE_CONTINUE, triggerBusMessage(&m_message));

    EXPECT_CALL(*m_glibWrapperMock, gMainLoopQuit(kMainLoop));
    expectDestruction();
    sut.reset();
}

/**
 * Test that construction is aborted gracefully (no thread, no main loop) if the bus can't be obtained.
 */
TEST_F(GstDispatcherThreadTest, FailsGracefullyWhenBusUnavailable)
{
    EXPECT_CALL(*m_gstWrapperMock, gstPipelineGetBus(GST_PIPELINE(&m_pipeline))).WillOnce(Return(nullptr));

    auto sut = std::make_unique<GstDispatcherThread>(m_client, &m_pipeline, m_flushOnPrerollControllerMock,
                                                      m_gstWrapperMock, m_glibWrapperMock);
    // No gMainLoopQuit/Unref/ContextUnref expected: destructor must no-op when construction failed early.
    sut.reset();
}

