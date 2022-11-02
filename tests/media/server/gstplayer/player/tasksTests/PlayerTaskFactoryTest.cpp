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

#include "tasks/PlayerTaskFactory.h"
#include "GlibWrapperMock.h"
#include "GstPlayerClientMock.h"
#include "GstPlayerPrivateMock.h"
#include "GstWrapperMock.h"
#include "PlayerContext.h"
#include "tasks/AttachSamples.h"
#include "tasks/AttachSource.h"
#include "tasks/CheckAudioUnderflow.h"
#include "tasks/EnoughData.h"
#include "tasks/Eos.h"
#include "tasks/FinishSetupSource.h"
#include "tasks/HandleBusMessage.h"
#include "tasks/IPlayerTask.h"
#include "tasks/NeedData.h"
#include "tasks/Pause.h"
#include "tasks/Play.h"
#include "tasks/ReadShmDataAndAttachSamples.h"
#include "tasks/ReportPosition.h"
#include "tasks/SetPlaybackRate.h"
#include "tasks/SetPosition.h"
#include "tasks/SetVideoGeometry.h"
#include "tasks/SetupElement.h"
#include "tasks/SetupSource.h"
#include "tasks/Shutdown.h"
#include "tasks/Stop.h"
#include "tasks/Underflow.h"
#include <gtest/gtest.h>

using testing::_;
using testing::Return;
using testing::StrictMock;

struct PlayerTaskFactoryTest : public testing::Test
{
    firebolt::rialto::server::PlayerContext m_context;
    StrictMock<firebolt::rialto::server::mock::GstPlayerPrivateMock> m_gstPlayer;
    StrictMock<firebolt::rialto::server::mock::GstPlayerClientMock> m_gstPlayerClient;
    std::shared_ptr<firebolt::rialto::server::mock::GlibWrapperMock> m_glibWrapper{
        std::make_shared<StrictMock<firebolt::rialto::server::mock::GlibWrapperMock>>()};
    std::shared_ptr<firebolt::rialto::server::mock::GstWrapperMock> m_gstWrapper{
        std::make_shared<StrictMock<firebolt::rialto::server::mock::GstWrapperMock>>()};
    firebolt::rialto::server::PlayerTaskFactory m_sut{&m_gstPlayerClient, m_gstWrapper, m_glibWrapper};
};

TEST_F(PlayerTaskFactoryTest, ShouldCreateAttachSamples)
{
    auto task = m_sut.createAttachSamples(m_context, m_gstPlayer, {});
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::AttachSamples &>(*task));
}

TEST_F(PlayerTaskFactoryTest, ShouldCreateAttachSource)
{
    auto task = m_sut.createAttachSource(m_context, firebolt::rialto::server::Source{});
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::AttachSource &>(*task));
}

TEST_F(PlayerTaskFactoryTest, ShouldCreateEnoughData)
{
    auto task = m_sut.createEnoughData(m_context, nullptr);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::EnoughData &>(*task));
}

TEST_F(PlayerTaskFactoryTest, ShouldCreateEos)
{
    auto task = m_sut.createEos(m_context, m_gstPlayer, firebolt::rialto::MediaSourceType::AUDIO);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::Eos &>(*task));
}

TEST_F(PlayerTaskFactoryTest, ShouldCreateFinishSetupSource)
{
    auto task = m_sut.createFinishSetupSource(m_context, m_gstPlayer);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::FinishSetupSource &>(*task));
}

TEST_F(PlayerTaskFactoryTest, ShouldCreateHandleBusMessage)
{
    auto task = m_sut.createHandleBusMessage(m_context, m_gstPlayer, nullptr);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::HandleBusMessage &>(*task));
}

TEST_F(PlayerTaskFactoryTest, ShouldCreateNeedData)
{
    auto task = m_sut.createNeedData(m_context, nullptr);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::NeedData &>(*task));
}

TEST_F(PlayerTaskFactoryTest, ShouldCreatePause)
{
    auto task = m_sut.createPause(m_gstPlayer);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::Pause &>(*task));
}

TEST_F(PlayerTaskFactoryTest, ShouldCreatePlay)
{
    auto task = m_sut.createPlay(m_gstPlayer);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::Play &>(*task));
}

TEST_F(PlayerTaskFactoryTest, ShouldCreateReadShmDataAndAttachSamples)
{
    auto task = m_sut.createReadShmDataAndAttachSamples(m_context, m_gstPlayer, nullptr);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::ReadShmDataAndAttachSamples &>(*task));
}

TEST_F(PlayerTaskFactoryTest, ShouldCreateReportPosition)
{
    auto task = m_sut.createReportPosition(m_context);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::ReportPosition &>(*task));
}

TEST_F(PlayerTaskFactoryTest, ShouldCreateCheckAudioUnderflow)
{
    auto task = m_sut.createCheckAudioUnderflow(m_context, m_gstPlayer);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::CheckAudioUnderflow &>(*task));
}

TEST_F(PlayerTaskFactoryTest, ShouldCreateSetPosition)
{
    auto task = m_sut.createSetPosition(m_context, m_gstPlayer, 0);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::SetPosition &>(*task));
}

TEST_F(PlayerTaskFactoryTest, ShouldCreateSetupElement)
{
    auto task = m_sut.createSetupElement(m_context, m_gstPlayer, nullptr);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::SetupElement &>(*task));
}

TEST_F(PlayerTaskFactoryTest, ShouldCreateSetupSource)
{
    auto task = m_sut.createSetupSource(m_context, m_gstPlayer, nullptr);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::SetupSource &>(*task));
}

TEST_F(PlayerTaskFactoryTest, ShouldCreateSetVideoGeometry)
{
    auto task = m_sut.createSetVideoGeometry(m_context, m_gstPlayer, firebolt::rialto::server::Rectangle{});
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::SetVideoGeometry &>(*task));
}

TEST_F(PlayerTaskFactoryTest, ShouldCreateShutdown)
{
    auto task = m_sut.createShutdown(m_gstPlayer);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::Shutdown &>(*task));
}

TEST_F(PlayerTaskFactoryTest, ShouldCreateStop)
{
    auto task = m_sut.createStop(m_context, m_gstPlayer);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::Stop &>(*task));
}

TEST_F(PlayerTaskFactoryTest, ShouldCreateUnderflow)
{
    bool flag{false};
    auto task = m_sut.createUnderflow(m_gstPlayer, flag);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::Underflow &>(*task));
}

TEST_F(PlayerTaskFactoryTest, ShouldCreateSetPlaybackRate)
{
    auto task = m_sut.createSetPlaybackRate(m_context, 1.25);
    EXPECT_NE(task, nullptr);
    EXPECT_NO_THROW(dynamic_cast<firebolt::rialto::server::SetPlaybackRate &>(*task));
}
