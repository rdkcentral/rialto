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

#include "GlibWrapperMock.h"
#include "GstTextTrackSinkFactory.h"
#include "IFactoryAccessor.h"
#include "SetSubtitleOffset.h"
#include "TextTrackPluginWrapperFactoryMock.h"
#include "TextTrackPluginWrapperMock.h"
#include "TextTrackWrapperMock.h"
#include "ThunderWrapperFactoryMock.h"
#include "ThunderWrapperMock.h"

#include <cstdarg>
#include <gmock/gmock.h>
#include <gst/base/gstbasesink.h>
#include <gtest/gtest.h>
#include <memory>

using firebolt::rialto::server::GenericPlayerContext;
using firebolt::rialto::server::GstTextTrackSinkFactory;
using firebolt::rialto::server::tasks::generic::SetSubtitleOffset;
using firebolt::rialto::wrappers::GlibWrapperMock;
using firebolt::rialto::wrappers::IFactoryAccessor;
using firebolt::rialto::wrappers::TextTrackPluginWrapperFactoryMock;
using firebolt::rialto::wrappers::TextTrackPluginWrapperMock;
using firebolt::rialto::wrappers::TextTrackWrapperMock;
using firebolt::rialto::wrappers::ThunderWrapperFactoryMock;
using firebolt::rialto::wrappers::ThunderWrapperMock;
using testing::_;
using testing::DoAll;
using testing::Return;
using testing::SetArgReferee;
using testing::StrEq;
using testing::StrictMock;

namespace
{
constexpr uint32_t kSuccess{0};
constexpr uint32_t kSessionId{1};
constexpr const char *kDisplay{"westeros-asplayer-subtitles"};
constexpr uint64_t kPosition{2 * GST_SECOND};

struct SinkTestMocks
{
    std::shared_ptr<StrictMock<TextTrackPluginWrapperFactoryMock>> pluginFactory{
        std::make_shared<StrictMock<TextTrackPluginWrapperFactoryMock>>()};
    std::shared_ptr<StrictMock<ThunderWrapperFactoryMock>> thunderFactory{
        std::make_shared<StrictMock<ThunderWrapperFactoryMock>>()};
    std::shared_ptr<StrictMock<TextTrackPluginWrapperMock>> plugin{
        std::make_shared<StrictMock<TextTrackPluginWrapperMock>>()};
    std::shared_ptr<StrictMock<TextTrackWrapperMock>> textTrack{std::make_shared<StrictMock<TextTrackWrapperMock>>()};
    std::shared_ptr<StrictMock<ThunderWrapperMock>> thunder{std::make_shared<StrictMock<ThunderWrapperMock>>()};
    bool accessorCreated{false};
};

SinkTestMocks &sinkTestMocks()
{
    static SinkTestMocks mocks;
    return mocks;
}
} // namespace

class SinkGlibWrapperMock : public StrictMock<GlibWrapperMock>
{
public:
    void gObjectSet(gpointer object, const gchar *firstPropertyName, ...) override
    {
        va_list args;
        va_start(args, firstPropertyName);
        const gint64 value = va_arg(args, gint64);
        va_end(args);

        GlibWrapperMock::gObjectSet(object, firstPropertyName, value, nullptr);
        g_object_set(object, firstPropertyName, value, nullptr);
    }
};

class GstTextTrackSinkOffsetTest : public testing::Test
{
protected:
    GstTextTrackSinkOffsetTest()
        : m_mocks{sinkTestMocks()}, m_pluginFactory{m_mocks.pluginFactory}, m_thunderFactory{m_mocks.thunderFactory},
          m_plugin{m_mocks.plugin}, m_textTrack{m_mocks.textTrack}, m_thunder{m_mocks.thunder}
    {
        IFactoryAccessor::instance().textTrackPluginWrapperFactory() = m_pluginFactory;
        IFactoryAccessor::instance().thunderWrapperFactory() = m_thunderFactory;
    }

    void TearDown() override
    {
        if (m_sink)
        {
            if (m_sinkStarted)
            {
                GST_BASE_SINK_GET_CLASS(m_sink)->stop(GST_BASE_SINK(m_sink));
            }
            gst_object_unref(m_sink);
        }
        if (m_caps)
        {
            gst_caps_unref(m_caps);
        }
        testing::Mock::VerifyAndClearExpectations(m_pluginFactory.get());
        testing::Mock::VerifyAndClearExpectations(m_thunderFactory.get());
        testing::Mock::VerifyAndClearExpectations(m_plugin.get());
        testing::Mock::VerifyAndClearExpectations(m_textTrack.get());
        testing::Mock::VerifyAndClearExpectations(m_thunder.get());
        IFactoryAccessor::instance().textTrackPluginWrapperFactory() = nullptr;
        IFactoryAccessor::instance().thunderWrapperFactory() = nullptr;
    }

