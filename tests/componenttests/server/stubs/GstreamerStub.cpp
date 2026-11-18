/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2023 Sky UK
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

#include "GstreamerStub.h"
#include "Constants.h"
#include "Matchers.h"

using testing::_;
using testing::DoAll;
using testing::Invoke;
using testing::Return;
using testing::SaveArg;
using testing::SaveArgPointee;
using testing::StrEq;

namespace
{
constexpr int32_t kMessageTimeoutMs{200};
} // namespace

namespace firebolt::rialto::server::ct
{
GstreamerStub::GstreamerStub(const std::shared_ptr<testing::StrictMock<wrappers::GlibWrapperMock>> &glibWrapperMock,
                             const std::shared_ptr<testing::StrictMock<wrappers::GstWrapperMock>> &gstWrapperMock,
                             GstElement *pipeline, GstBus *bus, GstElement *rialtoSource)
    : m_glibWrapperMock{glibWrapperMock}, m_gstWrapperMock{gstWrapperMock}, m_pipeline{pipeline}, m_bus{bus},
      m_rialtoSource{rialtoSource}, m_message{nullptr},
      m_fakeMainContext{reinterpret_cast<GMainContext *>(&m_mainContextIdentity)},
      m_fakeMainLoop{reinterpret_cast<GMainLoop *>(&m_mainLoopIdentity)},
      m_fakeBusSource{reinterpret_cast<GSource *>(&m_busSourceIdentity)},
      m_fakeQuitSource{reinterpret_cast<GSource *>(&m_quitSourceIdentity)}
{
}

void GstreamerStub::setupPipeline()
{
    EXPECT_CALL(*m_glibWrapperMock, gSignalConnect(m_pipeline, StrEq("source-setup"), NotNullMatcher(), NotNullMatcher()))
        .WillOnce(Invoke(
            [this](gpointer instance, const gchar *detailed_signal, GCallback c_handler, gpointer data)
            {
                m_setupSourceFunc = c_handler;
                m_setupSourceUserData = data;
                return m_setupSourceSignalId;
            }));
    EXPECT_CALL(*m_glibWrapperMock,
                gSignalConnect(m_pipeline, StrEq("element-setup"), NotNullMatcher(), NotNullMatcher()))
        .WillOnce(Invoke(
            [this](gpointer instance, const gchar *detailed_signal, GCallback c_handler, gpointer data)
            {
                m_setupElementFunc = c_handler;
                m_setupElementUserData = data;
                return m_setupElementSignalId;
            }));
    EXPECT_CALL(*m_glibWrapperMock,
                gSignalConnect(m_pipeline, StrEq("deep-element-added"), NotNullMatcher(), NotNullMatcher()))
        .WillOnce(Invoke(
            [this](gpointer instance, const gchar *detailed_signal, GCallback c_handler, gpointer data)
            {
                m_deepElementAddedFunc = c_handler;
                m_deepElementAddedUserData = data;
                return m_deepElementAddedSignalId;
            }));
    setupMessages(false);
}

void GstreamerStub::setupMessages(bool repeatedCallsToGstPipelineGetBus)
{
    if (repeatedCallsToGstPipelineGetBus)
    {
        // Web audio calls this twice, once from each of these...
        //  GstDispatcherThread::gstBusEventHandler
        //  GstWebAudioPlayer::termWebAudioPipeline
        EXPECT_CALL(*m_gstWrapperMock, gstPipelineGetBus(GST_PIPELINE(m_pipeline))).WillRepeatedly(Return(m_bus));
    }
    else
    {
        EXPECT_CALL(*m_gstWrapperMock, gstPipelineGetBus(GST_PIPELINE(m_pipeline))).WillOnce(Return(m_bus));
    }
    // GstDispatcherThread construction: a private GMainContext/GMainLoop with a bus watch attached.
    EXPECT_CALL(*m_glibWrapperMock, gMainContextNew()).WillOnce(Return(m_fakeMainContext));
    EXPECT_CALL(*m_glibWrapperMock, gMainLoopNew(m_fakeMainContext, FALSE)).WillOnce(Return(m_fakeMainLoop));
    EXPECT_CALL(*m_gstWrapperMock, gstBusCreateWatch(m_bus)).WillOnce(Return(m_fakeBusSource));
    EXPECT_CALL(*m_glibWrapperMock, gSourceSetCallback(m_fakeBusSource, _, _, nullptr))
        .WillOnce(Invoke(
            [this](GSource *, GSourceFunc func, gpointer data, GDestroyNotify)
            {
                m_busWatchFunc = func;
                m_busWatchUserData = data;
            }));
    EXPECT_CALL(*m_glibWrapperMock, gSourceAttach(m_fakeBusSource, m_fakeMainContext)).WillOnce(Return(1));
    EXPECT_CALL(*m_glibWrapperMock, gSourceUnref(m_fakeBusSource));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(m_bus));

