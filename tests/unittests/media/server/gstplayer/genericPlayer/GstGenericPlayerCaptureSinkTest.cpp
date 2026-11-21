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

#include "GstGenericPlayerTestCommon.h"
#include "IGstGenericPlayerPrivate.h"

#include <chrono>

using testing::_;
using testing::DoAll;
using testing::InSequence;
using testing::Invoke;
using testing::Return;
using testing::SetArgPointee;
using testing::StrEq;

namespace
{
constexpr std::chrono::milliseconds kReadyTimeout{5000};
const std::string kResolution{"1920x1080"};
} // namespace

/**
 * The capture sink set-up of the generic player (R7, design D2 to D5), with mocked gst and glib wrappers.
 */
class GstGenericPlayerCaptureSinkTest : public GstGenericPlayerTestCommon
{
protected:
    GstGenericPlayerCaptureSinkTest()
    {
        gstPlayerWillBeCreated();
        m_sut = std::make_unique<GstGenericPlayer>(&m_gstPlayerClient, m_decryptionServiceMock, MediaType::MSE,
                                                   m_videoReq, false, m_gstWrapperMock, m_glibWrapperMock,
                                                   m_rdkGstreamerUtilsWrapperMock, m_gstInitialiserMock,
                                                   std::move(m_flushWatcher), m_gstSrcFactoryMock,
                                                   m_gstProfilerFactoryMock, m_timerFactoryMock, std::move(m_taskFactory),
                                                   std::move(workerThreadFactory), std::move(gstDispatcherThreadFactory),
                                                   m_gstProtectionMetadataFactoryMock);
        m_sink = fakeElement();
        m_codecCaps = gst_caps_new_empty_simple("video/x-h264");
    }

    ~GstGenericPlayerCaptureSinkTest() override
    {
        gstPlayerWillBeDestroyed();
        m_sut.reset(); // first: the player may still refer to the sink
        gst_caps_unref(m_codecCaps);
        gst_object_unref(m_sink);
    }

    static GstElement *fakeElement()
    {
        GstElementFactory *elementFactory = gst_element_factory_find("fakesrc");
        GstElement *element = gst_element_factory_create(elementFactory, nullptr);
        gst_object_unref(elementFactory);
        return element;
    }

    void westerossinkWillBeCreated()
    {
        EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryFind(StrEq("westerossink"))).WillOnce(Return(m_factory));
        EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryCreate(m_factory, nullptr)).WillOnce(Return(m_sink));
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(GST_OBJECT(m_factory)));
    }

    void sinkHasCaptureProperties()
    {
        for (const char *kName : {"rdk-capture-session", "codec", "resolution"})
            EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, StrEq(kName))).WillOnce(Return(&m_property));
    }

    bool configure() { return m_sut->configureCaptureSink(m_session, m_codecCaps, kResolution, kReadyTimeout); }

    std::unique_ptr<IGstGenericPlayer> m_sut;
    VideoRequirements m_videoReq{kMinPrimaryVideoWidth, kMinPrimaryVideoHeight};
    char m_dummy{0};
    GstElementFactory *m_factory{reinterpret_cast<GstElementFactory *>(&m_dummy)};
    GstElement *m_sink{nullptr};
    GstCaps *m_codecCaps{nullptr};
    GParamSpec m_property{};
    void *m_session{reinterpret_cast<void *>(0x5E55)};
};

TEST_F(GstGenericPlayerCaptureSinkTest, ShouldConfigureEverythingBeforeTheSinkLeavesNull)
{
    executeTaskWhenEnqueued();
    {
        InSequence sequence;
        westerossinkWillBeCreated();
        sinkHasCaptureProperties();
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetPointerStub(m_sink, StrEq("rdk-capture-session"), m_session));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetPointerStub(m_sink, StrEq("codec"), m_codecCaps));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStrStub(m_sink, StrEq("resolution"), StrEq(kResolution)));
        EXPECT_CALL(*m_gstWrapperMock, gstObjectRef(m_sink));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_pipeline, StrEq("video-sink")));
        EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(m_sink, GST_STATE_READY)).WillOnce(Return(GST_STATE_CHANGE_ASYNC));
        EXPECT_CALL(*m_gstWrapperMock, gstElementGetState(m_sink, _, _, 5000 * GST_MSECOND))
            .WillOnce(DoAll(SetArgPointee<1>(GST_STATE_READY), Return(GST_STATE_CHANGE_SUCCESS)));
    }
    EXPECT_TRUE(configure());
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(GST_OBJECT(m_sink))); // the player's reference, at destruction
}