    void expectSessionCreation()
    {
        const bool createsAccessor = !m_mocks.accessorCreated;
        if (createsAccessor)
        {
            EXPECT_CALL(*m_pluginFactory, getTextTrackPluginWrapper()).WillOnce(Return(m_plugin));
            EXPECT_CALL(*m_thunderFactory, getThunderWrapper()).WillOnce(Return(m_thunder));
            EXPECT_CALL(*m_plugin, open()).WillOnce(Return(kSuccess));
            EXPECT_CALL(*m_plugin, isOperational()).WillOnce(Return(true));
            EXPECT_CALL(*m_plugin, interface()).WillOnce(Return(m_textTrack));
            m_mocks.accessorCreated = true;
        }

        EXPECT_CALL(*m_thunder, isSuccessful(kSuccess)).Times(createsAccessor ? 8 : 7).WillRepeatedly(Return(true));
        EXPECT_CALL(*m_textTrack, openSession(kDisplay, _)).WillOnce(DoAll(SetArgReferee<1>(kSessionId), Return(kSuccess)));
    }

    SinkTestMocks &m_mocks;
    std::shared_ptr<StrictMock<TextTrackPluginWrapperFactoryMock>> m_pluginFactory;
    std::shared_ptr<StrictMock<ThunderWrapperFactoryMock>> m_thunderFactory;
    std::shared_ptr<StrictMock<TextTrackPluginWrapperMock>> m_plugin;
    std::shared_ptr<StrictMock<TextTrackWrapperMock>> m_textTrack;
    std::shared_ptr<StrictMock<ThunderWrapperMock>> m_thunder;
    GstElement *m_sink{nullptr};
    GstCaps *m_caps{nullptr};
    bool m_sinkStarted{false};
};

TEST_F(GstTextTrackSinkOffsetTest, AppliesTaskOffsetsThroughSignedPropertyToSession)
{
    expectSessionCreation();
    EXPECT_CALL(*m_textTrack, setSessionWebVTTSelection(kSessionId)).WillOnce(Return(kSuccess));
    EXPECT_CALL(*m_textTrack, unmuteSession(kSessionId)).WillOnce(Return(kSuccess));
    EXPECT_CALL(*m_textTrack, sendSessionTimestamp(kSessionId, 1000)).WillOnce(Return(kSuccess));
    EXPECT_CALL(*m_textTrack, sendSessionTimestamp(kSessionId, 2000)).WillOnce(Return(kSuccess));
    EXPECT_CALL(*m_textTrack, sendSessionTimestamp(kSessionId, 3000)).WillOnce(Return(kSuccess));
    EXPECT_CALL(*m_textTrack, closeSession(kSessionId)).WillOnce(Return(kSuccess));

    GstTextTrackSinkFactory sinkFactory;
    m_sink = sinkFactory.createGstTextTrackSink();
    ASSERT_NE(nullptr, m_sink);
    GstBaseSinkClass *sinkClass = GST_BASE_SINK_GET_CLASS(m_sink);
    ASSERT_TRUE(sinkClass->start(GST_BASE_SINK(m_sink)));
    m_sinkStarted = true;

    g_object_set(G_OBJECT(m_sink), "position", kPosition, nullptr);
    GenericPlayerContext context;
    context.subtitleSink = m_sink;
    auto glibWrapper = std::make_shared<SinkGlibWrapperMock>();
    EXPECT_CALL(*glibWrapper, gObjectSetInt64Stub(m_sink, StrEq("offset"), -static_cast<int64_t>(GST_SECOND))).Times(1);
    EXPECT_CALL(*glibWrapper, gObjectSetInt64Stub(m_sink, StrEq("offset"), 0)).Times(1);
    EXPECT_CALL(*glibWrapper, gObjectSetInt64Stub(m_sink, StrEq("offset"), static_cast<int64_t>(GST_SECOND))).Times(1);

    SetSubtitleOffset negativeOffset{context, glibWrapper, -static_cast<int64_t>(GST_SECOND)};
    negativeOffset.execute();

    m_caps = gst_caps_from_string("text/vtt");
    ASSERT_NE(nullptr, m_caps);
    ASSERT_TRUE(sinkClass->set_caps(GST_BASE_SINK(m_sink), m_caps));

    SetSubtitleOffset zeroOffset{context, glibWrapper, 0};
    zeroOffset.execute();
    SetSubtitleOffset positiveOffset{context, glibWrapper, static_cast<int64_t>(GST_SECOND)};
    positiveOffset.execute();
}
