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

#include "GstTextTrackSinkFactory.h"
#include "IFactoryAccessor.h"
#include "TextTrackPluginWrapperFactoryMock.h"
#include "TextTrackPluginWrapperMock.h"
#include "TextTrackWrapperMock.h"
#include "ThunderWrapperFactoryMock.h"
#include "ThunderWrapperMock.h"
#include <chrono>
#include <future>
#include <gst/base/gstbasesink.h>
#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <utility>

using namespace firebolt::rialto::server;
using namespace firebolt::rialto::wrappers;
using testing::_;
using testing::Invoke;
using testing::Return;
using testing::StrictMock;

namespace
{
constexpr std::uint32_t kSessionId{1};
constexpr std::uint32_t kNoError{0};
constexpr std::chrono::milliseconds kWaitTimeout{1000};
} // namespace

class GstTextTrackSinkTest : public testing::Test
{
protected:
    void SetUp() override
    {
        m_textTrackPluginWrapperFactory = std::make_shared<StrictMock<TextTrackPluginWrapperFactoryMock>>();
        m_textTrackPluginWrapper = std::make_shared<StrictMock<TextTrackPluginWrapperMock>>();
        m_textTrackWrapper = std::make_shared<StrictMock<TextTrackWrapperMock>>();
        m_thunderWrapperFactory = std::make_shared<StrictMock<ThunderWrapperFactoryMock>>();
        m_thunderWrapper = std::make_shared<StrictMock<ThunderWrapperMock>>();

        EXPECT_CALL(*m_textTrackPluginWrapperFactory, getTextTrackPluginWrapper())
            .WillRepeatedly(Return(m_textTrackPluginWrapper));
        EXPECT_CALL(*m_textTrackPluginWrapper, open()).WillRepeatedly(Return(kNoError));
        EXPECT_CALL(*m_textTrackPluginWrapper, isOperational()).WillRepeatedly(Return(true));
        EXPECT_CALL(*m_textTrackPluginWrapper, interface()).WillRepeatedly(Return(m_textTrackWrapper));
        EXPECT_CALL(*m_thunderWrapperFactory, getThunderWrapper()).WillRepeatedly(Return(m_thunderWrapper));
        EXPECT_CALL(*m_thunderWrapper, isSuccessful(kNoError)).WillRepeatedly(Return(true));
        EXPECT_CALL(*m_textTrackWrapper, openSession(_, _))
            .WillOnce(Invoke(
                [](const std::string &, std::uint32_t &sessionId)
                {
                    sessionId = kSessionId;
                    return kNoError;
                }));
        EXPECT_CALL(*m_textTrackWrapper, unmuteSession(kSessionId)).Times(2).WillRepeatedly(Return(kNoError));

        IFactoryAccessor::instance().textTrackPluginWrapperFactory() = m_textTrackPluginWrapperFactory;
        IFactoryAccessor::instance().thunderWrapperFactory() = m_thunderWrapperFactory;

        auto factory = GstTextTrackSinkFactory::createFactory();
        ASSERT_TRUE(factory);
        m_sink = factory->createGstTextTrackSink();
        ASSERT_NE(m_sink, nullptr);
        ASSERT_TRUE(GST_BASE_SINK_GET_CLASS(m_sink)->start(GST_BASE_SINK(m_sink)));
    }

    void TearDown() override
    {
        if (m_sink)
        {
            gst_object_unref(m_sink);
        }
        EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(m_textTrackWrapper.get()));
        EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(m_textTrackPluginWrapper.get()));
        EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(m_thunderWrapper.get()));
        EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(m_textTrackPluginWrapperFactory.get()));
        EXPECT_TRUE(testing::Mock::VerifyAndClearExpectations(m_thunderWrapperFactory.get()));
        IFactoryAccessor::instance().textTrackPluginWrapperFactory() = nullptr;
        IFactoryAccessor::instance().thunderWrapperFactory() = nullptr;
    }

    std::shared_ptr<StrictMock<TextTrackPluginWrapperFactoryMock>> m_textTrackPluginWrapperFactory;
    std::shared_ptr<StrictMock<TextTrackPluginWrapperMock>> m_textTrackPluginWrapper;
    std::shared_ptr<StrictMock<TextTrackWrapperMock>> m_textTrackWrapper;
    std::shared_ptr<StrictMock<ThunderWrapperFactoryMock>> m_thunderWrapperFactory;
    std::shared_ptr<StrictMock<ThunderWrapperMock>> m_thunderWrapper;
    GstElement *m_sink{nullptr};
};