class GstGenericPlayerCaptureSinkRollbackTest : public GstGenericPlayerCaptureSinkTest
{
protected:
    void sinkWillBeConfiguredAndAssigned()
    {
        westerossinkWillBeCreated();
        sinkHasCaptureProperties();
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetPointerStub(m_sink, _, _)).Times(2);
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStrStub(m_sink, StrEq("resolution"), _));
        EXPECT_CALL(*m_gstWrapperMock, gstObjectRef(m_sink));
    }

    // Rollback: NULL first, then video-sink cleared, then the player's reference dropped.
    void sinkWillBeDiscarded()
    {
        InSequence sequence;
        EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(m_sink, GST_STATE_NULL)).WillOnce(Return(GST_STATE_CHANGE_SUCCESS));
        EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_pipeline, StrEq("video-sink")));
        EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(GST_OBJECT(m_sink)));
    }
};

TEST_F(GstGenericPlayerCaptureSinkRollbackTest, FailedReadyDiscardsTheSink)
{
    executeTaskWhenEnqueued();
    sinkWillBeConfiguredAndAssigned();
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_pipeline, StrEq("video-sink"))).Times(2); // assign, clear
    EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(m_sink, GST_STATE_READY)).WillOnce(Return(GST_STATE_CHANGE_FAILURE));
    EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(m_sink, GST_STATE_NULL)).WillOnce(Return(GST_STATE_CHANGE_SUCCESS));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(GST_OBJECT(m_sink)));
    EXPECT_FALSE(configure());

    m_sut->releaseCaptureSink(); // nothing left to release
}

TEST_F(GstGenericPlayerCaptureSinkRollbackTest, ReadyTimeoutDiscardsTheSink)
{
    executeTaskWhenEnqueued();
    sinkWillBeConfiguredAndAssigned();
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_pipeline, StrEq("video-sink"))).Times(2);
    EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(m_sink, GST_STATE_READY)).WillOnce(Return(GST_STATE_CHANGE_ASYNC));
    EXPECT_CALL(*m_gstWrapperMock, gstElementGetState(m_sink, _, _, _)).WillOnce(Return(GST_STATE_CHANGE_ASYNC));
    EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(m_sink, GST_STATE_NULL)).WillOnce(Return(GST_STATE_CHANGE_SUCCESS));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(GST_OBJECT(m_sink)));
    EXPECT_FALSE(configure());
}

TEST_F(GstGenericPlayerCaptureSinkRollbackTest, MissingPropertyLeavesNoSinkAndNoVideoSink)
{
    executeTaskWhenEnqueued();
    westerossinkWillBeCreated();
    EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, StrEq("rdk-capture-session"))).WillOnce(Return(&m_property));
    EXPECT_CALL(*m_glibWrapperMock, gObjectClassFindProperty(_, StrEq("codec"))).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(GST_OBJECT(m_sink)));
    // no video-sink assignment and no state change: the strict mocks would flag them
    EXPECT_FALSE(configure());
}

TEST_F(GstGenericPlayerCaptureSinkRollbackTest, MissingWesterossinkFails)
{
    executeTaskWhenEnqueued();
    EXPECT_CALL(*m_gstWrapperMock, gstElementFactoryFind(StrEq("westerossink"))).WillOnce(Return(nullptr));
    EXPECT_FALSE(configure());
}

TEST_F(GstGenericPlayerCaptureSinkRollbackTest, ReleaseAfterSuccessDiscardsTheSink)
{
    executeTaskWhenEnqueued();
    sinkWillBeConfiguredAndAssigned();
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_pipeline, StrEq("video-sink"))).RetiresOnSaturation();
    EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(m_sink, GST_STATE_READY)).WillOnce(Return(GST_STATE_CHANGE_SUCCESS));
    EXPECT_CALL(*m_gstWrapperMock, gstElementGetState(m_sink, _, _, _))
        .WillOnce(DoAll(SetArgPointee<1>(GST_STATE_READY), Return(GST_STATE_CHANGE_SUCCESS)));
    ASSERT_TRUE(configure());

    sinkWillBeDiscarded(); // as after an admission failure
    m_sut->releaseCaptureSink();
}

TEST_F(GstGenericPlayerCaptureSinkRollbackTest, DestroyingThePlayerDropsTheCaptureSinkReference)
{
    executeTaskWhenEnqueued();
    sinkWillBeConfiguredAndAssigned();
    EXPECT_CALL(*m_glibWrapperMock, gObjectSetStub(&m_pipeline, StrEq("video-sink")));
    EXPECT_CALL(*m_gstWrapperMock, gstElementSetState(m_sink, GST_STATE_READY)).WillOnce(Return(GST_STATE_CHANGE_SUCCESS));
    EXPECT_CALL(*m_gstWrapperMock, gstElementGetState(m_sink, _, _, _))
        .WillOnce(DoAll(SetArgPointee<1>(GST_STATE_READY), Return(GST_STATE_CHANGE_SUCCESS)));
    ASSERT_TRUE(configure());

    EXPECT_CALL(*m_gstWrapperMock, gstObjectUnref(GST_OBJECT(m_sink))); // with the pipeline, in the destructor
}
