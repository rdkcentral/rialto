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
#include "ThunderWrapperFactoryMock.h"
#include <gst/gst.h>
#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>

using namespace firebolt::rialto::server;
using namespace firebolt::rialto::wrappers;

using ::testing::AtMost;
using ::testing::Return;
using ::testing::StrictMock;
using ::testing::Throw;

class GstTextTrackSinkTest : public ::testing::Test
{
protected:
    GstTextTrackSinkTest()
        : m_previousTextTrackPluginWrapperFactory{IFactoryAccessor::instance().textTrackPluginWrapperFactory()},
          m_previousThunderWrapperFactory{IFactoryAccessor::instance().thunderWrapperFactory()}
    {
        // TextTrack is not available in unit tests: make the sink start without a TextTrack session
        EXPECT_CALL(*m_textTrackPluginWrapperFactoryMock, getTextTrackPluginWrapper())
            .WillOnce(Throw(std::runtime_error("TextTrack not available")));
        EXPECT_CALL(*m_thunderWrapperFactoryMock, getThunderWrapper()).Times(AtMost(1)).WillRepeatedly(Return(nullptr));
        IFactoryAccessor::instance().textTrackPluginWrapperFactory() = m_textTrackPluginWrapperFactoryMock;
        IFactoryAccessor::instance().thunderWrapperFactory() = m_thunderWrapperFactoryMock;

        m_sink = GstTextTrackSinkFactory{}.createGstTextTrackSink();
        gst_object_ref_sink(m_sink);
    }

    ~GstTextTrackSinkTest() override
    {
        gst_element_set_state(m_sink, GST_STATE_NULL);
        gst_object_unref(m_sink);

        IFactoryAccessor::instance().textTrackPluginWrapperFactory() = m_previousTextTrackPluginWrapperFactory;
        IFactoryAccessor::instance().thunderWrapperFactory() = m_previousThunderWrapperFactory;
    }

    void setSync(gboolean sync) { g_object_set(m_sink, "sync", sync, nullptr); }

    gboolean getSync()
    {
        gboolean sync{FALSE};
        g_object_get(m_sink, "sync", &sync, nullptr);
        return sync;
    }

    std::shared_ptr<StrictMock<TextTrackPluginWrapperFactoryMock>> m_textTrackPluginWrapperFactoryMock{
        std::make_shared<StrictMock<TextTrackPluginWrapperFactoryMock>>()};
    std::shared_ptr<StrictMock<ThunderWrapperFactoryMock>> m_thunderWrapperFactoryMock{
        std::make_shared<StrictMock<ThunderWrapperFactoryMock>>()};
    std::shared_ptr<ITextTrackPluginWrapperFactory> m_previousTextTrackPluginWrapperFactory;
    std::shared_ptr<IThunderWrapperFactory> m_previousThunderWrapperFactory;
    GstElement *m_sink{nullptr};
};

TEST_F(GstTextTrackSinkTest, shouldDisableSyncWhenGoingFromReadyToPaused)
{
    ASSERT_NE(m_sink, nullptr);
    ASSERT_EQ(gst_element_set_state(m_sink, GST_STATE_READY), GST_STATE_CHANGE_SUCCESS);

    // playsink sets sync=TRUE on the text sink after NULL->READY, before activating the text chain
    setSync(TRUE);
    ASSERT_TRUE(getSync());

    EXPECT_EQ(gst_element_set_state(m_sink, GST_STATE_PAUSED), GST_STATE_CHANGE_SUCCESS);
    EXPECT_FALSE(getSync());
}

TEST_F(GstTextTrackSinkTest, shouldDisableSyncAgainAfterTextChainReactivation)
{
    ASSERT_NE(m_sink, nullptr);
    ASSERT_EQ(gst_element_set_state(m_sink, GST_STATE_PAUSED), GST_STATE_CHANGE_SUCCESS);
    EXPECT_FALSE(getSync());

    ASSERT_EQ(gst_element_set_state(m_sink, GST_STATE_READY), GST_STATE_CHANGE_SUCCESS);
    setSync(TRUE);

    EXPECT_EQ(gst_element_set_state(m_sink, GST_STATE_PAUSED), GST_STATE_CHANGE_SUCCESS);
    EXPECT_FALSE(getSync());
}
