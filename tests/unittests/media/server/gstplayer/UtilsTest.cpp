/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 Sky UK
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
#include "Utils.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::Invoke;
using testing::Return;
using testing::StrictMock;

class UtilsTest : public testing::Test
{
protected:
    StrictMock<firebolt::rialto::wrappers::GlibWrapperMock> m_glibWrapper;
    GstElement m_element{};
    guint m_signals[1]{123};
};

TEST_F(UtilsTest, shouldReturnFirstAudioFrameSignalName)
{
    EXPECT_CALL(m_glibWrapper, gObjectType(&m_element)).WillOnce(Return(G_TYPE_PARAM));
    EXPECT_CALL(m_glibWrapper, gSignalListIds(G_TYPE_PARAM, _))
        .WillOnce(Invoke([&](GType type, guint *nSignals)
                         {
                             *nSignals = 1;
                             return m_signals;
                         }));
    EXPECT_CALL(m_glibWrapper, gSignalQuery(m_signals[0], _))
        .WillOnce(Invoke([](guint signalId, GSignalQuery *query) { query->signal_name = "first-audio-frame"; }));
    EXPECT_CALL(m_glibWrapper, gFree(m_signals));

    auto signalName = firebolt::rialto::server::getFirstAudioFrameSignalName(m_glibWrapper, &m_element);

    ASSERT_TRUE(signalName.has_value());
    EXPECT_EQ(signalName.value(), "first-audio-frame");
}

TEST_F(UtilsTest, shouldReturnFirstAudioFrameCallbackSignalName)
{
    EXPECT_CALL(m_glibWrapper, gObjectType(&m_element)).WillOnce(Return(G_TYPE_PARAM));
    EXPECT_CALL(m_glibWrapper, gSignalListIds(G_TYPE_PARAM, _))
        .WillOnce(Invoke([&](GType type, guint *nSignals)
                         {
                             *nSignals = 1;
                             return m_signals;
                         }));
    EXPECT_CALL(m_glibWrapper, gSignalQuery(m_signals[0], _))
        .WillOnce(Invoke(
            [](guint signalId, GSignalQuery *query) { query->signal_name = "first-audio-frame-callback"; }));
    EXPECT_CALL(m_glibWrapper, gFree(m_signals));

    auto signalName = firebolt::rialto::server::getFirstAudioFrameSignalName(m_glibWrapper, &m_element);

    ASSERT_TRUE(signalName.has_value());
    EXPECT_EQ(signalName.value(), "first-audio-frame-callback");
}

TEST_F(UtilsTest, shouldReturnNulloptWhenFirstAudioFrameSignalMissing)
{
    EXPECT_CALL(m_glibWrapper, gObjectType(&m_element)).WillOnce(Return(G_TYPE_PARAM));
    EXPECT_CALL(m_glibWrapper, gSignalListIds(G_TYPE_PARAM, _))
        .WillOnce(Invoke([&](GType type, guint *nSignals)
                         {
                             *nSignals = 1;
                             return m_signals;
                         }));
    EXPECT_CALL(m_glibWrapper, gSignalQuery(m_signals[0], _))
        .WillOnce(Invoke([](guint signalId, GSignalQuery *query) { query->signal_name = "buffer-underflow-callback"; }));
    EXPECT_CALL(m_glibWrapper, gFree(m_signals));

    auto signalName = firebolt::rialto::server::getFirstAudioFrameSignalName(m_glibWrapper, &m_element);

    EXPECT_FALSE(signalName.has_value());
}