    // GstDispatcherThread::gstBusEventHandler, running on its own worker thread.
    EXPECT_CALL(*m_glibWrapperMock, gMainContextPushThreadDefault(m_fakeMainContext));
    EXPECT_CALL(*m_glibWrapperMock, gMainLoopRun(m_fakeMainLoop))
        .WillOnce(Invoke([this](GMainLoop *mainLoop) { runFakeMainLoop(); }));
    EXPECT_CALL(*m_glibWrapperMock, gMainContextPopThreadDefault(m_fakeMainContext));

    // Reached from both handleMessage() (on GST_STATE_NULL) and the destructor, so must tolerate repeats.
    EXPECT_CALL(*m_glibWrapperMock, gMainLoopQuit(m_fakeMainLoop))
        .Times(testing::AtLeast(1))
        .WillRepeatedly(Invoke(
            [this](GMainLoop *mainLoop)
            {
                std::unique_lock lock(m_mutex);
                m_mainLoopShouldQuit = true;
                m_cv.notify_one();
            }));
    EXPECT_CALL(*m_glibWrapperMock, gMainLoopUnref(m_fakeMainLoop));
    EXPECT_CALL(*m_glibWrapperMock, gMainContextUnref(m_fakeMainContext));

    // Destructor shutdown: the quit request is scheduled via an idle source rather than calling
    // gMainLoopQuit directly, so dispatch it as soon as it's attached (the fake loop has no real
    // "not started yet" window to simulate, so immediate dispatch is an equivalent fake behaviour).
    // Retires once matched: DualVideoPlaybackTest's two stubs share this mock, and whichever is
    // destroyed second must still reach its own (older) expectation rather than this one again.
    EXPECT_CALL(*m_glibWrapperMock, gIdleSourceNew()).WillOnce(Return(m_fakeQuitSource)).RetiresOnSaturation();
    EXPECT_CALL(*m_glibWrapperMock, gSourceSetCallback(m_fakeQuitSource, _, _, nullptr))
        .WillOnce(Invoke(
            [this](GSource *, GSourceFunc func, gpointer data, GDestroyNotify)
            {
                m_quitFunc = func;
                m_quitUserData = data;
            }));
    EXPECT_CALL(*m_glibWrapperMock, gSourceAttach(m_fakeQuitSource, m_fakeMainContext))
        .WillOnce(Invoke(
            [this](GSource *source, GMainContext *context)
            {
                m_quitFunc(m_quitUserData);
                return 1U;
            }));
    EXPECT_CALL(*m_glibWrapperMock, gSourceUnref(m_fakeQuitSource));

    // The bus watch unrefs the original message once the callback returns, so handleMessage takes its own ref,
    // which the HandleBusMessage task (generic and webAudio) releases once it has finished with the message.
    EXPECT_CALL(*m_gstWrapperMock, gstMessageRef(_))
        .WillRepeatedly(Invoke([](GstMessage *message) { return gst_message_ref(message); }));
    EXPECT_CALL(*m_gstWrapperMock, gstMessageUnref(_))
        .WillRepeatedly(Invoke([](GstMessage *message) { gst_message_unref(message); }));
}