TEST_F(GstTextTrackSinkTest, serializesDecoderUpdateWithClosedCaptionConfigurationAndClosesOutsideStateLock)
{
    std::uint8_t initialDecoder{};
    std::uint8_t updatedDecoder{};
    const std::string initialDecoderId = std::to_string(reinterpret_cast<std::uintptr_t>(&initialDecoder));
    const std::string updatedDecoderId = std::to_string(reinterpret_cast<std::uintptr_t>(&updatedDecoder));
    g_object_set(m_sink, "video-decoder", &initialDecoder, nullptr);

    std::promise<void> selectionStarted;
    std::promise<void> continueSelection;
    auto continueSelectionFuture = continueSelection.get_future().share();
    testing::Sequence captionConfiguration;
    EXPECT_CALL(*m_textTrackWrapper, setSessionClosedCaptionsService(kSessionId, "CC1"))
        .InSequence(captionConfiguration)
        .WillOnce(Invoke(
            [&](std::uint32_t, const std::string &)
            {
                selectionStarted.set_value();
                continueSelectionFuture.wait();
                return kNoError;
            }));
    EXPECT_CALL(*m_textTrackWrapper, associateVideoDecoder(kSessionId, initialDecoderId))
        .InSequence(captionConfiguration)
        .WillOnce(Return(kNoError));
    EXPECT_CALL(*m_textTrackWrapper, associateVideoDecoder(kSessionId, updatedDecoderId))
        .InSequence(captionConfiguration)
        .WillOnce(Return(kNoError));

    GstCaps *caps = gst_caps_from_string("subtitle/x-subtitle-cc");
    ASSERT_NE(caps, nullptr);
    gboolean capsResult{FALSE};
    std::thread capsThread{[&] { capsResult = GST_BASE_SINK_GET_CLASS(m_sink)->set_caps(GST_BASE_SINK(m_sink), caps); }};

    auto selectionStartedFuture = selectionStarted.get_future();
    const bool selectionWasStarted = selectionStartedFuture.wait_for(kWaitTimeout) == std::future_status::ready;

    std::thread decoderSetter;
    std::promise<void> decoderSetterDone;
    std::future<void> decoderSetterDoneFuture;
    if (selectionWasStarted)
    {
        decoderSetterDoneFuture = decoderSetterDone.get_future();
        decoderSetter = std::thread{[&]
                                    {
                                        g_object_set(m_sink, "video-decoder", &updatedDecoder, nullptr);
                                        decoderSetterDone.set_value();
                                    }};
        EXPECT_EQ(decoderSetterDoneFuture.wait_for(std::chrono::milliseconds{100}), std::future_status::timeout);
    }
    continueSelection.set_value();
    capsThread.join();
    if (decoderSetter.joinable())
    {
        decoderSetter.join();
    }
    gst_caps_unref(caps);
    ASSERT_TRUE(selectionWasStarted);
    ASSERT_TRUE(capsResult);

    std::promise<void> initialPositionStarted;
    std::promise<void> continueInitialPosition;
    auto continueInitialPositionFuture = continueInitialPosition.get_future().share();
    testing::Sequence positionUpdates;
    EXPECT_CALL(*m_textTrackWrapper, sendSessionTimestamp(kSessionId, 1000))
        .InSequence(positionUpdates)
        .WillOnce(Invoke(
            [&](std::uint32_t, std::uint64_t)
            {
                initialPositionStarted.set_value();
                continueInitialPositionFuture.wait();
                return kNoError;
            }));
    EXPECT_CALL(*m_textTrackWrapper, sendSessionTimestamp(kSessionId, 1020))
        .InSequence(positionUpdates)
        .WillOnce(Return(kNoError));

    std::promise<void> positionSetterDone;
    auto positionSetterDoneFuture = positionSetterDone.get_future();
    std::thread positionSetter{[&]
                               {
                                   g_object_set(m_sink, "position", static_cast<guint64>(1000 * GST_MSECOND), nullptr);
                                   positionSetterDone.set_value();
                               }};
    auto initialPositionStartedFuture = initialPositionStarted.get_future();
    const bool firstPositionStarted = initialPositionStartedFuture.wait_for(kWaitTimeout) == std::future_status::ready;

    std::thread offsetSetter;
    std::promise<void> offsetSetterStarted;
    std::promise<void> offsetSetterDone;
    std::future<void> offsetSetterDoneFuture;
    bool offsetSetterWasBlocked{false};
    if (firstPositionStarted)
    {
        offsetSetterDoneFuture = offsetSetterDone.get_future();
        offsetSetter = std::thread{[&]
                                   {
                                       offsetSetterStarted.set_value();
                                       g_object_set(m_sink, "offset", static_cast<guint64>(20 * GST_MSECOND), nullptr);
                                       offsetSetterDone.set_value();
                                   }};
        offsetSetterStarted.get_future().wait();
        offsetSetterWasBlocked = offsetSetterDoneFuture.wait_for(std::chrono::milliseconds{100}) ==
                                 std::future_status::timeout;
    }
    continueInitialPosition.set_value();
    positionSetter.join();
    if (offsetSetter.joinable())
    {
        offsetSetter.join();
    }
    EXPECT_TRUE(firstPositionStarted);
    EXPECT_TRUE(offsetSetterWasBlocked);
    EXPECT_EQ(positionSetterDoneFuture.wait_for(kWaitTimeout), std::future_status::ready);

    GstCaps *webVttCaps = gst_caps_from_string("text/vtt");
    ASSERT_NE(webVttCaps, nullptr);
    EXPECT_CALL(*m_textTrackWrapper, setSessionWebVTTSelection(kSessionId)).WillOnce(Return(kNoError));
    ASSERT_TRUE(GST_BASE_SINK_GET_CLASS(m_sink)->set_caps(GST_BASE_SINK(m_sink), webVttCaps));
    gst_caps_unref(webVttCaps);

    std::promise<void> sendStarted;
    std::promise<void> continueSend;
    auto continueSendFuture = continueSend.get_future().share();
    EXPECT_CALL(*m_textTrackWrapper, sendSessionData(_, _, _, _))
        .WillOnce(Invoke(
            [&](std::uint32_t, ITextTrackWrapper::DataType, std::int64_t, const std::string &)
            {
                sendStarted.set_value();
                continueSendFuture.wait();
                return kNoError;
            }));

    GstBuffer *buffer = gst_buffer_new_allocate(nullptr, 1, nullptr);
    ASSERT_NE(buffer, nullptr);
    GstFlowReturn renderResult{GST_FLOW_ERROR};
    std::thread renderThread{[&]
                             { renderResult = GST_BASE_SINK_GET_CLASS(m_sink)->render(GST_BASE_SINK(m_sink), buffer); }};
    auto sendStartedFuture = sendStarted.get_future();
    const bool sendWasStarted = sendStartedFuture.wait_for(kWaitTimeout) == std::future_status::ready;

    std::promise<void> closeStarted;
    std::promise<void> continueClose;
    auto continueCloseFuture = continueClose.get_future().share();
    EXPECT_CALL(*m_textTrackWrapper, closeSession(kSessionId))
        .WillOnce(Invoke(
            [&](std::uint32_t)
            {
                closeStarted.set_value();
                continueCloseFuture.wait();
                return kNoError;
            }));

    gboolean stopResult{FALSE};
    std::promise<void> stopDone;
    auto stopDoneFuture = stopDone.get_future();
    std::thread stopThread{[&]
                           {
                               stopResult = GST_BASE_SINK_GET_CLASS(m_sink)->stop(GST_BASE_SINK(m_sink));
                               stopDone.set_value();
                           }};
    const bool stopWasCompleted = stopDoneFuture.wait_for(kWaitTimeout) == std::future_status::ready;
    auto closeStartedFuture = closeStarted.get_future();
    const bool closeWasDeferred = closeStartedFuture.wait_for(std::chrono::milliseconds{100}) ==
                                  std::future_status::timeout;

    continueSend.set_value();
    const bool closeWasStarted = closeStartedFuture.wait_for(kWaitTimeout) == std::future_status::ready;

    std::promise<void> propertySetterDone;
    auto propertySetterDoneFuture = propertySetterDone.get_future();
    std::thread propertySetter{[&]
                               {
                                   g_object_set(m_sink, "mute", TRUE, nullptr);
                                   propertySetterDone.set_value();
                               }};
    EXPECT_EQ(propertySetterDoneFuture.wait_for(kWaitTimeout), std::future_status::ready);
    continueClose.set_value();
    propertySetter.join();
    renderThread.join();
    stopThread.join();
    gst_buffer_unref(buffer);

    EXPECT_TRUE(sendWasStarted);
    EXPECT_TRUE(stopWasCompleted);
    EXPECT_TRUE(stopResult);
    EXPECT_TRUE(closeWasDeferred);
    EXPECT_TRUE(closeWasStarted);
    EXPECT_EQ(renderResult, GST_FLOW_OK);
}
