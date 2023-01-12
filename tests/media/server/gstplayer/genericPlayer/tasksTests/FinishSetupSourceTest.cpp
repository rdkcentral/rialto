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

#include "tasks/generic/FinishSetupSource.h"
#include "DecryptionServiceMock.h"
#include "GenericPlayerContext.h"
#include "GstGenericPlayerClientMock.h"
#include "GstGenericPlayerPrivateMock.h"
#include "GstSrcMock.h"
#include <gst/gst.h>
#include <gtest/gtest.h>

using testing::_;
using testing::Invoke;
using testing::Return;
using testing::StrictMock;

class FinishSetupSourceTest : public testing::Test
{
protected:
    firebolt::rialto::server::GenericPlayerContext m_context;
    StrictMock<firebolt::rialto::server::GstGenericPlayerPrivateMock> m_gstPlayer;
    StrictMock<firebolt::rialto::server::GstGenericPlayerClientMock> m_gstPlayerClient;
    std::shared_ptr<StrictMock<firebolt::rialto::server::DecryptionServiceMock>> m_decryptionServiceMock{
        std::make_shared<StrictMock<firebolt::rialto::server::DecryptionServiceMock>>()};
    std::shared_ptr<StrictMock<firebolt::rialto::server::GstSrcMock>> m_gstSrc{
        std::make_shared<StrictMock<firebolt::rialto::server::GstSrcMock>>()};
    GstElement m_source{};
    GstAppSrc m_audioAppSrc{};
    GstAppSrc m_videoAppSrc{};
    const guint m_kDataLength{7};
    const guint64 m_kOffset{123};
    GstAppSrcCallbacks m_audioCallbacks{};
    gpointer m_audioUserData{};
    GstAppSrcCallbacks m_videoCallbacks{};
    gpointer m_videoUserData{};

    FinishSetupSourceTest()
    {
        m_context.gstSrc = m_gstSrc;
        m_context.source = &m_source;
        m_context.decryptionService = m_decryptionServiceMock.get();
        m_context.streamInfo.emplace(firebolt::rialto::MediaSourceType::AUDIO, GST_ELEMENT(&m_audioAppSrc));
        m_context.streamInfo.emplace(firebolt::rialto::MediaSourceType::VIDEO, GST_ELEMENT(&m_videoAppSrc));
    }

    void expectFinishSetupSource()
    {
        EXPECT_CALL(*m_gstSrc, setupAndAddAppArc(std::dynamic_pointer_cast<firebolt::rialto::server::IDecryptionService>(
                                                     m_decryptionServiceMock)
                                                     .get(),
                                                 &m_source, GST_ELEMENT(&m_audioAppSrc), _, &m_gstPlayer,
                                                 firebolt::rialto::MediaSourceType::AUDIO))
            .WillOnce(Invoke(
                [this](firebolt::rialto::server::IDecryptionService *decryptionService, GstElement *element,
                       GstElement *appsrc, GstAppSrcCallbacks *callbacks, gpointer userData,
                       firebolt::rialto::MediaSourceType type)
                {
                    m_audioCallbacks = *callbacks;
                    m_audioUserData = userData;
                }));
        EXPECT_CALL(m_gstPlayer, notifyNeedMediaData(true, false));
        EXPECT_CALL(*m_gstSrc, setupAndAddAppArc(std::dynamic_pointer_cast<firebolt::rialto::server::IDecryptionService>(
                                                     m_decryptionServiceMock)
                                                     .get(),
                                                 &m_source, GST_ELEMENT(&m_videoAppSrc), _, &m_gstPlayer,
                                                 firebolt::rialto::MediaSourceType::VIDEO))
            .WillOnce(Invoke(
                [this](firebolt::rialto::server::IDecryptionService *decryptionService, GstElement *element,
                       GstElement *appsrc, GstAppSrcCallbacks *callbacks, gpointer userData,
                       firebolt::rialto::MediaSourceType type)
                {
                    m_videoCallbacks = *callbacks;
                    m_videoUserData = userData;
                }));
        EXPECT_CALL(m_gstPlayer, notifyNeedMediaData(false, true));
        EXPECT_CALL(*m_gstSrc, allAppSrcsAdded(&m_source));
        EXPECT_CALL(m_gstPlayerClient, notifyPlaybackState(firebolt::rialto::PlaybackState::IDLE));
    }
};