void GstreamerStub::runFakeMainLoop()
{
    while (true)
    {
        std::unique_lock lock(m_mutex);
        m_cv.wait_for(lock, std::chrono::milliseconds(kMessageTimeoutMs),
                      [this]() { return m_message || m_mainLoopShouldQuit; });
        GstMessage *message{m_message};
        m_message = nullptr;
        bool shouldQuit{m_mainLoopShouldQuit};
        lock.unlock();

        if (message)
        {
            auto busFunc = reinterpret_cast<gboolean (*)(GstBus *, GstMessage *, gpointer)>(m_busWatchFunc);
            busFunc(m_bus, message, m_busWatchUserData);
            gst_message_unref(message); // mimics the real bus watch's auto-unref once the callback returns
        }
        if (shouldQuit)
        {
            return;
        }
    }
}

void GstreamerStub::setupRialtoSource()
{
    ASSERT_TRUE(m_setupSourceFunc);
    reinterpret_cast<void (*)(GstElement *, GstElement *, gpointer)>(m_setupSourceFunc)(m_pipeline, m_rialtoSource,
                                                                                        m_setupSourceUserData);
}

void GstreamerStub::setupAppSrcCallbacks(GstAppSrc *appSrc)
{
    GstAppSrcCallbacks &callbacks{m_appSrcCallbacks[appSrc]};
    gpointer &userData{m_appSrcCallbacksUserDatas[appSrc]};
    EXPECT_CALL(*m_gstWrapperMock, gstAppSrcSetCallbacks(appSrc, _, _, nullptr))
        .WillOnce(DoAll(SaveArgPointee<1>(&callbacks), SaveArg<2>(&userData)));
}

void GstreamerStub::setupElement(GstElement *element)
{
    ASSERT_TRUE(m_setupElementFunc);
    reinterpret_cast<void (*)(GstElement *, GstElement *, gpointer)>(m_setupElementFunc)(m_pipeline, element,
                                                                                         m_setupElementUserData);
}

void GstreamerStub::sendStateChanged(GstState oldState, GstState newState, GstState pendingState, bool handleParseCall)
{
    std::unique_lock lock(m_mutex);
    m_message = gst_message_new_state_changed(GST_OBJECT(m_pipeline), oldState, newState, pendingState);

    if (handleParseCall)
    {
        EXPECT_CALL(*m_gstWrapperMock, gstMessageParseStateChanged(m_message, _, _, _))
            .WillRepeatedly(Invoke(
                [oldState, newState, pendingState](GstMessage *message, GstState *p_oldstate, GstState *p_newstate,
                                                   GstState *p_pending)
                {
                    *p_oldstate = oldState;
                    *p_newstate = newState;
                    *p_pending = pendingState;
                }));
    }

    m_cv.notify_one();
}

void GstreamerStub::sendEos()
{
    std::unique_lock lock(m_mutex);
    m_message = gst_message_new_eos(GST_OBJECT(m_pipeline));
    m_cv.notify_one();
}

void GstreamerStub::sendQos(GstElement *src)
{
    constexpr bool kLive{true};
    constexpr std::uint64_t kRunningTime{2};
    constexpr std::uint64_t kStreamTime{1};
    std::unique_lock lock(m_mutex);
    m_message =
        gst_message_new_qos(GST_OBJECT(src), kLive, kRunningTime, kStreamTime, kQosInfo.processed, kQosInfo.dropped);
    m_cv.notify_one();
}

void GstreamerStub::sendWarning(GstElement *src, GError *error, const gchar *debug)
{
    std::unique_lock lock(m_mutex);
    m_message = gst_message_new_warning(GST_OBJECT(src), error, debug);
    m_cv.notify_one();
}

} // namespace firebolt::rialto::server::ct