TEST_F(FinishSetupSourceTest, shouldFinishSetupSource)
{
    firebolt::rialto::server::FinishSetupSource task{m_context, m_gstPlayer, &m_gstPlayerClient};
    expectFinishSetupSource();
    task.execute();
}

TEST_F(FinishSetupSourceTest, shouldScheduleAudioNeedData)
{
    firebolt::rialto::server::FinishSetupSource task{m_context, m_gstPlayer, &m_gstPlayerClient};
    expectFinishSetupSource();
    task.execute();
    EXPECT_CALL(m_gstPlayer, scheduleNeedMediaData(&m_audioAppSrc));
    EXPECT_TRUE(m_audioCallbacks.need_data);
    EXPECT_TRUE(m_audioUserData);
    ((void (*)(GstAppSrc *, guint, gpointer))m_audioCallbacks.need_data)(&m_audioAppSrc, m_kDataLength, m_audioUserData);
}

TEST_F(FinishSetupSourceTest, shouldScheduleVideoNeedData)
{
    firebolt::rialto::server::FinishSetupSource task{m_context, m_gstPlayer, &m_gstPlayerClient};
    expectFinishSetupSource();
    task.execute();
    EXPECT_CALL(m_gstPlayer, scheduleNeedMediaData(&m_videoAppSrc));
    EXPECT_TRUE(m_videoCallbacks.need_data);
    EXPECT_TRUE(m_videoUserData);
    ((void (*)(GstAppSrc *, guint, gpointer))m_videoCallbacks.need_data)(&m_videoAppSrc, m_kDataLength, m_videoUserData);
}

TEST_F(FinishSetupSourceTest, shouldScheduleAudioEnoughData)
{
    firebolt::rialto::server::FinishSetupSource task{m_context, m_gstPlayer, &m_gstPlayerClient};
    expectFinishSetupSource();
    task.execute();
    EXPECT_CALL(m_gstPlayer, scheduleEnoughData(&m_audioAppSrc));
    EXPECT_TRUE(m_audioCallbacks.enough_data);
    EXPECT_TRUE(m_audioUserData);
    ((void (*)(GstAppSrc *, gpointer))m_audioCallbacks.enough_data)(&m_audioAppSrc, m_audioUserData);
}

TEST_F(FinishSetupSourceTest, shouldScheduleVideoEnoughData)
{
    firebolt::rialto::server::FinishSetupSource task{m_context, m_gstPlayer, &m_gstPlayerClient};
    expectFinishSetupSource();
    task.execute();
    EXPECT_CALL(m_gstPlayer, scheduleEnoughData(&m_videoAppSrc));
    EXPECT_TRUE(m_videoCallbacks.enough_data);
    EXPECT_TRUE(m_videoUserData);
    ((void (*)(GstAppSrc *, gpointer))m_videoCallbacks.enough_data)(&m_videoAppSrc, m_videoUserData);
}

TEST_F(FinishSetupSourceTest, shouldScheduleAudioSeekData)
{
    firebolt::rialto::server::FinishSetupSource task{m_context, m_gstPlayer, &m_gstPlayerClient};
    expectFinishSetupSource();
    task.execute();
    EXPECT_CALL(m_gstPlayer, scheduleEnoughData(&m_audioAppSrc));
    EXPECT_TRUE(m_audioCallbacks.seek_data);
    EXPECT_TRUE(m_audioUserData);
    ((gboolean(*)(GstAppSrc *, guint64, gpointer))m_audioCallbacks.seek_data)(&m_audioAppSrc, m_kOffset, m_audioUserData);
}

TEST_F(FinishSetupSourceTest, shouldScheduleVideoSeekData)
{
    firebolt::rialto::server::FinishSetupSource task{m_context, m_gstPlayer, &m_gstPlayerClient};
    expectFinishSetupSource();
    task.execute();
    EXPECT_CALL(m_gstPlayer, scheduleEnoughData(&m_videoAppSrc));
    EXPECT_TRUE(m_videoCallbacks.seek_data);
    EXPECT_TRUE(m_videoUserData);
    ((gboolean(*)(GstAppSrc *, guint64, gpointer))m_videoCallbacks.seek_data)(&m_videoAppSrc, m_kOffset, m_videoUserData);
}
